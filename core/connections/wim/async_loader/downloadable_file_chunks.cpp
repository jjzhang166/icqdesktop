#include "stdafx.h"

#include "downloadable_file_chunks.h"

core::wim::downloadable_file_chunks::downloadable_file_chunks()
    : priority_(default_priority)
    , downloaded_(0)
    , total_size_(0)
    , cancel_(true)
{
}

core::wim::downloadable_file_chunks::downloadable_file_chunks(priority_t _priority, const std::string& _url, const std::wstring& _file_name, int64_t _total_size)
    : priority_(_priority)
    , url_(_url)
    , file_name_(_file_name)
    , tmp_file_name_(_file_name + L".tmp")
    , downloaded_(0)
    , total_size_(_total_size)
    , cancel_(false)
{
}
