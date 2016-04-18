#include "stdafx.h"

#include "fetch_event_typing.h"
#include "../wim_im.h"

using namespace core;
using namespace wim;

fetch_event_typing::fetch_event_typing()
{
}

fetch_event_typing::~fetch_event_typing()
{
}

int32_t fetch_event_typing::parse(const rapidjson::Value& _node_event_data)
{
    const auto it_status = _node_event_data.FindMember("typingStatus");

    if (it_status != _node_event_data.MemberEnd() && it_status->value.IsString())
    {
        std::string status = it_status->value.GetString();
        is_typing_ = (status == "typing");
    }

    const auto it_aimid = _node_event_data.FindMember("aimId");

    if (it_aimid != _node_event_data.MemberEnd() && it_aimid->value.IsString())
    {
        aimid_ = it_aimid->value.GetString();
    }
    else
    {
        return -1;
    }

    const auto it_attr = _node_event_data.FindMember("MChat_Attrs");

    if (it_attr != _node_event_data.MemberEnd() && it_attr->value.IsObject())
    {
        const auto itsender = it_attr->value.FindMember("sender");

        if (itsender != it_attr->value.MemberEnd() && itsender->value.IsString())
        {
            chatter_aimid_ = itsender->value.GetString();
        }

        const auto itoptions = it_attr->value.FindMember("options");

        if (itoptions != it_attr->value.MemberEnd() && itoptions->value.IsObject())
        {
            const auto itname = itoptions->value.FindMember("senderName");

            if (itname != itoptions->value.MemberEnd() && itname->value.IsString())
            {
                chatter_name_ = itname->value.GetString();
            }
        }
    }

    return 0;
}

void fetch_event_typing::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_typing(this, _on_complete);
}
