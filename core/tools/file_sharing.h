#pragma once

#include "../namespaces.h"

CORE_TOOLS_NS_BEGIN

enum class file_sharing_preview_size
{
    min,

    small,
    normal,

    max
};

std::string format_file_sharing_preview_uri(const std::string &_id, const file_sharing_preview_size _size);

bool is_new_file_sharing_uri(const std::string &_uri);

bool parse_new_file_sharing_uri(const std::string &_uri, Out std::string &_fileId);

CORE_TOOLS_NS_END