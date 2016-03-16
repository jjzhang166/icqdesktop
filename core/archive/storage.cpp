#include "stdafx.h"
#include "storage.h"

using namespace core;
using namespace archive;

const int32_t max_data_block_size = (1024 * 1024);

storage::storage(const std::wstring& _file_name)
	:	file_name_(_file_name), last_error_(archive::error::ok)
{
}


storage::~storage()
{
}

void storage::clear()
{
	
}

bool storage::open(storage_mode _mode)
{
	last_error_ = archive::error::ok;

	if (active_file_stream_)
	{
		assert(!"file stream already opened");
		return false;
	}

	boost::filesystem::wpath path_for_file(file_name_);
	std::wstring forder_name = path_for_file.parent_path().wstring();
	boost::filesystem::wpath path_for_folder(forder_name);

	if (_mode.flags_.read_ && !_mode.flags_.write_ && !boost::filesystem::exists(path_for_file))
	{
		last_error_ = archive::error::file_not_exist;
		return false;
	}

	if (!boost::filesystem::exists(path_for_folder))
	{
		if (!boost::filesystem::create_directories(path_for_folder))
		{
			last_error_ = archive::error::create_directory_error;
			return false;
		}
	}
	
	std::ios_base::openmode open_mode = std::fstream::binary;

	if (_mode.flags_.read_)
		open_mode |= std::fstream::in;
	if (_mode.flags_.write_)
		open_mode |= std::fstream::out;
	if (_mode.flags_.append_)
		open_mode |= std::fstream::app;
	if (_mode.flags_.truncate_)
		open_mode |= std::fstream::trunc;

#ifdef _WIN32
	active_file_stream_.reset(new std::fstream(file_name_, open_mode));
#else
	active_file_stream_.reset(new std::fstream(tools::from_utf16(file_name_), open_mode));
#endif

	if (!active_file_stream_->is_open())
	{
		last_error_ = archive::error::open_file_error;
		active_file_stream_.reset();
		return false;
	}

	if (_mode.flags_.append_ && _mode.flags_.write_)
		active_file_stream_->seekp(0, std::ios::end);

	return true;
}

void storage::close()
{
	if (!active_file_stream_)
	{
		assert(!"file stream not opened");
		return;
	}

	active_file_stream_->close();
	active_file_stream_.reset();
}

bool storage::write_data_block(core::tools::binary_stream& _data, int64_t& _offset)
{
	if (!active_file_stream_)
	{
		assert(!"file stream not opened");
		return false;
	}

	_offset = active_file_stream_->tellp();

	uint32_t data_size = _data.available();

	active_file_stream_->write((const char*) &data_size, sizeof(data_size));
	active_file_stream_->write((const char*) &data_size, sizeof(data_size));

	if (data_size)
		active_file_stream_->write((const char*) _data.read(data_size), data_size);

	active_file_stream_->write((const char*) &data_size, sizeof(data_size));
	active_file_stream_->write((const char*) &data_size, sizeof(data_size));
		
	return true;
}

bool storage::read_data_block(int64_t _offset, core::tools::binary_stream& _data)
{
	if (_offset != -1)
		active_file_stream_->seekp(_offset);
	
	if (active_file_stream_->peek() == EOF)
	{
		last_error_ = archive::error::end_of_file;
		return false;
	}

	uint32_t sz1 = 0, sz2 = 0;
	active_file_stream_->read((char*) &sz1, sizeof(sz1));
	if (!active_file_stream_->good())
		return false;

	active_file_stream_->read((char*) &sz2, sizeof(sz2));
	if (!active_file_stream_->good())
		return false;

	if (sz1 != sz2 || sz1 > max_data_block_size)
		return false;

	if (sz1 != 0)
	{
		active_file_stream_->read(_data.alloc_buffer(sz1), sz1);
		if (!active_file_stream_->good())
			return false;
	}
		
	uint32_t sz3 = 0, sz4 = 0;
	active_file_stream_->read((char*) &sz3, sizeof(sz3));
	if (!active_file_stream_->good())
		return false;

	active_file_stream_->read((char*) &sz4, sizeof(sz4));
	if (!active_file_stream_->good())
		return false;

	if (sz1 != sz3 || sz1 != sz4)
		return false;

	return true;
}