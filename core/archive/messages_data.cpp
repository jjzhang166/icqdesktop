#include "stdafx.h"
#include "messages_data.h"
#include "storage.h"
#include "archive_index.h"

using namespace core;
using namespace archive;

messages_data::messages_data(const std::wstring& _file_name)
	:	storage_(new storage(_file_name))
{
}


messages_data::~messages_data()
{
}


bool messages_data::get_messages(headers_list& _headers, history_block& _messages)
{
	auto p_storage = storage_.get();
	archive::storage_mode mode;
	mode.flags_.read_ = mode.flags_.append_ = true;
	if (!storage_->open(mode))
		return false;
	core::tools::auto_scope lb([p_storage]{p_storage->close();});

	_messages.reserve(_headers.size());

	bool res = true;

	core::tools::binary_stream message_data;

	for (auto iter_header = _headers.begin(); iter_header != _headers.end(); iter_header++)
	{
		message_data.reset();

		if (!storage_->read_data_block(iter_header->get_data_offset(), message_data))
		{
			assert(!"invalid message data");
			res = false;
			continue;
		}

		auto msg = std::make_shared<history_message>();
		if (msg->unserialize(message_data) != 0)
		{
			assert(!"unserialize message error");
			continue;
		}

		if (msg->get_msgid() != iter_header->get_id())
		{
			assert(!"message data invalid");
			continue;
		}

		_messages.push_back(msg);
	}

	return res;
}

bool messages_data::update(const archive::history_block& _data)
{
	auto p_storage = storage_.get();
	archive::storage_mode mode;
	mode.flags_.write_ = mode.flags_.append_ = true;
	if (!storage_->open(mode))
		return false;
	core::tools::auto_scope lb([p_storage]{p_storage->close();});
	
	core::tools::binary_stream message_data;

	for (auto iter_hm = _data.begin(); iter_hm != _data.end(); iter_hm++)
	{
		auto msg = *iter_hm;

		message_data.reset();
		msg->serialize(message_data);

		int64_t offset = 0;
		if (!storage_->write_data_block(message_data, offset))
			return false;

		msg->set_data_offset(offset);
		msg->set_data_size(message_data.available());
	}

	return true;
}