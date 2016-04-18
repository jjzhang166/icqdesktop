#ifndef __UTILS_H_
#define __UTILS_H_

#pragma once

namespace core
{
    namespace utils
    {
        std::wstring get_product_data_path();
        std::string get_product_name();
        std::string get_app_name();
        std::string get_user_agent();
        std::string get_platform_string();

        std::wstring get_report_path();
        std::wstring get_report_log_path();
        std::wstring get_report_mini_dump_path();

        const boost::filesystem::wpath get_logs_path();
    }
}


#endif // !__UTILS_H_




