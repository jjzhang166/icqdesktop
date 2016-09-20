#include "stdafx.h"

#include "fetch_event_imstate.h"
#include "../wim_im.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

fetch_event_imstate::fetch_event_imstate()
{
}


fetch_event_imstate::~fetch_event_imstate()
{
}

const imstate_list& fetch_event_imstate::get_states() const
{
    return states_;
}

int32_t fetch_event_imstate::parse(const rapidjson::Value& _node_event_data)
{
    auto iter_imstates = _node_event_data.FindMember("imStates");
    if  (iter_imstates == _node_event_data.MemberEnd() || !iter_imstates->value.IsArray())
        return wpie_error_parse_response;

    for (auto iter_state = iter_imstates->value.Begin(); iter_state != iter_imstates->value.End(); ++iter_state)
    {
        imstate ustate;

        auto iter_request_id = iter_state->FindMember("sendReqId");
        if (iter_request_id == iter_state->MemberEnd() || !iter_request_id->value.IsString())
            continue;

        ustate.set_request_id(iter_request_id->value.GetString());

        auto iter_msg_id = iter_state->FindMember("msgId");
        if (iter_msg_id == iter_state->MemberEnd() || !iter_msg_id->value.IsString())
            continue;

        ustate.set_msg_id(iter_msg_id->value.GetString());

        auto iter_state_state = iter_state->FindMember("state");
        if (iter_state_state == iter_state->MemberEnd() || !iter_state_state->value.IsString())
            continue;

        std::string state = iter_state_state->value.GetString();

        if (state == "failed")
            ustate.set_state(imstate_sent_state::failed);
        else if (state == "sent")
            ustate.set_state(imstate_sent_state::sent);
        else if (state == "delivered")
            ustate.set_state(imstate_sent_state::delivered);

        auto iter_hist_msg_id = iter_state->FindMember("histMsgId");
        if (iter_hist_msg_id != iter_state->MemberEnd() && iter_hist_msg_id->value.IsInt64())
        {
            ustate.set_hist_msg_id(iter_hist_msg_id->value.GetInt64());
        }

        auto iter_before_hist_msg_id = iter_state->FindMember("beforeHistMsgId");
        if (iter_before_hist_msg_id != iter_state->MemberEnd() && iter_before_hist_msg_id->value.IsInt64())
        {
            ustate.set_before_hist_msg_id(iter_before_hist_msg_id->value.GetInt64());
        }

        auto iter_error_code = iter_state->FindMember("errorCode");
        if (iter_error_code != iter_state->MemberEnd() && iter_error_code->value.IsInt())
        {
            ustate.set_error_code(iter_error_code->value.GetInt());
        }

        states_.push_back(ustate);
    }

    return 0;
}

void fetch_event_imstate::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{


    _im->on_event_imstate(this, _on_complete);
}
