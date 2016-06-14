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
        class mod_chat_alpha : public robusto_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	about_;
            std::string name_;
            std::string	aimid_;
            boost::optional<bool> public_;

        public:

            mod_chat_alpha(const wim_packet_params& _params, const std::string& _aimId);

            void set_about(const std::string& _about);
            void set_name(const std::string& _name);
            void set_public(bool _public);

            virtual ~mod_chat_alpha();
        };

    }

}
