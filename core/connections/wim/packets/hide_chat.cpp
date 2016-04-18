#include "stdafx.h"
#include "hide_chat.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

hide_chat::hide_chat(const wim_packet_params& _params, const std::string& _aimid, int64_t _last_msg_id)
    :	
    wim_packet(_params),
    aimid_(_aimid),
    last_msg_id_(_last_msg_id)
{
}


hide_chat::~hide_chat()
{

}

int32_t hide_chat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string method;

    std::stringstream ss_url;
    ss_url << c_wim_host << "buddylist/hideChat" << method << "?f=json" << "&aimsid=" << get_params().aimsid_ << 
        "&buddy=" << escape_symbols(aimid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&lastMsgId=" << last_msg_id_;

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t hide_chat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t hide_chat::on_empty_data()
{
    return 0;
}
