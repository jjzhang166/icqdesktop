#include "stdafx.h"

#include "fetch_event_diff.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_contactlist_cache.h"

using namespace core;
using namespace wim;

fetch_event_diff::fetch_event_diff()
    :   diff_(new diffs_map())
{
}


fetch_event_diff::~fetch_event_diff()
{
}

int32_t fetch_event_diff::parse(const rapidjson::Value& _node_event_data)
{
	try
	{
		if (!_node_event_data.IsArray())
			return wpie_error_parse_response;

		for (auto iter = _node_event_data.Begin(); iter != _node_event_data.End(); ++iter)
		{
			std::string type;
			auto iter_type = iter->FindMember("type");
			if (iter_type != iter->MemberEnd() && iter_type->value.IsString())
				type = iter_type->value.GetString();

			auto iter_data = iter->FindMember("data");
			if (iter_data != iter->MemberEnd() && iter_data->value.IsArray())
			{
				auto cl = std::make_shared<contactlist>();
				cl->unserialize_from_diff(iter_data->value);
				diff_->insert(std::make_pair(type, cl));
			}
		}
	}
	catch (const std::exception&)
	{
		return wpie_error_parse_response;
	}
	
	return 0;
}

void fetch_event_diff::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_diff(this, _on_complete);
}
