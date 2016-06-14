#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"

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
        struct get_chat_blocked_params 
        {
            std::string		aimid_;
        };

        class get_chat_blocked: public robusto_packet
        {
            get_chat_blocked_params		params_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t on_response_error_code() override;

        public:
            std::list<chat_member_info>					result_;

            get_chat_blocked(
                const wim_packet_params& _params, 
                const get_chat_blocked_params& _chat_params);

            virtual ~get_chat_blocked();
        };

    }

}