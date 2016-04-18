#include "stdafx.h"
#include "set_buddy_attribute.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

set_buddy_attribute::set_buddy_attribute(
    const wim_packet_params& _params,
    const std::string& _aimid,
    const std::string& _friendly)
    :   wim_packet(_params),
        aimid_(_aimid),
        friendly_(_friendly)
{
}

set_buddy_attribute::~set_buddy_attribute()
{
}

int32_t set_buddy_attribute::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << c_wim_host << "buddylist/setBuddyAttribute" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_ <<
        "&buddy=" << escape_symbols(aimid_) <<
        "&friendly=" << escape_symbols(friendly_) << 
        "&r=" << core::tools::system::generate_guid();

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t set_buddy_attribute::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
