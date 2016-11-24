#include "stdafx.h"
#include "join_chat_alpha.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


join_chat_alpha::join_chat_alpha(const wim_packet_params& _params, const std::string& _stamp, const int _age)
    : robusto_packet(_params), stamp_(_stamp), age_(_age)
{
}


join_chat_alpha::~join_chat_alpha()
{

}

int32_t join_chat_alpha::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "joinChatAlpha", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stamp", stamp_, a);
    if (age_ > 0)
        node_params.AddMember("age", age_, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t join_chat_alpha::parse_results(const rapidjson::Value& _node_results)
{
    return 0;
}

int32_t join_chat_alpha::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}
