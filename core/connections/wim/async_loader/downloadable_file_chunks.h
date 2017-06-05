#pragma once

#include "async_handler.h"
#include "downloaded_file_info.h"

namespace core
{
    namespace wim
    {
        struct downloadable_file_chunks
        {
            downloadable_file_chunks();
            downloadable_file_chunks(priority_t _priority, const std::string& _url, const std::wstring& _file_name, int64_t _total_size);

            priority_t priority_;

            std::string url_;

            std::wstring file_name_;
            std::wstring tmp_file_name_;

            int64_t downloaded_;
            int64_t total_size_;

            bool cancel_;

            std::vector<async_handler<downloaded_file_info>> handlers_;
        };

        typedef std::shared_ptr<downloadable_file_chunks> downloadable_file_chunks_ptr;
    }
}
