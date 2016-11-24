#ifndef __GET_HOSTS_CONFIG_H_
#define __GET_HOSTS_CONFIG_H_

#pragma once

#include "../wim_packet.h"
#include "../../../configuration/hosts_config.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    class hosts_map;
}

namespace core
{
    namespace wim
    {
        class get_hosts_config : public wim_packet
        {
            const std::string config_url_;

            hosts_map hosts_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        public:

            get_hosts_config(const wim_packet_params& params, const std::string& _config_url);
            virtual ~get_hosts_config();

            const hosts_map& get_hosts() const;
        };
    }
}


#endif //__GET_HOSTS_CONFIG_H_