#include "stdafx.h"
#include "mod_chat_member_alpha.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

mod_chat_member_alpha::mod_chat_member_alpha(
    const wim_packet_params& _params,
    const std::string& _aimid,
    const std::string& _contact,
    const std::string& _role)
    : robusto_packet(_params)
    , aimid_(_aimid)
    , contact_(_contact)
    , role_(_role)
{
}

mod_chat_member_alpha::~mod_chat_member_alpha()
{
}

int32_t mod_chat_member_alpha::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();
    
    doc.AddMember("method", "modChatMemberAlpha", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("memberSn", contact_, a);
    node_params.AddMember("role", role_, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t mod_chat_member_alpha::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
