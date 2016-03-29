#include "stdafx.h"

#ifdef _WIN32

#include "crash_sender.h"
#include <fstream>
#include "log/log.h"
#include "utils.h"
#include "core.h"
#include "http_request.h"
#include "async_task.h"
#include <sys/types.h>
#include <sys/stat.h>

namespace core
{
    namespace dump
    {
        bool report_sender::send_to_hockey_app(const std::string& _login)
        {                
            // http://support.hockeyapp.net/kb/api/api-crashes
            core::http_request_simple post_request;
            post_request.set_url(hockeyapp_url);
            post_request.set_post_form(true);
            post_request.push_post_form_parameter("contact", _login);
            post_request.push_post_form_filedata(L"log", utils::get_report_log_path());
            post_request.push_post_form_filedata(L"attachment0", utils::get_report_mini_dump_path());
            return post_request.post();
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
            boost::filesystem::remove_all(utils::get_report_path().c_str());
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

                std::weak_ptr<report_sender> wr_this = shared_from_this();
                send_thread_->run_async_function([wr_this]
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return 0;

                    if (ptr_this->send_to_hockey_app(ptr_this->login_))
                        ptr_this->clear_report_folder();
                    return 0;
                });
            }
        }
    }
}

#endif // _WIN32