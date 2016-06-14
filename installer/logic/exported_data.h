#pragma once

class MRABase;

namespace installer
{
    namespace logic
    {
        struct wim_account
        {
            std::wstring database_key_;
            std::string login_;
            std::string nick_;
            std::string aimid_;
            std::string token_;
            std::string session_key_;
            std::string devid_;
            time_t exipired_in_;
            time_t time_offset_;
            std::string aim_sid_;
            std::string fetch_url_;

            std::vector<char> avatar_;

            wim_account()
                :   exipired_in_(0),
                    time_offset_(0)
            {

            }
        };

        struct settings_8x
        {
            bool show_in_taskbar_;
            bool enable_sounds_;
            bool auto_save_files_;
            std::string path_file_save_; 
            int send_hotkey_;
            bool enable_preview_;
            std::string language_;
            bool notify_messages_;
            bool auto_run_;

            settings_8x()
                :   show_in_taskbar_(true),
                    enable_sounds_(true),
                    auto_save_files_(true),
                    send_hotkey_(1),
                    enable_preview_(true),
                    language_("en"),
                    notify_messages_(true),
                    auto_run_(true)
            {

            }
        };

        typedef std::vector<std::shared_ptr<wim_account>> accounts_list;

        class exported_data
        {
            accounts_list accounts_list_;
            std::shared_ptr<settings_8x> settings_;

            std::wstring get_profile_folder() const;
            std::wstring get_options_database_filename() const;
            std::wstring get_key_file_name() const;

            std::shared_ptr<wim_account> exported_account_;
            
            void read_accounts(MRABase& _base);
            void read_settings(MRABase& _base);

        public:

            exported_data();
            virtual ~exported_data();
                                  
            void read(bool _accounts, bool _settings);

            std::shared_ptr<settings_8x> get_settings();
            
            const accounts_list& get_accounts() const;
            void set_exported_account(std::shared_ptr<wim_account> _account);
            void store_exported_account(const QString& _file_name, bool _is_from_8x);
            void store_exported_settings(const QString& _file_name, bool _is_from_8x);
        };

        exported_data& get_exported_data();

        installer::error set_8x_update_downloaded();
    }
}

