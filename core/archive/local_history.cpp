#include "stdafx.h"
#include "local_history.h"
#include "history_message.h"
#include "contact_archive.h"
#include "archive_index.h"
#include "not_sent_messages.h"
#include "../../corelib/collection_helper.h"
#include "../log/log.h"

using namespace core;
using namespace archive;

local_history::local_history(const std::wstring& _archive_path)
    :	archive_path_(_archive_path)
{
}


local_history::~local_history()
{
}

std::shared_ptr<contact_archive> local_history::get_contact_archive(const std::string& _contact)
{
    // load contact archive, insert to map

    auto iter_arch = archives_.find(_contact);
    if (iter_arch != archives_.end())
        return iter_arch->second;

    std::wstring contact_folder = core::tools::from_utf8(_contact);
    std::replace(contact_folder.begin(), contact_folder.end(), L'|', L'_');
    auto contact_arch = std::make_shared<contact_archive>(archive_path_ + L"/" + contact_folder);

    archives_.insert(std::make_pair(_contact, contact_arch));

    return contact_arch;
}

void local_history::update_history(const std::string& _contact, std::shared_ptr<archive::history_block> _data, /*out*/ headers_list& _inserted_messages)
{
    get_contact_archive(_contact)->insert_history_block(_data, _inserted_messages);
}

void local_history::get_messages_index(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ headers_list& _headers)
{
    get_contact_archive(_contact)->load_from_local();
    get_contact_archive(_contact)->get_messages_index(_from, _count, _headers);
}

bool local_history::get_messages(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ std::shared_ptr<history_block> _messages)
{
    headers_list headers;

    auto archive = get_contact_archive(_contact);
    if (!archive)
        return false;

    archive->load_from_local();
    archive->get_messages_index(_from, _count, headers);

    auto ids_list = std::make_shared<archive::msgids_list>();

    for (auto header : headers)
        ids_list->push_back(header.get_id());

    archive->get_messages_buddies(ids_list, _messages);

    return true;
}

void local_history::get_messages_buddies(
    const std::string& _contact,
    std::shared_ptr<archive::msgids_list> _ids,
    /*out*/ std::shared_ptr<history_block> _messages)
{
    get_contact_archive(_contact)->get_messages_buddies(_ids, _messages);
}

void local_history::get_dlg_state(const std::string& _contact, /*out*/ dlg_state& _state)
{
    _state = get_contact_archive(_contact)->get_dlg_state();
}

bool local_history::set_dlg_state(const std::string& _contact, const dlg_state& _state)
{
    get_contact_archive(_contact)->set_dlg_state(_state);

    return true;
}


bool local_history::clear_dlg_state(const std::string& _contact)
{
    get_contact_archive(_contact)->clear_dlg_state();

    return true;
}



std::shared_ptr<archive_hole> local_history::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto hole = std::make_shared<archive_hole>();

    if (get_contact_archive(_contact)->get_next_hole(_from, *hole, _depth))
        return hole;

    return nullptr;
}

void local_history::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    const std::string c_headers = "headers";
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (auto iter_header = _headers->begin(); iter_header != _headers->end(); iter_header++)
    {
        std::unique_ptr<hheader> hh(new hheader());
        hh->id_ = iter_header->get_id();
        hh->prev_id_ = iter_header->get_prev_msgid();
        hh->time_ = iter_header->get_time();
        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders(c_headers, val_headers.get());
}


void local_history::serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll)
{
    const std::string c_headers = "headers";
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (auto iter = _data->begin(); iter != _data->end(); iter++)
    {
        std::unique_ptr<hheader> hh(new hheader());
        hh->id_ = (*iter)->get_msgid();
        hh->prev_id_ = (*iter)->get_prev_msgid();
        hh->time_ = (*iter)->get_time();

        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders(c_headers, val_headers.get());
}

not_sent_messages& local_history::get_pending_messages()
{
    if (!not_sent_messages_)
    {
        not_sent_messages_.reset(new not_sent_messages(archive_path_ + L"/" + L"pending.db"));
        not_sent_messages_->load_if_need();
    }

    return *not_sent_messages_;
}

int32_t local_history::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    get_pending_messages().insert(_contact, _msg);
    return 0;
}


int32_t local_history::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    get_pending_messages().update_if_exist(_contact, _msg);
    return 0;
}


not_sent_message_sptr local_history::get_first_message_to_send()
{
    return get_pending_messages().get_first_ready_to_send();
}

not_sent_message_sptr local_history::get_not_sent_message_by_iid(const std::string& _iid)
{
    return get_pending_messages().get_by_iid(_iid);
}

void local_history::get_pending_file_sharing(std::list<not_sent_message_sptr>& _messages)
{
    return get_pending_messages().get_pending_file_sharing_messages(_messages);
}

int32_t local_history::remove_messages_from_not_sent(const std::string& _contact, archive::history_block_sptr _data)
{
    get_pending_messages().remove(_contact, _data);
    return 0;
}

bool local_history::has_not_sent_messages(const std::string& _contact)
{
    return get_pending_messages().exist(_contact);
}

void local_history::get_not_sent_messages(const std::string& _contact, /*out*/ std::shared_ptr<history_block> _messages)
{
    get_pending_messages().get_messages(_contact, *_messages);
}

void local_history::optimize_contact_archive(const std::string& _contact)
{
    get_contact_archive(_contact)->optimize();
}









face::face(const std::wstring& _archive_path)
    :	thread_(new core::async_executer()),
    history_cache_(new local_history(_archive_path))
{

}

std::shared_ptr<update_history_handler> face::update_history(const std::string& _contact, std::shared_ptr<archive::history_block> _data)
{
    __LOG(core::log::info("archive", boost::format("update_history, contact=%1%") % _contact);)

        auto handler = std::make_shared<update_history_handler>();
    auto history_cache = history_cache_;
    auto ids = std::make_shared<headers_list>();

    thread_->run_async_function([history_cache, _data, _contact, ids]()->int32_t
    {
        history_cache->update_history(_contact, _data, *ids);
        return 0;

    })->on_result_ = [handler, ids](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(ids);
    };

    return handler;
}

std::shared_ptr<request_headers_handler> face::get_messages_index(const std::string& _contact, int64_t _from, int64_t _count)
{
    __LOG(core::log::info("archive", boost::format("get_history, contact=%1%") % _contact);)

        auto history_cache = history_cache_;
    auto handler = std::make_shared<request_headers_handler>();
    auto headers = std::make_shared<headers_list>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([history_cache, _contact, headers, _from, _count]()->int32_t
    {
        history_cache->get_messages_index(_contact, _from, _count, *headers);
        return 0;

    })->on_result_ = [wr_this, history_cache, handler, headers, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->thread_->run_async_function([history_cache, _contact]()->int32_t
        {
            history_cache->optimize_contact_archive(_contact);
            return 0;

        });

        if (handler->on_result)
            handler->on_result(headers);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids)
{
    auto handler = std::make_shared<request_buddies_handler>();
    auto history_cache = history_cache_;
    auto out_messages = std::make_shared<history_block>();

    thread_->run_async_function([history_cache, _contact, _ids, out_messages]()->int32_t
    {
        history_cache->get_messages_buddies(_contact, _ids, out_messages);
        return 0;

    })->on_result_ = [handler, out_messages](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages(const std::string& _contact, int64_t _from, int64_t _count)
{
    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([_contact, out_messages, _from, _count, history_cache]()->int32_t
    {
        return (history_cache->get_messages(_contact, _from, _count, out_messages) ? 0 : -1);

    })->on_result_ = [wr_this, handler, out_messages, _contact, history_cache](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->thread_->run_async_function([history_cache, _contact]()->int32_t
        {
            history_cache->optimize_contact_archive(_contact);
            return 0;
        });

        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}

std::shared_ptr<request_dlg_state_handler> face::get_dlg_state(const std::string& _contact)
{
    auto handler = std::make_shared<request_dlg_state_handler>();
    auto history_cache = history_cache_;
    auto dialog = std::make_shared<dlg_state>();

    thread_->run_async_function([history_cache, dialog, _contact]()->int32_t
    {
        history_cache->get_dlg_state(_contact, *dialog);

        return 0;

    })->on_result_ = [handler, dialog](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*dialog);
    };

    return handler;
}

std::shared_ptr<set_dlg_state_handler> face::set_dlg_state(const std::string& _contact, const dlg_state& _state)
{
    auto handler = std::make_shared<set_dlg_state_handler>();
    auto history_cache = history_cache_;

    thread_->run_async_function([history_cache, _state, _contact]()->int32_t
    {
        return (history_cache->set_dlg_state(_contact, _state) ? 0 : -1);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result();
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::clear_dlg_state(const std::string& _contact)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([history_cache, _contact]()->int32_t
    {
        return (history_cache->clear_dlg_state(_contact) ? 0 : -1);

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(_error);
    };

    return handler;
}



std::shared_ptr<request_next_hole_handler> face::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto handler = std::make_shared<request_next_hole_handler>();
    auto history_cache = history_cache_;
    auto hole = std::make_shared<archive_hole>();

    thread_->run_async_function([history_cache, hole, _contact, _from, _depth]()->int32_t
    {
        auto new_hole = history_cache->get_next_hole(_contact, _from, _depth);

        if (new_hole)
        {
            *hole = *new_hole;
            return 0;
        }

        return -1;

    })->on_result_ = [handler, hole](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result( (_error == 0) ? hole : nullptr);
    };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_pending_message()
{
    auto handler = std::make_shared<not_sent_messages_handler>();
    auto history_cache = history_cache_;
    auto message = std::make_shared<not_sent_message_sptr>();

    auto task = thread_->run_async_function(
        [history_cache, message]
    {
        *message = history_cache->get_first_message_to_send();
        return (*message) ? 0 : -1;
    }
    );

    task->on_result_ = [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_not_sent_message_by_iid(const std::string& _iid)
{
    assert(!_iid.empty());

    auto handler = std::make_shared<not_sent_messages_handler>();
    auto history_cache = history_cache_;
    auto message = std::make_shared<not_sent_message_sptr>();

    auto task = thread_->run_async_function(
        [history_cache, message, _iid]
    {
        *message = history_cache->get_not_sent_message_by_iid(_iid);
        return (*message) ? 0 : -1;
    }
    );

    task->on_result_ =
        [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _msg, history_cache]()->int32_t
    {
        return history_cache->insert_not_sent_message(_contact, _msg);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _msg, history_cache]()->int32_t
    {
        return history_cache->update_if_exist_not_sent_message(_contact, _msg);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::remove_messages_from_not_sent(const std::string& _contact, archive::history_block_sptr _data)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _data, history_cache]()->int32_t
    {
        return history_cache->remove_messages_from_not_sent(_contact, _data);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::remove_message_from_not_sent(const std::string& _contact, const history_message_sptr _data)
{
    auto block = std::make_shared<history_block>();
    block->push_back(std::const_pointer_cast<history_message>(_data));

    return remove_messages_from_not_sent(_contact, block);
}

void face::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    local_history::serialize(_headers, _coll);
}

void face::serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll)
{
    local_history::serialize_headers(_data, _coll);
}

std::shared_ptr<has_not_sent_handler> face::has_not_sent_messages(const std::string& _contact)
{
    auto handler = std::make_shared<has_not_sent_handler>();
    auto history_cache = history_cache_;
    auto res = std::make_shared<bool>(false);

    thread_->run_async_function([_contact, history_cache, res]()->int32_t
    {
        *res = history_cache->has_not_sent_messages(_contact);

        return 0;

    })->on_result_ = [handler, res](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*res);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_not_sent_messages(const std::string& _contact)
{
    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();

    thread_->run_async_function([_contact, out_messages, history_cache]()->int32_t
    {
        history_cache->get_not_sent_messages(_contact, out_messages);

        return 0;

    })->on_result_ = [handler, out_messages](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}


std::shared_ptr<pending_messages_handler> face::get_pending_file_sharing()
{
    auto history_cache = history_cache_;
    auto handler = std::make_shared<pending_messages_handler>();
    auto messages_list = std::make_shared<std::list<not_sent_message_sptr>>();

    thread_->run_async_function([messages_list, history_cache]()->int32_t
    {
        history_cache->get_pending_file_sharing(*messages_list);

        return 0;

    })->on_result_ = [handler, messages_list](int32_t _error)
    {
        handler->on_result(*messages_list);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::sync_with_history()
{
    auto handler = std::make_shared<async_task_handlers>();

    thread_->run_async_function([]()->int32_t
    {
        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(_error);
    };

    return handler;
}