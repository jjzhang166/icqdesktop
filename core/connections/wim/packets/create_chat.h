#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class chat_params;
        
        class create_chat : public robusto_packet
        {
        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	aimid_;
            std::vector<std::string> chat_members_;
            std::unique_ptr<chat_params> chat_params_;

        public:
            create_chat(const wim_packet_params& _params, const std::string& _aimId, const std::string& _chatName, const std::vector<std::string> &_chatMembers);
            virtual ~create_chat();

            chat_params *get_chat_params();
            void set_chat_params(chat_params *&_chat_params);
            
            size_t members_count() const { return chat_members_.size(); }
        };

    }

}
