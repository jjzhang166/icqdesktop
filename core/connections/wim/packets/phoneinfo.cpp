#include "stdafx.h"
#include "phoneinfo.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

phoneinfo::phoneinfo(const wim_packet_params &params, const std::string &phone, const std::string &gui_locale):
    wim_packet(params), phone_(phone), gui_locale_(gui_locale)
{
}

phoneinfo::~phoneinfo()
{
}

int32_t phoneinfo::init_request(std::shared_ptr<core::http_request_simple> request)
{
    /*
     
    https://clientapi.mail.ru/fcgi-bin/smsphoneinfo?service=icq_registration&phone=84951234567&info=typing_check,ivr,timezone&lang=ru&id=unique_id_per_instance_or_session
    
    */
    
    std::stringstream get_request;
    get_request << "https://clientapi.mail.ru/fcgi-bin/smsphoneinfo" <<
        "?service=icq_registration" <<
        "&info=typing_check,score" <<
        "&lang=" << (gui_locale_.length() == 2 ? gui_locale_ : "en") <<
        "&phone=" << escape_symbols(phone_) <<
        "&id=" << core::tools::system::generate_guid();

    request->set_url(get_request.str());
    request->set_keep_alive();

    return 0;
}

int32_t phoneinfo::parse_response_data(const rapidjson::Value &data)
{
    return 0;
}
