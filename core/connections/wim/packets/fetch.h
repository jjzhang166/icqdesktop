#ifndef __FETCH_H__
#define __FETCH_H__

#pragma once


#include "../wim_packet.h"
#include "../auth_parameters.h"

namespace core
{
    namespace wim
    {
        class fetch_event;

        class fetch : public wim_packet
        {
            std::string fetch_url_;
            time_t timeout_;
            std::function<bool(int32_t)> wait_function_;
            timepoint fetch_time_;
            bool session_ended_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            std::list< std::shared_ptr<core::wim::fetch_event> > events_;

            std::string next_fetch_url_;
            timepoint next_fetch_time_;
            time_t ts_;
            time_t time_offset_;

        public:

            const std::string get_next_fetch_url() const { return next_fetch_url_; }
            const timepoint get_next_fetch_time() const { return next_fetch_time_; }
            const time_t get_ts() const { return ts_; }
            const time_t get_time_offset() const { return time_offset_; }
            bool is_session_ended() const;

            std::shared_ptr<core::wim::fetch_event> push_event(std::shared_ptr<core::wim::fetch_event> _event);
            std::shared_ptr<core::wim::fetch_event> pop_event();

            fetch(
                const wim_packet_params& params, 
                const std::string& fetch_url, 
                time_t timeout, 
                timepoint _fetch_time,
                std::function<bool(int32_t)> wait_function);
            virtual ~fetch();
        };

    }
}


#endif//__FETCH_H__