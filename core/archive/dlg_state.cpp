#include "stdafx.h"
#include "dlg_state.h"
#include "../../corelib/core_face.h"
#include "../../corelib/collection_helper.h"
#include "storage.h"
#include "../../core/core.h"
#include "history_message.h"

using namespace core;
using namespace archive;

namespace
{
	const int offline_timeout = 30 * 60;//30 min
}

enum dlg_state_fields
{
	unreads_count			= 1,
	last_msg_id				= 2,
	yours_last_read			= 3,
	theirs_last_read		= 4,
	theirs_last_delivered	= 5,
	last_message			= 7,
	visible					= 8,
	last_message_friendly	= 9
};

dlg_state::dlg_state()
	:
	unread_count_(0),
	last_msgid_(-1), 
	yours_last_read_(-1),
	theirs_last_read_(-1),
	theirs_last_delivered_(-1),
	last_message_(new history_message()),
	visible_(true)
{
}

dlg_state::~dlg_state()
{

}

dlg_state::dlg_state(const dlg_state& _state)
{
	last_message_.reset(new history_message());
	copy_from(_state);
}

void dlg_state::copy_from(const dlg_state& _state)
{
	unread_count_ = _state.unread_count_;
	last_msgid_ = _state.last_msgid_;
	yours_last_read_ = _state.yours_last_read_;
	theirs_last_read_ = _state.theirs_last_read_;
	theirs_last_delivered_ = _state.theirs_last_delivered_;
	visible_ = _state.visible_;
	*last_message_ = *_state.last_message_;
	last_message_friendly_ = _state.last_message_friendly_;
}

dlg_state& dlg_state::operator=(const dlg_state& _state)
{
	copy_from(_state);

	return * this;
}

const history_message& dlg_state::get_last_message() const
{
	return *last_message_;
}

void dlg_state::set_last_message(const history_message& _message)
{
	*last_message_ = _message;
}

const std::string& dlg_state::get_last_message_friendly() const
{
	return last_message_friendly_;
}

void dlg_state::set_last_message_friendly(const std::string& _friendly)
{
	last_message_friendly_ = _friendly;
}

void dlg_state::serialize(icollection* _collection, const time_t _offset, const time_t _last_successful_fetch, bool _serialize_message) const
{
	coll_helper coll(_collection, false);

	coll.set_value_as_int64("unreads", get_unread_count());
	coll.set_value_as_int64("last_msg_id", get_last_msgid());
	coll.set_value_as_int64("yours_last_read", get_yours_last_read());
	coll.set_value_as_int64("theirs_last_read", get_theirs_last_read());
	coll.set_value_as_int64("theirs_last_delivered", get_theirs_last_delivered());
	coll.set_value_as_string("last_message_friendly", get_last_message_friendly());

	auto cur_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + _offset;
	bool timed_out = cur_time - _last_successful_fetch >= offline_timeout;
	coll.set_value_as_bool("visible", get_visible() && !timed_out);

	coll_helper msg_coll(g_core->create_collection(), true);
	get_last_message().serialize(msg_coll.get(), _offset, _serialize_message);
	coll.set_value_as_collection("message", msg_coll.get());
}

void dlg_state::serialize(core::tools::binary_stream& _data) const
{
	core::tools::tlvpack state_pack;
	state_pack.push_child(core::tools::tlv(dlg_state_fields::unreads_count, get_unread_count()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::last_msg_id, get_last_msgid()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::yours_last_read, get_yours_last_read()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::theirs_last_read, get_theirs_last_read()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::theirs_last_delivered, get_theirs_last_delivered()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::visible, get_visible()));
	state_pack.push_child(core::tools::tlv(dlg_state_fields::last_message_friendly, get_last_message_friendly()));

	core::tools::binary_stream bs_message;
	get_last_message().serialize(bs_message);
	state_pack.push_child(core::tools::tlv(dlg_state_fields::last_message, bs_message));

	state_pack.serialize(_data);
}

bool dlg_state::unserialize(core::tools::binary_stream& _data)
{
	core::tools::tlvpack state_pack;
	if (!state_pack.unserialize(_data))
		return false;

	auto tlv_unreads_count = state_pack.get_item(dlg_state_fields::unreads_count);
	auto tlv_last_msg_id = state_pack.get_item(dlg_state_fields::last_msg_id);
	auto tlv_yours_last_read = state_pack.get_item(dlg_state_fields::yours_last_read);
	auto tlv_theirs_last_read = state_pack.get_item(dlg_state_fields::theirs_last_read);
	auto tlv_theirs_last_delivered = state_pack.get_item(dlg_state_fields::theirs_last_delivered);
	auto tlv_last_message = state_pack.get_item(dlg_state_fields::last_message);
	auto tlv_visible = state_pack.get_item(dlg_state_fields::visible);
	auto tlv_last_message_friendly = state_pack.get_item(dlg_state_fields::last_message_friendly);
	
	if (!tlv_unreads_count || !tlv_last_msg_id || !tlv_yours_last_read || !tlv_theirs_last_read ||
		!tlv_theirs_last_delivered || !tlv_last_message || !tlv_visible)
		return false;

	set_unread_count(tlv_unreads_count->get_value<uint32_t>(0));
	set_last_msgid(tlv_last_msg_id->get_value<int64_t>(0));
	set_yours_last_read(tlv_yours_last_read->get_value<int64_t>(0));
	set_theirs_last_read(tlv_theirs_last_read->get_value<int64_t>(0));
	set_theirs_last_delivered(tlv_theirs_last_delivered->get_value<int64_t>(0));
	set_visible(tlv_visible->get_value<bool>(true));

	if (tlv_last_message_friendly)
		set_last_message_friendly(tlv_last_message_friendly->get_value<std::string>());
	
	core::tools::binary_stream bs_message = tlv_last_message->get_value<core::tools::binary_stream>();
	last_message_->unserialize(bs_message);

	return true;
}







archive_state::archive_state(const std::wstring& _file_name)
	:	storage_(new storage(_file_name))
{

}

archive_state::~archive_state()
{

}

bool archive_state::save()
{
	if (!state_)
	{
		assert(!"dlg_state not intialized");
		return false;
	}

	archive::storage_mode mode;
	mode.flags_.write_ = true;
	mode.flags_.truncate_ = true;
	if (!storage_->open(mode))
		return false;

	auto p_storage = storage_.get();
	core::tools::auto_scope lb([p_storage]{p_storage->close();});

	core::tools::binary_stream block_data;
	state_->serialize(block_data);

	int64_t offset = 0;
	return storage_->write_data_block(block_data, offset);
}

bool archive_state::load()
{
	archive::storage_mode mode;
	mode.flags_.read_ = true;

	if (!storage_->open(mode))
		return false;

	auto p_state_storage = storage_.get();
	core::tools::auto_scope lbs([p_state_storage]{p_state_storage->close();});	

	core::tools::binary_stream state_stream;
	if (!storage_->read_data_block(-1, state_stream))
		return false;

	return state_->unserialize(state_stream);
}

const dlg_state& archive_state::get_state()
{
	if (!state_)
	{
		state_.reset(new dlg_state());
		
		load();
	}

	return *state_;
}

void archive_state::merge_state(const dlg_state& _state)
{
	state_->set_unread_count(_state.get_unread_count());
	state_->set_last_msgid(_state.get_last_msgid());
	state_->set_yours_last_read(_state.get_yours_last_read());
	state_->set_theirs_last_read(_state.get_theirs_last_read());
	state_->set_theirs_last_delivered(_state.get_theirs_last_delivered());
	state_->set_visible(_state.get_visible());
	

	if (_state.get_last_message().get_msgid() > 0 || !_state.get_last_message().get_internal_id().empty())
		state_->set_last_message(_state.get_last_message());

	if (!_state.get_last_message().get_sender_friendly().empty())
		state_->set_last_message_friendly(_state.get_last_message().get_sender_friendly());
}

void archive_state::set_state(const dlg_state& _state)
{
	if (!state_)
		state_.reset(new dlg_state());
	
	merge_state(_state);
	
	save();
}

void archive_state::clear_state()
{
    if (state_)
        *state_ = dlg_state();

    save();
}
