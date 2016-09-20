#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

class snap_viewed : public robusto_packet
{
public:
    snap_viewed(
        const wim_packet_params& _params,
        const uint64_t _snap_id,
        const bool _mark_prev_snaps_read,
        const std::string &_contact_aimid);

    virtual ~snap_viewed();

private:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

    virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

    const uint64_t snap_id_;

    const bool mark_prev_snaps_read_;

    const std::string contact_aimid_;

};

CORE_WIM_NS_END