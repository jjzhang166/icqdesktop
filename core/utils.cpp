#include "stdafx.h"
#include "utils.h"

#include "tools/system.h"
#include "../../common.shared/version_info.h"

namespace core
{
    namespace utils
    {
        std::wstring get_product_data_path()
        {
#ifdef __linux__
            return (core::tools::system::get_user_profile() + L"/.config/" + L"ICQ");
#else
            return (core::tools::system::get_user_profile() + L"/" + L"ICQ");
#endif //__linux__
        }

        std::string get_product_name()
        {
            return "icq.desktop";
        }

        std::string get_app_name()
        {
            if (platform::is_windows())
            {
                return "Mail.ru Windows ICQ";
            }
            else if (platform::is_apple())
            {
                return "Mail.ru Mac OS X ICQ";
            }
            else if (platform::is_linux())
            {
                return "Mail.ru Linux ICQ";
            }
            else
            {
                assert(false);
                return "Mail.ru ICQ";
            }
        }

        std::string get_user_agent()
        {
            return (get_app_name() + " (version " + tools::version_info().get_version() + ")");
        }

        std::string get_platform_string()
        {
            if (platform::is_windows())
            {
                return "Win";
            }
            else if (platform::is_apple())
            {
                return "Apple";
            }
            else
            {
                assert(false);
                return "Unknown";
            }
        }

        std::wstring get_report_path()
        {
            return core::utils::get_product_data_path() + L"/reports";
        }

        std::wstring get_report_log_path()
        {
            return get_report_path() + L"/crash.log";
        }

        std::wstring get_report_mini_dump_path()
        {
            return get_report_path() + L"/crashdump.dmp";
        }

        const boost::filesystem::wpath get_logs_path()
        {
            boost::filesystem::wpath logs_dir(get_product_data_path());

            logs_dir.append(L"logs");

            return logs_dir;
        }
    }
}
