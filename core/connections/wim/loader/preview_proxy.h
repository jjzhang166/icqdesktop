#pragma once

#include "../../../namespaces.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

typedef std::unique_ptr<class preview_info> preview_info_uptr;

typedef boost::optional<int32_t> int32_t_opt;

class preview_info
{
public:
    preview_info(std::string &&_preview_uri);

    std::string get_preview_uri(
        const int32_t_opt _width = int32_t_opt(),
        const int32_t_opt _height = int32_t_opt()) const;

private:
    const std::string preview_uri_;
};

Str2StrMap format_params(
    const std::string &_uri_to_preview,
    const int32_t_opt _width = int32_t_opt(),
    const int32_t_opt _height = int32_t_opt(),
    const bool _crop = false);

const std::string& host();

preview_info_uptr parse_json(InOut char *_json);

CORE_WIM_PREVIEW_PROXY_NS_END