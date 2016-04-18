#include "stdafx.h"
#include "gen_robusto_token.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


gen_robusto_token::gen_robusto_token(const wim_packet_params& _params)
    :	robusto_packet(_params)
{
}


gen_robusto_token::~gen_robusto_token()
{

}

int32_t gen_robusto_token::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    const std::string host = c_robusto_host + "genToken";
    _request->set_url(host);
    _request->set_keep_alive();
    _request->push_post_parameter("a", escape_symbols(params_.a_token_));
    _request->push_post_parameter("k", escape_symbols(params_.dev_id_));

    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    _request->push_post_parameter("ts", tools::from_int64(ts));

    std::map<std::string, std::string> post_params;
    _request->get_post_parameters(post_params);

    _request->push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, post_params, params_, true)));
    return 0;
}

int32_t gen_robusto_token::parse_results(const rapidjson::Value& _node_results)
{
    auto iter_token = _node_results.FindMember("authToken");
    if (iter_token == _node_results.MemberEnd())
        return wpie_http_parse_response;

    token_ = iter_token->value.GetString();

    return 0;
}

int32_t gen_robusto_token::on_response_error_code()
{
    switch (status_code_)
    {
    case 40001: //- bad akes (app key)
        return wpie_error_robusto_bad_app_key;
    case 40002: //- icq auth expired (mb password changed)
        return wpie_error_robusto_icq_auth_expired;
    case 40003: //- bad sigsha (signature mismatch)
        return wpie_error_robusto_bad_sigsha;
    case 40004: //- bad timestamp in sigsha
        return wpie_error_robusto_bad_ts;
    }

    return robusto_packet::on_response_error_code();
}

