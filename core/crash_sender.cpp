#include "stdafx.h"

#ifdef _WIN32

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "crash_sender.h"
#include "log/log.h"
#include "utils.h"
#include "core.h"
#include "http_request.h"
#include "async_task.h"
#include "../common.shared/keys.h"

namespace core
{
    namespace dump
    {
        const std::string& get_hockeyapp_url()
        {
            static std::string hockeyapp_url = "https://rink.hockeyapp.net/api/2/apps/" + hockey_app_id + "/crashes/upload";
            return hockeyapp_url;
        }

        bool report_sender::send_to_hockey_app(const std::string& _login, const proxy_settings& _proxy)
        {
            // http://support.hockeyapp.net/kb/api/api-crashes
            core::http_request_simple post_request(_proxy, utils::get_user_agent());
            post_request.set_url(get_hockeyapp_url());
            post_request.set_post_form(true);
            post_request.push_post_form_parameter("contact", _login);
            post_request.push_post_form_filedata(L"log", utils::get_report_log_path());

            auto dump_name = utils::get_report_mini_dump_path();
            try
            {
                boost::system::error_code e;
                auto size = boost::filesystem::file_size(dump_name, e);
                if (size < 1024 * 1024) // 1 mb
                    post_request.push_post_form_filedata(L"attachment0", dump_name);
                return post_request.post();
            }
            catch (const std::exception& /*exp*/)
            {
                // if error - delete folder
                return true;
            };
        }

        bool report_sender::is_report_existed()
        {
            struct stat info;
            if (stat(tools::from_utf16(utils::get_report_path()).c_str(), &info) != 0)
                return false;
            else if(info.st_mode & S_IFDIR)
                return true;
            else
                return false;
        }

        void report_sender::clear_report_folder()
        {
            boost::system::error_code e;
            boost::filesystem::remove_all(utils::get_report_path().c_str(), e);
        }

        report_sender::report_sender(const std::string& _login)
            : login_(_login)
        {
        }

        report_sender::~report_sender()
        {
            send_thread_.reset();
        }

        void report_sender::send_report()
        {
            if (core::dump::report_sender::is_report_existed())
            {
                send_thread_.reset(new async_executer());

                auto user_proxy = g_core->get_user_proxy_settings();

                std::weak_ptr<report_sender> wr_this = shared_from_this();
                send_thread_->run_async_function([wr_this, user_proxy]
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return 0;

                    if (ptr_this->send_to_hockey_app(ptr_this->login_, user_proxy))
                        ptr_this->clear_report_folder();
                    return 0;
                });
            }
        }
    }
}

#endif // _WIN32