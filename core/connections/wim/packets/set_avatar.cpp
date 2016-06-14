#include "stdafx.h"
#include "set_avatar.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

set_avatar::set_avatar(const wim_packet_params& _params, tools::binary_stream _image, const std::string& _aimid)
    : wim_packet(_params)
    , image_(_image)
    , aimid_(_aimid)
{
}

set_avatar::~set_avatar()
{
}

int32_t set_avatar::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << c_wim_host << "expressions/upload" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_ <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&type=largeBuddyIcon";

    if (!aimid_.empty())
        ss_url << "&t=" << aimid_;
    
    auto size = image_.available();
    _request->set_post_data(image_.read_available(), size, true);
    image_.reset_out();
    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t set_avatar::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t set_avatar::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
