#include "stdafx.h"

#include "../log/log.h"

#include "contact_archive.h"
#include "messages_data.h"
#include "archive_index.h"
#include "history_message.h"

using namespace core;
using namespace archive;

contact_archive::contact_archive(const std::wstring& _archive_path, const std::string& _contact_id)
    : index_(new archive_index(_archive_path + L"/" + version_db_filename(L"_idx")))
    , data_(new messages_data(_archive_path + L"/" + version_db_filename(L"_db")))
    , state_(new archive_state(_archive_path + L"/" + version_db_filename(L"_ste"), _contact_id))
    , path_(_archive_path)
    , local_loaded_(false)
{
}


contact_archive::~contact_archive()
{
}

void contact_archive::get_messages_index(int64_t _from, int64_t _count, headers_list& _headers) const
{
    index_->serialize_from(_from, _count, _headers);
}

bool contact_archive::get_messages_buddies(std::shared_ptr<archive::msgids_list> _ids, std::shared_ptr<history_block> _messages) const
{
    headers_list _headers;

    for (auto iter_id = _ids->begin(); iter_id != _ids->end(); iter_id++)
    {
        message_header msg_header;

        if (!index_->get_header(*iter_id, Out msg_header))
        {
            assert(!"message header not found");
            continue;
        }

        if (msg_header.is_patch())
        {
            continue;
        }

        _headers.emplace_back(msg_header);
    }

    data_->get_messages(_headers, *_messages);

    return true;
}

const dlg_state& contact_archive::get_dlg_state() const
{
    return state_->get_state();
}

void contact_archive::set_dlg_state(const dlg_state& _state, Out dlg_state_changes& _changes)
{
    state_->set_state(_state, Out _changes);
}

void contact_archive::clear_dlg_state()
{
    state_->clear_state();
}

bool contact_archive::update_dlg_state_last_message()
{
    const auto &dlg_state = state_->get_state();

    if (!dlg_state.has_last_msgid())
    {
        if (!dlg_state.get_last_message().has_msgid())
        {
            return false;
        }

        auto patched_dlg_state = dlg_state;

        patched_dlg_state.set_last_message(history_message());

        dlg_state_changes changes;
        state_->set_state(patched_dlg_state, Out changes);

        return true;
    }

    const auto last_msgid = dlg_state.get_last_msgid();
    assert(last_msgid > 0);

    if (!index_->has_header(last_msgid))
    {
        return false;
    }

    auto ids = std::make_shared<msgids_list>();
    ids->push_back(last_msgid);

    auto messages = std::make_shared<history_block>();
    get_messages_buddies(ids, Out messages);
    assert(messages->size() == 1);

    if (messages->empty())
    {
        return false;
    }

    const auto &message = *messages->at(0);
    assert(!message.is_deleted());
    assert(!message.is_patch());

    const auto is_same_message = message.contents_equal(dlg_state.get_last_message());
    if (is_same_message)
    {
        return false;
    }

    auto patched_dlg_state = dlg_state;

    patched_dlg_state.set_last_message(message);

    dlg_state_changes changes;
    state_->set_state(patched_dlg_state, Out changes);

    return true;
}

void contact_archive::insert_history_block(
    history_block_sptr _data,
    Out headers_list& _inserted_messages,
    Out dlg_state& _updated_state,
    Out dlg_state_changes& _state_changes
)
{
    Out _updated_state = state_->get_state();

    archive::history_block insert_data;
    insert_data.reserve(_data->size());

    const auto last_msgid = state_->get_state().get_last_msgid();
    auto last_message_updated = false;

    for (auto iter_msg = _data->begin(); iter_msg != _data->end(); iter_msg++)
    {
        const auto &message = *iter_msg;
        assert(message);
        assert(message->has_msgid());

        message_header existing_header;
        if (index_->get_header(message->get_msgid(), Out existing_header))
        {
            const auto is_patch_operation = (message->is_patch() ^ existing_header.is_patch());

            const auto skip_duplicate = !is_patch_operation;
            if (skip_duplicate)
            {
                // message is already in the db, and it's not a patch thus just skip it
                continue;
            }
        }

        const auto is_message_obsolete = (
            message->has_msgid() &&
            (message->get_msgid() <= _updated_state.get_del_up_to())
        );
        if (is_message_obsolete)
        {
            assert(message->is_patch());

            __INFO(
                "delete_history",
                "skipped obsolete message\n"
                "    id=<%1%>\n"
                "    is_patch=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % logutils::yn(message->is_patch()) % _updated_state.get_del_up_to()
            );

            continue;
        }

        const auto is_prev_id_obsolete = (
            message->has_prev_msgid() &&
            (message->get_prev_msgid() <= _updated_state.get_del_up_to())
        );
        if (is_prev_id_obsolete)
        {
            __INFO(
                "delete_history",
                "fixed message prev_msgid\n"
                "    id=<%1%>\n"
                "    prev_id=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % message->get_prev_msgid() % _updated_state.get_del_up_to()
            );

            message->set_prev_msgid(-1);
        }

        last_message_updated = (
            last_message_updated ||
            (message->get_msgid() == last_msgid)
        );

        insert_data.push_back(message);
    }

    if (insert_data.empty())
    {
        return;
    }

    if (!data_->update(insert_data))
    {
        assert(!"update data error");
        return;
    }

    if (!index_->update(insert_data, _inserted_messages))
    {
        assert(!"update index error");
        return;
    }

    const auto last_message_changed = (state_->get_state().get_last_message().get_msgid() != last_msgid);

    if (last_message_updated || last_message_changed)
    {
        _state_changes.last_message_changed_ = update_dlg_state_last_message();

        if (_state_changes.last_message_changed_)
        {
            Out _updated_state = state_->get_state();
        }
    }
}

int contact_archive::load_from_local()
{
    if (local_loaded_)
        return 0;

    local_loaded_ = true;

    if (!index_->load_from_local())
    {
        if (index_->get_last_error() != archive::error::file_not_exist)
        {
            assert(!"index file crash, need repair");
            index_->save_all();
        }
    }


    return 0;
}

bool contact_archive::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth)
{
    return index_->get_next_hole(_from, _hole, _depth);
}

bool contact_archive::need_optimize()
{
    return index_->need_optimize();
}

void contact_archive::optimize()
{
    index_->optimize();
}

void contact_archive::delete_messages_up_to(const int64_t _up_to)
{
    assert(_up_to > -1);

    auto dlg_state = state_->get_state();

    const auto &last_message = dlg_state.get_last_message();
    const auto is_last_message_obsolete = (last_message.has_msgid() && (_up_to >= last_message.get_msgid()));

    const auto is_last_msgid_obsolete = (dlg_state.has_last_msgid() && (_up_to >= dlg_state.get_last_msgid()));

    if (is_last_message_obsolete || is_last_msgid_obsolete)
    {
        dlg_state.clear_last_message();
        dlg_state.clear_last_msgid();

        dlg_state_changes changes;
        state_->set_state(dlg_state, Out changes);
    }

    index_->delete_up_to(_up_to);
}

std::wstring archive::version_db_filename(const std::wstring &filename)
{
    assert(!std::any_of(
        filename.begin(),
        filename.end(),
        [](const wchar_t _ch)
        {
            return ::iswdigit(_ch);
        }
    ));

    const auto DB_VERSION = 2;

    return (filename + std::to_wstring(DB_VERSION));
}