#ifndef __ARCHIVE_STORAGE_H_
#define __ARCHIVE_STORAGE_H_

#pragma once

#include "errors.h"

namespace core
{
    namespace archive
    {
        class storage_data_block
        {
            core::tools::binary_stream	data_;
        };


        union storage_mode
        {
            struct
            {
                uint32_t	read_		: 1;
                uint32_t	write_		: 1;
                uint32_t	append_		: 1;
                uint32_t	truncate_	: 1;

            } flags_;

            uint32_t	value_;

            storage_mode()
                :	value_(0)
            {
            }
        };

        class storage
        {
            const std::wstring					file_name_;
            std::list<storage_data_block>		data_list_;
            std::unique_ptr<std::fstream>		active_file_stream_;

            archive::error						last_error_;

        public:

            void clear();

            bool open(storage_mode _mode);
            void close();

            bool write_data_block(core::tools::binary_stream& _data, int64_t& _offset);
            bool read_data_block(int64_t _offset, core::tools::binary_stream& _data);

            archive::error get_last_error() { return last_error_; }

            const std::wstring& get_file_name() const { return file_name_; }

            storage(const std::wstring& _file_name);
            virtual ~storage();
        };

    }
}

#endif //__ARCHIVE_STORAGE_H_