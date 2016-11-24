#include "stdafx.h"

#include "get_hosts_config.h"

#include "../../../http_request.h"
#include "../../../configuration/hosts_config.h"


using namespace core;
using namespace wim;

get_hosts_config::get_hosts_config(const wim_packet_params& params, const std::string& _config_url)
    :   wim_packet(params),
        config_url_(_config_url)
{
}


get_hosts_config::~get_hosts_config()
{
}

int32_t get_hosts_config::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(config_url_);

    return 0;
}

int32_t get_hosts_config::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!hosts_.parse(*_response))
        return wpie_http_empty_response;

    return 0;
}


const hosts_map& get_hosts_config::get_hosts() const
{
    return hosts_;
}
