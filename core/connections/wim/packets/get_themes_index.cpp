#include "stdafx.h"
#include "get_themes_index.h"
#include "../../../tools/strings.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../tools/binary_stream.h"
#include "../../../core.h"
#include "../../../http_request.h"

using namespace core;
using namespace wim;

get_themes_index::get_themes_index(const wim_packet_params& _params)
:	wim_packet(_params)
{
}


get_themes_index::~get_themes_index()
{
}

int32_t get_themes_index::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;
    
//    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
//    std::stringstream ss_ts;
//    ss_ts << (int64_t) ts;
    
    const std::string host = "https://icq.com/wallpaperlist/windows";
    
//    params["a"] = params_.a_token_;
//    params["f"] = "json";
//    params["k"] = params_.dev_id_;
//    params["ts"] = ss_ts.str();
//    
//    if (!md5_.empty())
//    {
//        params["md5"] = md5_;
//    }
//    
//    params["client"] = "icq";
//    
//    const auto sha256 = escape_symbols(get_url_sign(host, params, params_, false));
//    params["sig_sha256"] = sha256;
    
    std::stringstream ss_url;
    ss_url << host;// << params_map_2_string(params);
    
    _request->set_url(ss_url.str());
    
    return 0;
}

int32_t get_themes_index::execute()
{
    return wim_packet::execute();
}

int32_t get_themes_index::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;
    
    response_ = _response;
    
    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_themes_index::get_response()
{
    return response_;
}
