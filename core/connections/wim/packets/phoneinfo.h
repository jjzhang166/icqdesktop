#ifndef __WIM_PHONEINFO_H_
#define __WIM_PHONEINFO_H_

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
        class phoneinfo : public wim_packet
        {
        private:
            std::string phone_;
            std::string gui_locale_;
            
        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t parse_response_data(const rapidjson::Value &data) override;

        public:
            phoneinfo(const wim_packet_params &params, const std::string &phone, const std::string &gui_locale);
            virtual ~phoneinfo();
        };

    }

}

#endif // __WIM_PHONEINFO_H_
