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
            if (status != "typing")
                return 0;
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
            for (auto i = it->value.MemberBegin(), ie = it->value.MemberEnd(); i != ie; ++i)
                if (i->value.IsString())
                    chattersAimIds_.push_back(i->value.GetString());
    }
    return 0;
}

void fetch_event_typing::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
	_im->on_event_typing(this, _on_complete);
}
