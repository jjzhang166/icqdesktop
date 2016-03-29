#include "stdafx.h"
#include "get_flags.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

get_flags::get_flags(
	const wim_packet_params& _params)
	:	wim_packet(_params)
{
}

get_flags::~get_flags()
{
}

int32_t get_flags::init_request(std::shared_ptr<core::http_request_simple> _request)
{
	std::stringstream ss_url;

	ss_url << c_wim_host << "db/getFlags" <<
		"?f=json" <<
		"&aimsid=" << get_params().aimsid_ <<
        "&r=" <<  core::tools::system::generate_guid();

	_request->set_url(ss_url.str());
    _request->set_keep_alive();

	return 0;
}

int32_t get_flags::parse_response_data(const rapidjson::Value& _data)
{
	int32_t err = wpie_http_parse_response;

	try
	{
		auto iter_flags = _data.FindMember("op_flag");
		if (iter_flags == _data.MemberEnd() || !iter_flags->value.IsInt())
			return wpie_http_parse_response;

        flags_ = iter_flags->value.GetUint();
		err = 0;
	}
	catch (const std::exception&)
	{
		
	}
	
	return err;
}

int get_flags::flags() const
{
    return flags_;
}
