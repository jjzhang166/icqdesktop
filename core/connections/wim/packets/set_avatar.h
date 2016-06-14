#ifndef __WIM_SET_AVATAR_H_
#define __WIM_SET_AVATAR_H_

#pragma once

#include "../wim_packet.h"

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
        class set_avatar : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            std::string aimid_;
            tools::binary_stream image_;

        public:
            set_avatar(const wim_packet_params& _params, tools::binary_stream _image, const std::string& _aimId);
            virtual ~set_avatar();
        };
    }
}

#endif// __WIM_SET_AVATAR_H_
