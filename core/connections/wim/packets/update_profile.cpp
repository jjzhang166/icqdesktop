#include "stdafx.h"
#include "update_profile.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

update_profile::update_profile(
    const wim_packet_params& _params,
    const std::vector<std::pair<std::string, std::string>>& _fields)
    :	wim_packet(_params),
    fields_(_fields)
{
}

update_profile::~update_profile()
{
}

int32_t update_profile::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(!fields_.empty());
    if (fields_.empty())
        return 0;

    std::stringstream ss_url;

    ss_url << c_wim_host << "memberDir/update" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_ <<
        "&r=" <<  core::tools::system::generate_guid();

    std::for_each(fields_.begin(), fields_.end(), [&ss_url](const std::pair<std::string, std::string> item){ ss_url << "&set=" << item.first << "=" << escape_symbols(item.second); });
    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t update_profile::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
