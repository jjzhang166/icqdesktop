#include "stdafx.h"
#include "end_session.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"

using namespace core;
using namespace wim;

end_session::end_session(const wim_packet_params& _params)
    :
    wim_packet(_params)
{
}

end_session::~end_session()
{
}

int32_t end_session::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;
    ss_url << c_wim_host << "aim/endSession" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_;
 
    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t end_session::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t end_session::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->get())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();

        return wpie_http_error;
    }

    return 0;
}
