#include "stdafx.h"
#include "robusto_add_client.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


robusto_add_client::robusto_add_client(const wim_packet_params& _params)
	:	robusto_packet(_params),
		client_id_(0)
{
}


robusto_add_client::~robusto_add_client()
{

}

int32_t robusto_add_client::init_request(std::shared_ptr<core::http_request_simple> _request)
{
	_request->set_url(c_robusto_host);
    _request->set_keep_alive();

	rapidjson::Document doc(rapidjson::Type::kObjectType);
    
	auto& a = doc.GetAllocator();
	
	doc.AddMember("method", "addClient", a);
	doc.AddMember("reqId", get_req_id(), a);
	doc.AddMember("authToken", robusto_params_.robusto_token_, a);

	rapidjson::Value node_params(rapidjson::Type::kObjectType);
	doc.AddMember("params", node_params, a);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	_request->push_post_parameter(buffer.GetString(), "");
		
	return 0;
}

int32_t robusto_add_client::parse_results(const rapidjson::Value& _node_results)
{
	auto iter_client_id = _node_results.FindMember("clientId");
	if (iter_client_id == _node_results.MemberEnd())
		return wpie_http_parse_response;

	client_id_ = iter_client_id->value.GetUint();

	return 0;
}

int32_t robusto_add_client::on_response_error_code()
{
	return robusto_packet::on_response_error_code();
}

