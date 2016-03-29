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
    {
        const auto it = _node_event_data.FindMember("typingStatus");
        if (it != _node_event_data.MemberEnd() && it->value.IsString())
        {
            std::string status = it->value.GetString();
            isTyping_ = (status == "typing");
        }
    }
    {
        const auto it = _node_event_data.FindMember("aimId");
        if (it != _node_event_data.MemberEnd() && it->value.IsString())
            aimId_ = it->value.GetString();
        else
            return -1;
    }
    {
        const auto it = _node_event_data.FindMember("MChat_Attrs");
        if (it != _node_event_data.MemberEnd() && it->value.IsObject())
        {
            const auto itsender = it->value.FindMember("sender");
            if (itsender != it->value.MemberEnd() && itsender->value.IsString())
                chatterAimId_ = itsender->value.GetString();
            
            const auto itoptions = it->value.FindMember("options");
            if (itoptions != it->value.MemberEnd() && itoptions->value.IsObject())
            {
                const auto itname = itoptions->value.FindMember("senderName");
                if (itname != itoptions->value.MemberEnd() && itname->value.IsString())
                    chatterName_ = itname->value.GetString();
            }
        }
    }
    return 0;
}

void fetch_event_typing::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
	_im->on_event_typing(this, _on_complete);
}
