#include "stdafx.h"

#include "../../corelib/collection.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"

#include "../log/log.h"

#include "../tools/system.h"

#include "history_message.h"
#include "storage.h"

#include "not_sent_messages.h"

using namespace core;
using namespace archive;

namespace
{
	enum not_sent_message_fields
	{
		min,

		contact,
		message,

		max
	};
}

not_sent_message_sptr not_sent_message::make(const core::tools::tlvpack& _pack)
{
	const not_sent_message_sptr msg(new not_sent_message);
	if (msg->unserialize(_pack))
	{
		return msg;
	}

	return not_sent_message_sptr();
}

not_sent_message_sptr not_sent_message::make(const not_sent_message_sptr& _message, const std::string& _wimid, const uint64_t _time)
{
	return not_sent_message_sptr(new not_sent_message(_message, _wimid, _time));
}

not_sent_message_sptr not_sent_message::make(
	const std::string& _aimid,
	const std::string& _message,
	const message_type _type,
	const uint64_t _time,
    const std::string& _internal_id)
{
	return not_sent_message_sptr(new not_sent_message(_aimid, _message, _type, _time, _internal_id));
}

not_sent_message_sptr not_sent_message::make_outgoing_file_sharing(
	const std::string& _aimid,
	const uint64_t _time,
	const std::string& _local_path)
{
	assert(!_aimid.empty());
	assert(_time > 0);
	assert(!_local_path.empty());

    not_sent_message_sptr not_sent(
        new not_sent_message(_aimid, "", message_type::file_sharing, _time, "")
    );

    not_sent->get_message()->init_file_sharing_from_local_path(_local_path);

	return not_sent;
}

not_sent_message_sptr not_sent_message::make_incoming_file_sharing(
    const std::string& _aimid,
    const uint64_t _time,
    const std::string& _uri,
    const std::string &_internal_id)
{
    assert(!_aimid.empty());
    assert(_time > 0);
    assert(!_uri.empty());
    assert(!_internal_id.empty());

    not_sent_message_sptr not_sent(
        new not_sent_message(_aimid, _uri, message_type::file_sharing, _time, _internal_id)
    );

    not_sent->get_message()->init_file_sharing_from_link(_uri);

    return not_sent;
}

not_sent_message::not_sent_message()
	: message_(new history_message())
{
}

not_sent_message::not_sent_message(
	const std::string& _aimid,
	const std::string& _message,
	const message_type _type,
	const uint64_t _time,
    const std::string& _internal_id)
	: message_(new history_message())
	, aimid_(_aimid)
{
    auto internal_id = _internal_id;
    if (internal_id.empty())
        internal_id = core::tools::system::generate_internal_id();

    message_->set_internal_id(internal_id);

	message_->set_outgoing(true);
	message_->set_time(_time);

    if (history_message::is_file_sharing_uri(_message) && _type != message_type::file_sharing)
    {
        message_->init_file_sharing_from_link(_message);
        message_->set_text(_message);
        return;
    }

	if (_type == message_type::sticker)
	{
		message_->init_sticker_from_text(_message);
	}
	else
	{
		message_->set_text(_message);
	}
}

not_sent_message::not_sent_message(const not_sent_message_sptr& _message, const std::string& _wimid, const uint64_t time)
	: message_(new history_message())
{
	copy_from(_message);
	message_->set_wimid(_wimid);
	message_->set_time(time);
}

not_sent_message::~not_sent_message()
{
}

void not_sent_message::copy_from(const not_sent_message_sptr& _message)
{
	assert(_message);

	aimid_ = _message->aimid_;
	message_ = _message->message_;
}

const history_message_sptr& not_sent_message::get_message() const
{
	return message_;
}

const std::string& not_sent_message::get_aimid() const
{
	assert(!aimid_.empty());
	return aimid_;
}

const std::string& not_sent_message::get_internal_id() const
{
	return message_->get_internal_id();
}

const std::string& not_sent_message::get_file_sharing_local_path() const
{
	assert(message_);
	return message_->get_file_sharing_data()->get_local_path();
}

bool not_sent_message::is_ready_to_send() const
{
    const auto &message = get_message();
    assert(message);

    const auto &fs_data = message->get_file_sharing_data();
    if (fs_data && !fs_data->get_local_path().empty())
    {
        return false;
    }

    return true;
}

void not_sent_message::serialize(core::tools::tlvpack& _pack) const
{
	_pack.push_child(tools::tlv(not_sent_message_fields::contact, aimid_));

	core::tools::binary_stream bs_message;
	message_->serialize(bs_message);
	_pack.push_child(tools::tlv(not_sent_message_fields::message, bs_message));
}

void not_sent_message::serialize(core::coll_helper& _coll, const time_t _offset) const
{
	get_message()->serialize(_coll.get(), _offset);
}

bool not_sent_message::unserialize(const core::tools::tlvpack& _pack)
{
	auto tlv_aimid = _pack.get_item(not_sent_message_fields::contact);
	auto tlv_message = _pack.get_item(not_sent_message_fields::message);

	if (!tlv_message || !tlv_aimid)
		return false;

	aimid_ = tlv_aimid->get_value<std::string>();

    core::tools::binary_stream stream = tlv_message->get_value<core::tools::binary_stream>();
    return !message_->unserialize(stream);
}

not_sent_messages::not_sent_messages(const std::wstring& _file_name)
	:	storage_(new storage(_file_name)),
		is_loaded_(false)
{

}

not_sent_messages::~not_sent_messages()
{
}

void not_sent_messages::insert(const std::string& _aimid, const not_sent_message_sptr &_message)
{
	assert(_message);

	auto &contact_messages = messages_by_aimid_[_aimid];

	bool was_updated = false;
	for (auto &existing_message : contact_messages)
	{
		const auto have_same_internal_id = (_message->get_message()->get_internal_id() == existing_message->get_message()->get_internal_id());
		if (!have_same_internal_id)
		{
			continue;
		}

		assert(!"not sent message already exist");
		existing_message = _message;
		was_updated = true;
		break;
	}

	if (!was_updated)
	{
		contact_messages.push_back(_message);
	}

	save();
}

void not_sent_messages::update_if_exist(const std::string &_aimid, const not_sent_message_sptr &_message)
{
	assert(!_aimid.empty());
	assert(_message);

	auto iter_messages = messages_by_aimid_.find(_aimid);
	if (iter_messages == messages_by_aimid_.end())
	{
		return;
	}

	auto &messages = iter_messages->second;

	for (auto &message : messages)
	{
		if (_message->get_internal_id() == message->get_internal_id())
		{
			message = _message;

			save();

			return;
		}
	}
}

void not_sent_messages::load_if_need()
{
	if (!is_loaded_)
	{
		load();
	}

	is_loaded_ = true;
}

void not_sent_messages::remove(const std::string &_aimid, const history_block_sptr &_data)
{
	assert(!_aimid.empty());
	assert(_data);

	auto &messages = messages_by_aimid_[_aimid];
	if (messages.empty())
	{
		return;
	}

	bool save_on_exit = false;

	tools::auto_scope as(
		[&save_on_exit, this]
		{
			if (save_on_exit)
			{
				save();
			}
		}
	);

	for (const auto &block : *_data)
	{
		const auto &block_internal_id = block->get_internal_id();

		for (auto iter = messages.begin(); iter != messages.end();)
		{
			const auto &not_sent_message = **iter;

			const auto &message = *not_sent_message.get_message();

			const auto same_internal_id =
				(message.get_internal_id() == block_internal_id);

			if (same_internal_id)
			{
				save_on_exit = true;
				iter = messages.erase(iter);
				continue;
			}

			++iter;
		}
	}

	if (messages.empty())
	{
		messages_by_aimid_.erase(_aimid);
	}
}

bool not_sent_messages::exist(const std::string& _aimid) const
{
	auto iter_c = messages_by_aimid_.find(_aimid);
	if (iter_c == messages_by_aimid_.end())
		return false;

	return (!iter_c->second.empty());
}

void not_sent_messages::get_messages(const std::string& _aimid, Out history_block& _messages)
{
	auto iter_c = messages_by_aimid_.find(_aimid);
	if (iter_c == messages_by_aimid_.end())
		return;

	const not_sent_messages_list& messages_list = iter_c->second;

	_messages.reserve(messages_list.size());

	for (const auto& msg : messages_list)
	{
		_messages.push_back(std::make_shared<history_message>(*msg->get_message()));
	}
}

void not_sent_messages::get_pending_file_sharing_messages(not_sent_messages_list& _messages) const
{
    for (const auto &pair : messages_by_aimid_)
    {
        const auto &messages = pair.second;

        for (const auto& msg : messages)
        {
            if (msg->get_message()->is_file_sharing() && !msg->is_ready_to_send())
                _messages.push_back(msg);
        }
    }
}

not_sent_message_sptr not_sent_messages::get_first_ready_to_send() const
{
	for (const auto &pair : messages_by_aimid_)
	{
		const auto &messages = pair.second;

		const auto found = std::find_if(
			messages.begin(),
			messages.end(),
			[](const not_sent_message_sptr &_message)
			{
				return _message->is_ready_to_send();
			}
		);

		if (found != messages.end())
		{
			return *found;
		}
	}

	return not_sent_message_sptr();
}

not_sent_message_sptr not_sent_messages::get_by_iid(const std::string& _iid) const
{
	assert(!_iid.empty());

	for (const auto &pair : messages_by_aimid_)
	{
		const auto &messages = pair.second;

		const auto found = std::find_if(
			messages.begin(),
			messages.end(),
			[&_iid](const not_sent_message_sptr &_message)
			{
				return (_message->get_internal_id() == _iid);
			}
		);

		if (found != messages.end())
		{
			return *found;
		}
	}

	return not_sent_message_sptr();
}

bool not_sent_messages::save()
{
	archive::storage_mode mode;
	mode.flags_.write_ = true;
	mode.flags_.truncate_ = true;
	if (!storage_->open(mode))
	{
		return false;
	}

	auto p_storage = storage_.get();
	core::tools::auto_scope lb([p_storage] { p_storage->close(); });

	core::tools::binary_stream block_data;

	core::tools::tlvpack pack_root;

	for (const auto &pair : messages_by_aimid_)
	{
		const auto &messages = pair.second;

		for (const auto &message : messages)
		{
			core::tools::tlvpack pack_message;
			message->serialize(pack_message);

			core::tools::binary_stream bs_message;
			pack_message.serialize(bs_message);

			pack_root.push_child(core::tools::tlv(0, bs_message));
		}
	}

	pack_root.serialize(block_data);

	int64_t offset = 0;
	return storage_->write_data_block(block_data, offset);
}

bool not_sent_messages::load()
{
	archive::storage_mode mode;
	mode.flags_.read_ = true;

	if (!storage_->open(mode))
	{
		return false;
	}

	auto p_state_storage = storage_.get();
	core::tools::auto_scope lbs([p_state_storage]{ p_state_storage->close(); });

	core::tools::binary_stream bs_data;
	if (!storage_->read_data_block(-1, bs_data))
	{
		return false;
	}

	core::tools::tlvpack pack_root;
	if (!pack_root.unserialize(bs_data))
	{
		return false;
	}

	auto tlv_msg = pack_root.get_first();
	while (tlv_msg)
	{
		auto bs_message = tlv_msg->get_value<core::tools::binary_stream>();

		core::tools::tlvpack pack_message;

		if (pack_message.unserialize(bs_message))
		{
			auto msg = not_sent_message::make(pack_message);
			if (msg)
			{
				messages_by_aimid_[msg->get_aimid()].emplace_back(std::move(msg));
			}
		}

		tlv_msg = pack_root.get_next();
	}

	return true;
}
