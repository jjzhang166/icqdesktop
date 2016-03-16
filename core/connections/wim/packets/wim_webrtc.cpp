#include "stdafx.h"
#include "wim_webrtc.h"

#include "../../../http_request.h"


#if 1
    #define LOGIN_SERVER    "api.login.icq.net"
    #define SESSION_SERVER  "api.icq.net"
#else // sandbox
    #define LOGIN_SERVER    "178.22.94.24"
    #define SESSION_SERVER  "178.22.94.82" // this can be changed
#endif

namespace core {
    namespace wim {

        wim_allocate::wim_allocate(const wim_packet_params& params, const std::string& internal_params)
            : wim_packet(params) 
            , _internal_params(internal_params) {

        }

        wim_allocate::~wim_allocate() {

        }

        int32_t wim_allocate::init_request(std::shared_ptr<core::http_request_simple> _request) {
            if (!_request) { assert(false); return 1; }
            if (params_.a_token_.empty()) { assert(false); return 1; }
            if (params_.dev_id_.empty()) { assert(false); return 1; }

            std::string host = "https://" SESSION_SERVER "/webrtc/alloc";
            const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

            using namespace std::chrono;
            const milliseconds msNow = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

            std::stringstream randNum;
            randNum << msNow.count() << '_' << rand();;
			
            std::map<std::string, std::string> params;
            params["a"] = params_.a_token_;
            params["f"] = "json";
            params["k"] = params_.dev_id_;
            params["r"] = escape_symbols(randNum.str());
            params["ts"] = tools::from_int64(ts);

            if (!_internal_params.empty()) {
                long amp_pos = std::string::npos;
                long start_pos = amp_pos;
                do {
                    amp_pos++;
                    start_pos = amp_pos;
                    amp_pos = _internal_params.find('&', amp_pos);
                    std::string sstr = amp_pos != std::string::npos ? _internal_params.substr(start_pos, amp_pos - start_pos) : _internal_params.substr(start_pos);
                    long eq_pos = sstr.find('=');
                    if (std::string::npos != eq_pos) {
                        std::string nam = sstr.substr(0, eq_pos);
                        std::string val = sstr.substr(eq_pos + 1);

                        if (!nam.empty() && !val.empty()) {
                            params[nam] = val;
                        } else {
                            assert(false);
                        }
                    }
                } while(amp_pos != std::string::npos);
            }

			const auto sha256 = escape_symbols(get_url_sign(host, params, params_, false));
            params["sig_sha256"] = sha256;
            std::stringstream ss_url;
            ss_url << host.c_str();

            bool first = true;
            for (auto it : params) {
                if (first) {
                    ss_url << "?";
                } else {
                    ss_url << "&";
                }
                first = false;
                ss_url << it.first.c_str() << "=" << it.second.c_str();
            }

            _request->set_connect_timeout(10 * 1000);
            _request->set_timeout(15 * 1000);
            _request->set_url(ss_url.str());
            return 0;
        }

        int32_t wim_allocate::parse_response(std::shared_ptr<core::tools::binary_stream> response) {
            if (!response->available()) {
                assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
			return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_allocate::getRawData() {
            return _data;
        }


        wim_webrtc::wim_webrtc(const wim_packet_params& params, const voip_manager::VoipProtoMsg& internal_params)
            : wim_packet(params) 
            , _internal_params(internal_params) {

        }

        wim_webrtc::~wim_webrtc() {

        }


        int32_t wim_webrtc::init_request(std::shared_ptr<core::http_request_simple> _request) {
            if (!_request) { assert(false); return 1; }

            std::string host = "https://" SESSION_SERVER "/voip/webrtcMsg";
            std::stringstream ss_url;
            ss_url << host.c_str()
                << "?aimsid=" << params_.aimsid_.c_str()
                << "&f=json"
                << "&r=" << escape_symbols(_internal_params.requestId);

            if (!_internal_params.request.empty()) {
                ss_url << "&" << _internal_params.request.c_str();
            }

            _request->set_url(ss_url.str());
            return 0;
        }

        int32_t wim_webrtc::parse_response(std::shared_ptr<core::tools::binary_stream> response) {
            if (!response->available()) {
                assert(false);
                return wpie_http_empty_response;
            }

            _data = response;
			return 0;
        }

        std::shared_ptr<core::tools::binary_stream> wim_webrtc::getRawData() {
            return _data;
        }
    }
}