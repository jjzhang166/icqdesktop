#include "stdafx.h"
#include "add_members.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

add_members::add_members(
	const wim_packet_params& _params,
	const std::string& _aimid,
	const std::string & _members_to_add)
	:	wim_packet(_params),
	aimid_(_aimid),
	members_to_add_(_members_to_add)
{
}

add_members::~add_members()
{
}

int32_t add_members::init_request(std::shared_ptr<core::http_request_simple> _request)
{
	std::stringstream ss_url;


	ss_url << c_wim_host << "mchat/AddChat" <<
		"?f=json" <<
		"&chat_id=" <<  aimid_ <<
		"&aimsid=" << get_params().aimsid_ <<
        "&r=" <<  core::tools::system::generate_guid() <<
		"&members=" << members_to_add_;

	_request->set_url(ss_url.str());
    _request->set_keep_alive();

	return 0;
}

int32_t add_members::parse_response_data(const rapidjson::Value& _data)
{
	return 0;
}
