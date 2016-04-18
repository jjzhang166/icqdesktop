#pragma once
#ifdef _WIN32

#include "../common.shared/keys.h"
#include "proxy_settings.h"

namespace core
{
    class async_executer;

    namespace dump
    {
        const static std::string hockeyapp_url =  "https://rink.hockeyapp.net/api/2/apps/" + hockey_app_id + "/crashes/upload";

        class report_sender : public std::enable_shared_from_this<report_sender>
        {
        public:
            report_sender(const std::string& _login);
            void send_report();
            ~report_sender();
        private:
            void clear_report_folder();
            bool is_report_existed();
            bool send_to_hockey_app(const std::string& _login, const proxy_settings& _proxy);
            std::unique_ptr<async_executer> send_thread_;
            std::string login_;
        };
    }
}

#endif // _WIN32
