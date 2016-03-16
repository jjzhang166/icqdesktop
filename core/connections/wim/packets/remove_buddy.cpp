#include "stdafx.h"
#include "remove_buddy.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

remove_buddy::remove_buddy(
	const wim_packet_params& _params,
	const std::string& _aimid)
	:	wim_packet(_params),
		aimid_(_aimid)
{
}

remove_buddy::~remove_buddy()
{
}

int32_t remove_buddy::init_request(std::shared_ptr<core::http_request_simple> _request)
{
	std::stringstream ss_url;

	ss_url << c_wim_host << "buddylist/removeBuddy" <<
		"?f=json" <<
		"&aimsid=" << get_params().aimsid_ <<
		"&buddy=" << escape_symbols(aimid_) <<
        "&r=" << core::tools::system::generate_guid() <<
		"&allGroups=1";

	_request->set_url(ss_url.str());
    _request->set_keep_alive();

	return 0;
}

int32_t remove_buddy::parse_response_data(const rapidjson::Value& _data)
{
	return 0;
}
