#include "stdafx.h"

#include "file_sharing.h"

CORE_TOOLS_NS_BEGIN

using namespace boost::xpressive;

namespace
{
    static const auto new_id_regex = sregex::compile("^http(s?)://files.icq.net/get/(?P<id>\\w{33})$");

    static const auto NEW_ID_LENGTH = 33;
}

std::string format_file_sharing_preview_uri(const std::string &_id, const file_sharing_preview_size _size)
{
    assert(!_id.empty());
    assert(_size > file_sharing_preview_size::min);
    assert(_size < file_sharing_preview_size::max);

    if (_id.length() != NEW_ID_LENGTH)
    {
        return std::string();
    }

    std::string result;
    result.reserve(128);

    result.append("https://files.icq.com/preview/max/");

    switch(_size)
    {
        case file_sharing_preview_size::small:
            result.append("192/");
            break;

        case file_sharing_preview_size::normal:
            result.append("600/");
            break;

        default:
            assert(!"unknown preview size");
            return std::string();
    }

    result.append(_id);

    return result;
}

bool is_new_file_sharing_uri(const std::string &_uri)
{
    assert(!_uri.empty());

    smatch m;
    return regex_match(_uri, m, new_id_regex);
}

bool parse_new_file_sharing_uri(const std::string &_uri, Out std::string &_fileId)
{
    assert(!_uri.empty());

    smatch m;
    if (!regex_match(_uri, m, new_id_regex))
    {
        Out _fileId.clear();

        return false;
    }

    Out _fileId = m["id"].str();
    assert(_fileId.length() == NEW_ID_LENGTH);

    return true;
}

CORE_TOOLS_NS_END