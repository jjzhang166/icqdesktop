#ifndef __SPAM_REPORT_H_
#define __SPAM_REPORT_H_

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
        class spam_report : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

            std::string message_text_;
            std::string uin_spam_;
            std::string uin_from_;
            time_t message_time_;

            std::string get_report_xml(time_t _current_time);
            std::string get_report();

        public:

            spam_report(
                const wim_packet_params& _params,
                const std::string& _message_text,
                const std::string& _uin_spam,
                const std::string& _uin_from,
                time_t _message_time
                );
            virtual ~spam_report();
        };
    }
}

#endif //__SPAM_REPORT_H_