#include "stdafx.h"
#include "permit_info.h"
#include "wim_packet.h"

using namespace core;
using namespace wim;

permit_info::permit_info()
{
}

permit_info::~permit_info()
{

}

int32_t permit_info::parse_response_data(const rapidjson::Value& _node_results)
{
    auto ignores = _node_results.FindMember("ignores");
    if (ignores == _node_results.MemberEnd() || !ignores->value.IsArray())
        return wpie_http_parse_response;

    ignore_aimid_list_.reserve(ignores->value.Size());
    for (auto iter = ignores->value.Begin(); iter != ignores->value.End(); ++iter)
    {
        std::string aimid = iter->GetString();
        ignore_aimid_list_.emplace_back(aimid);
    }

    return 0;
}

std::vector<std::string> permit_info::get_ignore_list() const
{
    return ignore_aimid_list_;
}
