#include "stdafx.h"
#include "attach_phone.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../login_info.h"

using namespace core;
using namespace wim;

#define WIM_API_ATTACH_PHONE_HOST "https://www.icq.com/smsreg/attachPhoneNumber.php"

attach_phone::attach_phone(
    const wim_packet_params& _params, const phone_info& _phone_info)

    : wim_packet(_params)
    , phone_info_(_phone_info)
{
}

attach_phone::~attach_phone()
{
}

int32_t attach_phone::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (phone_info_.get_phone().empty())// || password_.empty())
        return wpie_invalid_login;

    const std::string host = WIM_API_ATTACH_PHONE_HOST;
    _request->set_url(host);
    _request->push_post_parameter("a", escape_symbols(params_.a_token_));
    _request->push_post_parameter("k", escape_symbols(params_.dev_id_));
    _request->push_post_parameter("f", "json");
    _request->push_post_parameter("msisdn", phone_info_.get_phone());
    _request->push_post_parameter("sms_code", phone_info_.get_sms_code());
    _request->push_post_parameter("trans_id", phone_info_.get_trans_id());
    _request->push_post_parameter("r", core::tools::system::generate_guid());

    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    _request->push_post_parameter("ts", tools::from_int64(ts));

    std::map<std::string, std::string> post_params;
    _request->get_post_parameters(post_params);

    _request->push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, post_params, params_, true)));

    return 0;
}

int32_t attach_phone::on_response_error_code()
{
    switch (status_code_)
    {
    case 471:
        return wpie_error_attach_busy_phone;
    case 401:
        return wpie_wrong_login;
    default:
        return wpie_login_unknown_error;
    }
}

int32_t attach_phone::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t attach_phone::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t attach_phone::on_empty_data()
{
    return 0;
}
