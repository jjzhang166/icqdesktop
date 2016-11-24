#include "stdafx.h"
#include "mod_chat_alpha.h"
#include "../chat_params.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

mod_chat_alpha::mod_chat_alpha(
    const wim_packet_params& _params,
    const std::string& _aimid)
    : robusto_packet(_params)
    , aimid_(_aimid)
    , chat_params_(new chat_params())
{
}

mod_chat_alpha::~mod_chat_alpha()
{
}

chat_params *mod_chat_alpha::get_chat_params()
{
    return chat_params_.get();
}

void mod_chat_alpha::set_chat_params(chat_params *&_chat_params)
{
    chat_params_.reset(_chat_params);
}

int32_t mod_chat_alpha::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "modChatAlpha", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    if (chat_params_->get_name().is_initialized())
        node_params.AddMember("name", chat_params_->get_name().get(), a);
    if (chat_params_->get_about().is_initialized())
        node_params.AddMember("about", chat_params_->get_about().get(), a);
    if (chat_params_->get_public().is_initialized())
        node_params.AddMember("public", chat_params_->get_public().get(), a);
    if (chat_params_->get_join().is_initialized())
        node_params.AddMember("joinModeration", chat_params_->get_join().get(), a);
    if (chat_params_->get_joiningByLink().is_initialized())
        node_params.AddMember("live", chat_params_->get_joiningByLink().get(), a);
    if (chat_params_->get_readOnly().is_initialized())
        node_params.AddMember("defaultRole", std::string(chat_params_->get_readOnly().get() ? "readonly" : "member"), a);
    if (chat_params_->get_ageGate().is_initialized())
        node_params.AddMember("ageRestriction", chat_params_->get_ageGate().get(), a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t mod_chat_alpha::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
