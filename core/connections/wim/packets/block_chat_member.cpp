#include "stdafx.h"
#include "block_chat_member.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

block_chat_member::block_chat_member(
    const wim_packet_params& _params,
    const std::string& _aimid,
    const std::string& _contact,
    bool _block)
    : robusto_packet(_params)
    , aimid_(_aimid)
    , contact_(_contact)
    , block_(_block)
{
}

block_chat_member::~block_chat_member()
{
}

int32_t block_chat_member::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    if (block_)
        doc.AddMember("method", "blockChatMembers", a);
    else
        doc.AddMember("method", "unblockChatMembers", a);

    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    
    rapidjson::Value node_member(rapidjson::Type::kObjectType);
    node_member.AddMember("sn", contact_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.PushBack(node_member, a);

    node_params.AddMember("members", node_members, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t block_chat_member::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
