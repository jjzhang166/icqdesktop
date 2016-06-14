#ifndef __WIMPACKET_H_
#define __WIMPACKET_H_

#pragma once

#include "../../async_task.h"
#include "../../../corelib/common.h"
#include "../../proxy_settings.h"

namespace core
{
    class http_request_simple;

    namespace tools
    {
        class binary_stream;
    }
}

namespace core
{
    namespace wim
    {

#define WIM_CLIENT_DIST_ID		30016

        const std::string c_wim_host = "https://api.icq.net/";
        const std::string c_robusto_host = "https://rapi.icq.net/";

#define WIM_CAP_VOIP_VOICE   "094613504c7f11d18222444553540000"
#define WIM_CAP_VOIP_VIDEO   "094613514c7f11d18222444553540000"
#define WIM_CAP_FILETRANSFER "094613434c7f11d18222444553540000"
#define WIM_CAP_UNIQ_REQ_ID  "094613534c7f11d18222444553540000"
#define WIM_CAP_EMOJI        "094613544c7f11d18222444553540000"

#define SAAB_SESSION_OLDER_THAN_AUTH_UPDATE          1010



        //////////////////////////////////////////////////////////////////////////
        // wim protocol
        //////////////////////////////////////////////////////////////////////////
        enum wim_protocol_internal_error
        {
            wpie_invalid_login = 1,
            wpie_network_error = 2,
            wpie_http_error = 3,
            wpie_http_empty_response = 4,
            wpie_http_parse_response = 5,
            wpie_wrong_login = 6,
            wpie_request_timeout = 7,
            wpie_login_unknown_error = 8,

            wpie_start_session_wrong_credentials = 9,
            wpie_start_session_invalid_request = 10,
            wpie_start_session_request_timeout = 11,
            wpie_start_session_rate_limit = 12,
            wpie_start_session_wrong_devid = 13,
            wpie_start_session_unknown_error = 14,

            wpie_error_parse_response = 15,

            wpie_error_request_canceled = 16,

            wpie_error_too_fast_sending = 17,
            wpie_error_dest_not_avalible = 18,
            wpie_error_rate_limit = 19,

            wpie_error_message_unknown = 20,
            wpie_error_no_members_found = 21,

            wpie_get_sms_code_unknown_error = 22,
            wpie_phone_not_verified = 23,
            wpie_invalid_phone_number = 24,

            wpie_error_robusto_token_invalid = 25,
            wpie_error_robusto_bad_app_key = 26,
            wpie_error_robusto_icq_auth_expired = 27,
            wpie_error_robusto_bad_sigsha = 28,
            wpie_error_robusto_bad_ts = 29,

            wpie_error_internal_logic = 30,
            wpie_error_invalid_request = 31,
            wpie_error_profile_not_found = 32,

            wpie_error_robusto_you_are_not_chat_member = 33,

            wpie_error_attach_busy_phone = 34,

            wpie_error_too_large_file = 35,

            wpie_client_http_error = 400,

            wpie_error_need_relogin= 1000,

            wpie_error_task_canceled = 2000,

            wpie_error_empty_avatar_data = 4000
        };

        enum wim_protocol_error
        {
            OK = 200,
            MORE_AUTHN_REQUIRED = 330,
            INVALID_REQUEST = 400,
            AUTHN_REQUIRED = 401,
            REQUEST_TIMEOUT = 408,
            SEND_IM_RATE_LIMIT = 430,
            SESSION_NOT_FOUND = 451,
            MISSING_REQUIRED_PARAMETER = 460,
            PARAMETER_ERROR = 462,
            GENERIC_SERVER_ERROR = 500,
            INVALID_TARGET = 600,
            TARGET_DOESNT_EXIST = 601,
            TARGET_NOT_AVAILABLE = 602,
            CAPCHA = 603,
            MESSAGE_TOO_BIG = 606,
            TARGET_RATE_LIMIT_REACHED = 607
        };

        struct robusto_packet_params
        {
            std::string		robusto_token_;
            int32_t			robusto_client_id_;
            uint32_t		robusto_req_id_;

            robusto_packet_params()
                :
                robusto_client_id_(0),
                robusto_req_id_(0)
            {
            }

            robusto_packet_params(
                const std::string& robusto_token,
                int32_t robusto_client_id,
                uint32_t robusto_req_id)
                :
            robusto_token_(robusto_token),
                robusto_client_id_(robusto_client_id),
                robusto_req_id_(robusto_req_id)
            {
            }
        };

        struct wim_packet_params
        {
            std::function<bool()> stop_handler_;
            std::string a_token_;
            std::string session_key_;
            std::string dev_id_;
            std::string aimsid_;
            time_t time_offset_;
            std::string uniq_device_id_;
            std::string aimid_;
            proxy_settings proxy_;
            bool full_log_;

            wim_packet_params(
                std::function<bool()> _stop_handler,
                const std::string& _a_token,
                const std::string& _session_key,
                const std::string& _dev_id,
                const std::string& _aimsid,
                const std::string& _uniq_device_id,
                const std::string& _aimid,
                const time_t _time_offset,
                const proxy_settings& _proxy,
                const bool _full_log)
                :
            stop_handler_(_stop_handler),
                a_token_(_a_token),
                session_key_(_session_key),
                dev_id_(_dev_id),
                aimsid_(_aimsid),
                time_offset_(_time_offset),
                uniq_device_id_(_uniq_device_id),
                aimid_(_aimid),
                proxy_(_proxy),
                full_log_(_full_log)
            {
            }

            bool is_auth_valid() const
            {
                return (!a_token_.empty() && !session_key_.empty() && !dev_id_.empty() && !aimsid_.empty());
            }

        };

        class wim_packet : public async_task
        {
            std::string response_str_;
            std::string header_str_;

        protected:

            void load_response_str(const char* buf, unsigned size);
            const std::string& response_str() const;
            const std::string& header_str() const;
            
            uint32_t status_code_;
            uint32_t status_detail_code_;
            std::string status_text_;
            uint32_t http_code_;
            uint32_t repeat_count_;

            wim_packet_params params_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request);
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request);
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> response);
            virtual int32_t parse_response_data(const rapidjson::Value& _data);
            virtual void parse_response_data_on_error(const rapidjson::Value& _data);

            virtual int32_t on_empty_data();
            virtual int32_t on_response_error_code();

            virtual int32_t on_http_client_error();

            const std::string get_rand_float() const;
            const wim_packet_params& get_params() const;

        public:

            virtual int32_t execute() override;
            static std::string escape_symbols(const std::string& data);
            static std::string escape_symbols_data(const char* _data, uint32_t _len);
            static std::string get_url_sign(const std::string& host, const Str2StrMap& params, const wim_packet_params& _wim_params, bool post_method, bool make_escape_symbols = true);
            static std::string format_get_params(const Str2StrMap& _params);
            static std::string detect_digest(const std::string& hashed_data, const std::string& session_key);
            static std::string create_query_from_map(const Str2StrMap& params);
            static void replace_log_messages(tools::binary_stream& _bs);

            virtual std::shared_ptr<core::tools::binary_stream> getRawData() { return NULL; }
            uint32_t get_status_code() const { return status_code_; }
            uint32_t get_status_detail_code() const { return status_detail_code_; }
            std::string get_status_text() const { return status_text_; }
            uint32_t get_http_code() const { return http_code_; }
            uint32_t get_repeat_count() const;
            bool is_stopped() const;

            wim_packet(const wim_packet_params& _params);
            virtual ~wim_packet();
        };

    }
}



#endif //__WIMPACKET_H_
