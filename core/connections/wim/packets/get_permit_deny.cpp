#include "stdafx.h"
#include "get_permit_deny.h"
#include "../permit_info.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;

get_permit_deny::get_permit_deny(const wim_packet_params& _params)
    :	wim_packet(_params)
    , permit_info_(new permit_info)
{
}

get_permit_deny::~get_permit_deny()
{
}

int32_t get_permit_deny::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << c_wim_host << "preference/getPermitDeny" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_;

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t get_permit_deny::parse_response_data(const rapidjson::Value& _node_results)
{
    return permit_info_->parse_response_data(_node_results);
}

std::vector<std::string> get_permit_deny::get_ignore_list() const
{
    return permit_info_->get_ignore_list();
}
