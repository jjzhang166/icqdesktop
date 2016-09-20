#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "snap_viewed.h"

CORE_WIM_NS_BEGIN

snap_viewed::snap_viewed(
    const wim_packet_params& _params,
    const uint64_t _snap_id,
    const bool _mark_prev_snaps_read,
    const std::string &_contact_aimid)
    : robusto_packet(_params)
    , snap_id_(_snap_id)
    , mark_prev_snaps_read_(_mark_prev_snaps_read)
    , contact_aimid_(_contact_aimid)
{
    assert(_snap_id > 0);
    assert(!contact_aimid_.empty());
}

snap_viewed::~snap_viewed()
{
}

int32_t snap_viewed::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);

    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    if (mark_prev_snaps_read_)
    {
        doc.AddMember("method", "snapViewed", a);
    }
    else
    {
        doc.AddMember("method", "singleSnapViewed", a);
    }

    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);
    node_params.AddMember("snapId", snap_id_, a);

    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    return 0;
}

int32_t snap_viewed::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

CORE_WIM_NS_END