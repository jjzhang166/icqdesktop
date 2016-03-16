#include "stdafx.h"

#include "contact_archive.h"
#include "messages_data.h"
#include "archive_index.h"
#include "history_message.h"

using namespace core;
using namespace archive;

contact_archive::contact_archive(const std::wstring& _archive_path)
	:	index_(new archive_index(_archive_path + L"/" + version_db_filename(L"_idx"))),
		data_(new messages_data(_archive_path + L"/" + version_db_filename(L"_db"))),
		state_(new archive_state(_archive_path + L"/" + version_db_filename(L"_ste"))),
		path_(_archive_path),
		local_loaded_(false)
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

		if (!index_->get_header(*iter_id, msg_header))
		{
			assert(!"message header not found");
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

void contact_archive::set_dlg_state(const dlg_state& _state)
{
	state_->set_state(_state);
}

void contact_archive::clear_dlg_state()
{
    state_->clear_state();
}

int contact_archive::insert_history_block(std::shared_ptr<archive::history_block> _data, /*out*/ headers_list& _inserted_messages)
{
	archive::history_block insert_data;
	insert_data.reserve(_data->size());

	message_header msg_header;

	for (auto iter_hm = _data->begin(); iter_hm != _data->end(); iter_hm++)
	{
		if (index_->get_header((*iter_hm)->get_msgid(), msg_header))
			continue;

		insert_data.push_back(*iter_hm);
	}

	if (insert_data.size())
	{
		if (!data_->update(insert_data))
		{
			assert(!"update data error");
			return -1;
		}

		if (!index_->update(insert_data, _inserted_messages))
		{
			assert(!"update index error");
			return -1;
		}
	}

	return 0;
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
