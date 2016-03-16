#include "stdafx.h"

#include <algorithm>


#include "archive_index.h"
#include "storage.h"
#include "options.h"


using namespace core;
using namespace archive;

//////////////////////////////////////////////////////////////////////////
// archive_index class
//////////////////////////////////////////////////////////////////////////

const int32_t max_index_size			= 25000;
const int32_t index_size_need_optimize	= 30000;

enum archive_index_types : uint32_t
{
	header		= 1,
	msgid		= 2,
	msgflags	= 3,
};

archive_index::archive_index(const std::wstring& _file_name)
	:	storage_(new storage(_file_name)),
		last_error_(archive::error::ok)
{
}


archive_index::~archive_index()
{
}

void archive_index::serialize(headers_list& _list) const
{
	for (auto iter_header = headers_index_.begin(); iter_header != headers_index_.end(); iter_header++)
		_list.emplace_back(iter_header->second);
}

bool archive_index::serialize_from(int64_t _from, int64_t _count, headers_list& _list) const
{
	if (headers_index_.empty())
		return true;

	headers_map::const_iterator iter_header;

	if (_from == -1)
	{
		iter_header = headers_index_.end();
	}
	else
	{
		iter_header = headers_index_.find(_from);
		if (iter_header == headers_index_.end())
		{
			assert(!"invalid index number");
			return false;
		}
	}

	if (iter_header == headers_index_.begin())
		return true;

	do
	{
		--iter_header;
		
		if (_count != -1 && _list.size() >= _count)
			return true;
			
		_list.emplace_front(iter_header->second);
	}
	while (iter_header != headers_index_.begin());

	return true;
}

bool archive_index::insert_block(const archive::headers_list& _inserted_headers)
{
	for (auto iter_header = _inserted_headers.begin(); iter_header != _inserted_headers.end(); iter_header++)
	{
		headers_index_.emplace(iter_header->get_id(), *iter_header);
	}
				
	return true;
}

bool archive_index::get_header(int64_t _msgid, message_header& _header) const
{
	auto iter_header = headers_index_.find(_msgid);
	if (iter_header == headers_index_.end())
		return false;

	_header = iter_header->second;
	
	return true;
}

bool archive_index::update(const archive::history_block& _data, /*out*/ headers_list& _headers)
{
	_headers.clear();

	for (auto iter_hm = _data.begin(); iter_hm != _data.end(); iter_hm++)
	{
		_headers.emplace_back(
			(*iter_hm)->get_flags(),
			(*iter_hm)->get_time(),
			(*iter_hm)->get_msgid(),
			(*iter_hm)->get_prev_msgid(),
			(*iter_hm)->get_data_offset(),
			(*iter_hm)->get_data_size());
	}
	
	insert_block(_headers);
	
	return save_block(_headers);
}


void archive_index::serialize_block(const headers_list& _headers, core::tools::binary_stream& _data) const
{
	core::tools::tlvpack tlv_headers;

	core::tools::binary_stream header_data;

	for (auto iter_header = _headers.begin(); iter_header != _headers.end(); iter_header++)
	{
		header_data.reset();
		iter_header->serialize(header_data);

		tlv_headers.push_child(core::tools::tlv(archive_index_types::header, header_data));
	}

	tlv_headers.serialize(_data);
}



bool archive_index::unserialize_block(core::tools::binary_stream& _data)
{
	uint32_t tlv_type = 0;
	uint32_t tlv_length = 0;
	archive::message_header msg_header;

	while (uint32_t available = _data.available())
	{
		if (available < sizeof(uint32_t)*2)
			return false;

		tlv_type = _data.read<uint32_t>();
		tlv_length = _data.read<uint32_t>();

		if (available < tlv_length)
			return false;

		if (!msg_header.unserialize(_data))
			return false;

		headers_index_.insert(std::make_pair(msg_header.get_id(), msg_header));
	}

	return true;
}

bool archive_index::save_block(const archive::headers_list& _block)
{
	archive::storage_mode mode;
	mode.flags_.write_ = true;
	mode.flags_.append_ = true;
	if (!storage_->open(mode))
		return false;

	auto p_storage = storage_.get();
	core::tools::auto_scope lb([p_storage]{p_storage->close();});

	core::tools::binary_stream block_data;
	serialize_block(_block, block_data);

	int64_t offset = 0;
	return storage_->write_data_block(block_data, offset);
}

bool archive_index::save_all()
{
	archive::storage_mode mode;
	mode.flags_.write_ = true;
	mode.flags_.truncate_ = true;
	if (!storage_->open(mode))
		return false;

	auto p_storage = storage_.get();
	core::tools::auto_scope lb([p_storage]{p_storage->close();});

	if (headers_index_.empty())
		return true;

	std::list<message_header> headers;

	auto iter_last = headers_index_.end();
	iter_last--;

	for (auto iter_header = headers_index_.begin(); iter_header != headers_index_.end(); iter_header++)
	{
		headers.emplace_back(iter_header->second);

		if (headers.size() >= history_block_size || iter_header == iter_last)
		{
			core::tools::binary_stream block_data;
			serialize_block(headers, block_data);

			int64_t offset = 0;
			if (!storage_->write_data_block(block_data, offset))
				return false;

			headers.clear();
		}
	}
		
	return true;
}

bool archive_index::load_from_local()
{
	last_error_ = archive::error::ok;

	archive::storage_mode mode;
	mode.flags_.read_ = true;

	if (!storage_->open(mode))
	{
		last_error_ = storage_->get_last_error();
		return false;
	}
	
	auto p_storage = storage_.get();
	core::tools::auto_scope lb([p_storage]{p_storage->close();});	
	
	core::tools::binary_stream data_stream;
	while (storage_->read_data_block(-1, data_stream))
	{
		if (!unserialize_block(data_stream))
			return false;

		data_stream.reset();
	}

	if (storage_->get_last_error() != archive::error::end_of_file)
	{
		last_error_ = storage_->get_last_error();
		return false;
	}
		
	return true;
}

int64_t archive_index::get_last_msgid()
{
	if (headers_index_.empty())
		return -1;

	return headers_index_.rbegin()->first;
}

bool archive_index::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const
{
	if (headers_index_.empty())
		return true;

	headers_map::const_iterator iter_header = headers_index_.cend();

	int64_t current_depth = 0;

	if (_from == -1)
	{
		--iter_header;
	}
	else
	{
		--iter_header;

		if (iter_header->first < _from) // if "from" from dlg_state 
		{
			_hole.set_from(-1);
			_hole.set_to(iter_header->second.get_id());

			return true;
		}
		else
		{
			iter_header = headers_index_.find(_from);
			if (iter_header == headers_index_.cend())
			{
				assert(!"index not found");
				return false;
			}
		}
	}
	
	do 
	{
		current_depth++;

		if (iter_header == headers_index_.begin())
		{
			if (iter_header->second.get_prev_msgid() != -1)
			{
				_hole.set_from(iter_header->second.get_id());
				return true;
			}
		}
		else
		{
			auto iter_prev_header = iter_header;
			--iter_prev_header;

			if (iter_header->second.get_prev_msgid() != iter_prev_header->second.get_id())
			{
				_hole.set_from(iter_header->second.get_id());
				_hole.set_to(iter_prev_header->second.get_id());
				_hole.set_depth(current_depth);
								
				return true;
			}
		}

		if (_depth != -1 && current_depth >= _depth)
			break;
	} 
	while (iter_header-- != headers_index_.begin());
	
	return false;
}

bool archive_index::need_optimize()
{
	assert(max_index_size < index_size_need_optimize);
	assert(max_index_size > 0);
	assert(index_size_need_optimize > 0);
	if (max_index_size >= index_size_need_optimize || index_size_need_optimize <= 0 || max_index_size <= 0)
		return false;

	return (headers_index_.size() > index_size_need_optimize);
}

void archive_index::optimize()
{
	if (!need_optimize())
		return;

	if (headers_index_.size() > index_size_need_optimize)
	{
		auto iter = headers_index_.end();
		for (int32_t i = 0; i < max_index_size && iter != headers_index_.begin(); i++)
		{
			--iter;
		}
		
		headers_index_.erase(headers_index_.begin(), iter);
		
		assert(!headers_index_.empty());
		if (!headers_index_.empty())
			headers_index_.begin()->second.set_prev_msgid(-1);

		save_all();
	}
}
