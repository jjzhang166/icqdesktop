#include "stdafx.h"
#include "create_chat.h"
#include "../chat_params.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

create_chat::create_chat(
    const wim_packet_params& _params,
    const std::string& _aimid,
    const std::string& _chatName,
    const std::vector<std::string> &_chatMembers)
    : robusto_packet(_params)
    , aimid_(_aimid)
    , chat_members_(_chatMembers)
    , chat_params_(new chat_params())
{
    chat_params_->set_name(_chatName);
}

create_chat::~create_chat()
{
}

chat_params *create_chat::get_chat_params()
{
    return chat_params_.get();
}

void create_chat::set_chat_params(chat_params *&_chat_params)
{
    chat_params_.reset(_chat_params);
}

int32_t create_chat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "createChat", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);
    
    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    {
        node_params.AddMember("name", chat_params_->get_name().get(), a);
        node_params.AddMember("aimSid", get_params().aimsid_, a);
        if (chat_params_->get_avatar().is_initialized())
            node_params.AddMember("avatarId", chat_params_->get_avatar().get(), a);
        if (!chat_members_.empty())
        {
            rapidjson::Value members_array(rapidjson::Type::kArrayType);
            {
                for (const auto &sn: chat_members_)
                {
                    rapidjson::Value member_params(rapidjson::Type::kObjectType);
                    member_params.AddMember("sn", sn, a);
                    members_array.PushBack(member_params, a);
                }
            }
            node_params.AddMember("members", members_array, a);
        }
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
    }
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t create_chat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
