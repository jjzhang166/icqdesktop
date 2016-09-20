#include "stdafx.h"
#include "mod_chat_alpha.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

mod_chat_alpha::mod_chat_alpha(
    const wim_packet_params& _params,
    const std::string& _aimid)
    : robusto_packet(_params)
    , aimid_(_aimid)
{
}

mod_chat_alpha::~mod_chat_alpha()
{
}

void mod_chat_alpha::set_about(const std::string& _about)
{
    about_.reset(_about);
}

void mod_chat_alpha::set_name(const std::string& _name)
{
    name_ = _name;
}

void mod_chat_alpha::set_public(bool _public)
{
    public_.reset(_public);
}

void mod_chat_alpha::set_join(bool _approved)
{
    approved_.reset(_approved);
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
    if (!name_.empty())
        node_params.AddMember("name", name_, a);
    else if (about_.is_initialized())
        node_params.AddMember("about", about_.get(), a);
    else if (public_.is_initialized())
        node_params.AddMember("public", public_.get(), a);
    else if (approved_.is_initialized())
        node_params.AddMember("joinModeration", approved_.get(), a);
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
