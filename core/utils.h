#pragma once

namespace core
{
    namespace utils
    {
        typedef boost::optional<std::string> string_opt;

        std::wstring get_product_data_path();
        std::string get_product_name();
        std::string get_app_name();
        std::string get_user_agent(const string_opt &_uin = string_opt());
        std::string get_platform_string();

        std::wstring get_report_path();
        std::wstring get_report_log_path();
        std::wstring get_report_mini_dump_path();

        const boost::filesystem::wpath get_logs_path();

        bool is_writable(const boost::filesystem::path &p);
    }
}
