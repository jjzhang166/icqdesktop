#include "stdafx.h"

#include "fetch_event_presence.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_contactlist_cache.h"

using namespace core;
using namespace wim;

fetch_event_presence::fetch_event_presence()
    :	presense_(new cl_presence())
{
}


fetch_event_presence::~fetch_event_presence()
{
}

int32_t fetch_event_presence::parse(const rapidjson::Value& _node_event_data)
{
    try
    {
        auto iter_aimid = _node_event_data.FindMember("aimId");
        if  (iter_aimid == _node_event_data.MemberEnd() || !iter_aimid->value.IsString())
            return wpie_error_parse_response;

        aimid_ = iter_aimid->value.GetString();

        auto iter_state = _node_event_data.FindMember("state");
        if  (iter_state != _node_event_data.MemberEnd() && iter_state->value.IsString())
            presense_->state_ = iter_state->value.GetString();

        auto iter_friendly = _node_event_data.FindMember("friendly");
        if  (iter_friendly != _node_event_data.MemberEnd() && iter_friendly->value.IsString())
            presense_->friendly_ = iter_friendly->value.GetString();

        auto iter_usertype = _node_event_data.FindMember("userType");
        if (iter_usertype != _node_event_data.MemberEnd() && iter_usertype->value.IsString())
            presense_->usertype_ = iter_usertype->value.GetString();

        auto iter_status_msg = _node_event_data.FindMember("statusMsg");
        if (iter_status_msg != _node_event_data.MemberEnd() && iter_status_msg->value.IsString())
            presense_->status_msg_ = iter_status_msg->value.GetString();

        auto iter_otherNumber = _node_event_data.FindMember("otherNumber");
        if (iter_otherNumber != _node_event_data.MemberEnd() && iter_otherNumber->value.IsString())
            presense_->other_number_ = iter_otherNumber->value.GetString();

        auto iter_lastseen = _node_event_data.FindMember("lastseen");
        if (iter_lastseen != _node_event_data.MemberEnd() && iter_lastseen->value.IsUint())
        {
            presense_->lastseen_ = iter_lastseen->value.GetUint();
            if (presense_->lastseen_ != 0 && presense_->state_ != "offline" && presense_->state_ != "mobile")
                presense_->state_ = "offline";
        }
        if (presense_->state_ == "occupied" || presense_->state_ == "na" || presense_->state_ == "busy")
            presense_->state_ = "dnd";
        else if (presense_->state_ == "away")
            presense_->state_ = "away";
        else if (presense_->state_ != "offline" && presense_->state_ != "invisible" && presense_->state_ != "mobile")
            presense_->state_ = "online";

        if (presense_->state_ == "mobile" && presense_->lastseen_ == 0)
            presense_->state_ = "online";

        auto iter_capabilities = _node_event_data.FindMember("capabilities");
        if (iter_capabilities != _node_event_data.MemberEnd() && iter_capabilities->value.IsArray())
        {
            for (auto iter = iter_capabilities->value.Begin(); iter != iter_capabilities->value.End(); ++iter)
                presense_->capabilities_.insert(iter->GetString());
        }

        auto iter_official = _node_event_data.FindMember("official");
        if (iter_official != _node_event_data.MemberEnd())
        {
            if (iter_official->value.IsUint())
                presense_->official_ = iter_official->value.GetUint() == 1;
        }

        auto iter_mute = _node_event_data.FindMember("mute");
        if (iter_mute != _node_event_data.MemberEnd())
        {
            if (iter_mute->value.IsUint())
                presense_->muted_ = iter_mute->value != 0;
            else if (iter_mute->value.IsBool())
                presense_->muted_ = iter_mute->value.GetBool();
        }

        auto iter_livechat = _node_event_data.FindMember("livechat");
        if (iter_livechat != _node_event_data.MemberEnd())
            presense_->is_live_chat_ = iter_livechat->value != 0;

        auto iter_iconId = _node_event_data.FindMember("iconId");
        if (iter_iconId != _node_event_data.MemberEnd() && iter_iconId->value.IsString())
            presense_->icon_id_ = iter_iconId->value.GetString();

        auto iter_bigIconId = _node_event_data.FindMember("bigIconId");
        if (iter_bigIconId != _node_event_data.MemberEnd() && iter_bigIconId->value.IsString())
            presense_->big_icon_id_ = iter_bigIconId->value.GetString();

        auto iter_largeIconId = _node_event_data.FindMember("largeIconId");
        if (iter_largeIconId != _node_event_data.MemberEnd() && iter_largeIconId->value.IsString())
            presense_->large_icon_id_ = iter_largeIconId->value.GetString();

    }
    catch (const std::exception&)
    {
        return wpie_error_parse_response;
    }

    return 0;
}

void fetch_event_presence::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_presence(this, _on_complete);
}
