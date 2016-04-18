#include "stdafx.h"

#include "fetch_event_dlg_state.h"

#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../archive/archive_index.h"
#include "../../../log/log.h"

using namespace core;
using namespace wim;

fetch_event_dlg_state::fetch_event_dlg_state()
    : messages_(new archive::history_block())
{
}


fetch_event_dlg_state::~fetch_event_dlg_state()
{
}

int32_t fetch_event_dlg_state::parse(const rapidjson::Value& _node_event_data)
{
    auto iter_sn = _node_event_data.FindMember("sn");
    auto iter_last_msg_id = _node_event_data.FindMember("lastMsgId");

    if (
        iter_sn == _node_event_data.MemberEnd() ||
        iter_last_msg_id == _node_event_data.MemberEnd() ||
        !iter_sn->value.IsString() ||
        !iter_last_msg_id->value.IsInt64())
    {
        __TRACE(
            "delivery",
            "%1%",
            "failed to parse incoming dlg state event");

        return wpie_error_parse_response;
    }

    aimid_ = iter_sn->value.GetString();
    state_.set_last_msgid(iter_last_msg_id->value.GetInt64());

    auto iter_unread_count = _node_event_data.FindMember("unreadCnt");
    if (iter_unread_count != _node_event_data.MemberEnd() && iter_unread_count->value.IsUint())
        state_.set_unread_count(iter_unread_count->value.GetUint());

    auto iter_yours = _node_event_data.FindMember("yours");
    if (iter_yours != _node_event_data.MemberEnd() && iter_yours->value.IsObject())
    {
        auto iter_last_read = iter_yours->value.FindMember("lastRead");
        if (iter_last_read != iter_yours->value.MemberEnd() && iter_last_read->value.IsInt64())
            state_.set_yours_last_read(iter_last_read->value.GetInt64());
    }

    auto iter_theirs = _node_event_data.FindMember("theirs");
    if (iter_theirs != _node_event_data.MemberEnd() && iter_theirs->value.IsObject())
    {
        auto iter_last_read = iter_theirs->value.FindMember("lastRead");
        if (iter_last_read != iter_theirs->value.MemberEnd() && iter_last_read->value.IsInt64())
            state_.set_theirs_last_read(iter_last_read->value.GetInt64());

        auto iter_last_delivered = iter_theirs->value.FindMember("lastDelivered");
        if (iter_last_delivered != iter_theirs->value.MemberEnd() && iter_last_delivered->value.IsInt64())
            state_.set_theirs_last_delivered(iter_last_delivered->value.GetInt64());
    }

    int64_t older_msg_id = -1;
    auto iter_older_msgid = _node_event_data.FindMember("olderMsgId");
    if (iter_older_msgid != _node_event_data.MemberEnd() && iter_older_msgid->value.IsInt64())
    {
        older_msg_id = iter_older_msgid->value.GetInt64();
    }

    auto iter_patch_version = _node_event_data.FindMember("patchVersion");
    if (iter_patch_version != _node_event_data.MemberEnd())
    {
        const std::string patch_version = iter_patch_version->value.GetString();
        assert(!patch_version.empty());

        state_.set_dlg_state_patch_version(patch_version);
    }

    auto iter_del_up_to = _node_event_data.FindMember("delUpto");
    if (
        (iter_del_up_to != _node_event_data.MemberEnd()) &&
        iter_del_up_to->value.IsInt64()
        )
    {
        state_.set_del_up_to(iter_del_up_to->value.GetInt64());
    }

    core::archive::persons_map persons;
    if (!parse_history_messages_json(_node_event_data, older_msg_id, aimid_, *messages_, persons))
    {
        __TRACE(
            "delivery",
            "%1%",
            "failed to parse incoming dlg state messages");

        return wpie_error_parse_response;
    }

    auto iter_person = persons.find(aimid_);
    if (iter_person != persons.end())
    {
        state_.set_friendly(iter_person->second.friendly_);
        state_.set_official(iter_person->second.official_);
    }

    if (::build::is_debug())
    {
        __INFO(
            "delete_history",
            "incoming dlg state event\n"
            "    contact=<%1%>\n"
            "    dlg-patch=<%2%>\n"
            "    del-up-to=<%3%>",
            aimid_ % state_.get_dlg_state_patch_version() % state_.get_del_up_to()
            );

        __TRACE(
            "delivery",
            "parsed incoming dlg state event\n"
            "	size=<%1%>\n"
            "	last_msgid=<%2%>",
            messages_->size() %
            state_.get_last_msgid());

        for (const auto &message : *messages_)
        {
            __TRACE(
                "delivery",
                "parsed incoming dlg state message\n"
                "	id=<%1%>",
                message->get_msgid());
        }
    }

    return 0;
}

void fetch_event_dlg_state::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_dlg_state(this, _on_complete);
}
