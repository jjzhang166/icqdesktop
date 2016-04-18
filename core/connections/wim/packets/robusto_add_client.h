#ifndef __ROBUSTO_ADD_CLIENT_H_
#define __ROBUSTO_ADD_CLIENT_H_

#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class robusto_add_client : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

            uint32_t	client_id_;

        public:

            robusto_add_client(const wim_packet_params& _params);
            virtual ~robusto_add_client();

            uint32_t get_client_id() const { return client_id_; }
        };

    }

}


#endif// __ROBUSTO_ADD_CLIENT_H_