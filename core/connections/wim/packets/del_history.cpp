#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "del_history.h"

CORE_WIM_NS_BEGIN

del_history::del_history(
    const wim_packet_params& _params,
    const int64_t _up_to_id,
    const std::string &_contact_aimid
)
    : robusto_packet(_params)
    , up_to_id_(_up_to_id)
    , contact_aimid_(_contact_aimid)
{
    assert(up_to_id_ > 0);
    assert(!contact_aimid_.empty());
}

int32_t del_history::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);

    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "delHistory", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);
    node_params.AddMember("uptoMsgId", up_to_id_, a);

    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    __INFO(
        "delete_history",
        "sending history deletion request\n"
        "    contact_aimid=<%1%>\n"
        "    up_to=<%2%>",
        contact_aimid_ % up_to_id_);

    return 0;
}

int32_t del_history::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t del_history::on_response_error_code()
{
    __INFO(
        "delete_history",
        "    status_code=<%1%>",
        status_code_
    );

    return robusto_packet::on_response_error_code();
}



CORE_WIM_NS_END