#include "stdafx.h"

#include "../wim_packet.h"

#include "preview_proxy.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

preview_info::preview_info(std::string &&_preview_uri)
    : preview_uri_(_preview_uri)
{
    assert(!preview_uri_.empty());
}

std::string preview_info::get_preview_uri(
    const int32_t_opt _width,
    const int32_t_opt _height) const
{
    assert(!_width || (*_width >= 0));
    assert(!_height || (*_height >= 0));

    std::string result;

    const auto MAX_ARGS_LENGTH = 20u;
    result.reserve(preview_uri_.length() + MAX_ARGS_LENGTH);

    result += preview_uri_;

    if (_width && (*_width > 0))
    {
        static const std::string W_PARAM = "&w=";

        result += W_PARAM;
        result += std::to_string(*_width);
    }

    if (_height && (*_height > 0))
    {
        static const std::string H_PARAM = "&h=";

        result += H_PARAM;
        result += std::to_string(*_height);
    }

    return result;
}

Str2StrMap format_params(
    const std::string &_uri_to_preview,
    const int32_t_opt _width,
    const int32_t_opt _height,
    const bool _crop)
{
    assert(!_uri_to_preview.empty());
    assert(!_width || (*_width >= 0));
    assert(!_height || (*_height >= 0));

    Str2StrMap result;

    result.emplace("url", wim_packet::escape_symbols(_uri_to_preview));

    if (_width && (*_width > 0))
    {
        result.emplace("w", std::to_string(*_width));
    }

    if (_height && (*_height > 0))
    {
        result.emplace("h", std::to_string(*_height));
    }

    if (_crop)
    {
        assert(_width || _height);
        result.emplace("crop", "1");
    }

    return result;
}

const std::string& host()
{
    static const std::string API_PREFIX = "http://api.icq.net/preview/getPreview";

    return API_PREFIX;
}

preview_info_uptr parse_json(InOut char *_json)
{
    assert(_json);

    rapidjson::Document doc;
    if (doc.ParseInsitu(_json).HasParseError())
    {
        return nullptr;
    }

    auto iter_doc = doc.FindMember("doc");
    if ((iter_doc == doc.MemberEnd()) ||
        !iter_doc->value.IsObject())
    {
        return nullptr;
    }

    auto iter_images = iter_doc->value.FindMember("images");
    if ((iter_images == doc.MemberEnd()) ||
        !iter_images->value.IsArray() ||
        iter_images->value.Empty())
    {
        return nullptr;
    }

    const auto &node_image = iter_images->value[0];
    if (!node_image.IsObject() ||
        !node_image.HasMember("preview_url"))
    {
        return nullptr;
    }

    const auto &node_preview_url = node_image["preview_url"];
    if (!node_preview_url.IsString())
    {
        return nullptr;
    }

    return preview_info_uptr(
        new preview_info(
            node_preview_url.GetString()));
}

CORE_WIM_PREVIEW_PROXY_NS_END