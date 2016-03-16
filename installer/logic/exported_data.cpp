#include "stdafx.h"
#include "exported_data.h"
#include "../legacy/gbase.h"
#include "../legacy/makarov_blowfish.h"
#include "../legacy/login_data.h"
#include "../legacy/http_login_session.h"
#include "../legacy/contact_info.h"
#include "../legacy/const.h"

namespace installer
{
    namespace logic
    {
        const std::wstring profile_folder = L"ICQ-Profile";

        exported_data::exported_data()
            : exported_account_(nullptr)
        {
        }


        exported_data::~exported_data()
        {
        }

        std::wstring exported_data::get_profile_folder() const
        {
            std::wstringstream icq_profile_path;

            wchar_t app_data[1024] = { 0 };

            if (::SHGetSpecialFolderPath(NULL, app_data, CSIDL_APPDATA, TRUE))
            {
                icq_profile_path << app_data << L"\\" << profile_folder;
            }

            return icq_profile_path.str();
        }

        std::wstring exported_data::get_options_database_filename() const
        {
            return get_profile_folder() + L"\\Base\\opt.dbs";
        }

        std::wstring exported_data::get_key_file_name() const
        {
            return get_profile_folder() + L"\\Update\\ver.txt";
        }

        std::shared_ptr<wim_account> convert_from_8x(const wim_auth_parameters& _old_account, const MAKFC_CLoginData& _ld)
        {
            auto new_account = std::make_shared<wim_account>();

            new_account->login_ = _ld.GetLogin().NetStrA(CP_UTF8);
            new_account->aimid_ = _old_account.m_aimid.NetStrA(CP_UTF8);
            new_account->token_ = _old_account.m_a_token.NetStrA(CP_UTF8);
            new_account->session_key_ = _old_account.m_session_key.NetStrA(CP_UTF8);
            new_account->devid_ = _old_account.m_dev_id.NetStrA(CP_UTF8);
            new_account->exipired_in_ = _old_account.m_exipired_in;
            new_account->time_offset_ = (time_t) _old_account.m_time_offset;
            new_account->aim_sid_ = _old_account.m_aim_sid.NetStrA(CP_UTF8);
            new_account->fetch_url_ = _old_account.m_fetch_url.NetStrA(CP_UTF8);

            return new_account;
        }

        bool read_avatar_from_file(const std::wstring& _file, std::vector<char>& _data)
        {
            CHandle file(::CreateFile(_file.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
            if (file.m_h == INVALID_HANDLE_VALUE)
                return false;

            unsigned int file_size = ::GetFileSize(file.m_h, 0);
            if (!file_size)
                return false;

            _data.resize(file_size);
            DWORD size_read = 0;
            if (!::ReadFile(file.m_h, &_data[0], file_size, &size_read, 0) || size_read != file_size)
            {
                _data.clear();
                return false;
            }

            return true;
        }
        
        bool read_avatar(const MAKFC_CLoginData& _login_data, const std::wstring& _profile_folder, std::vector<char>& _data)
        {
            static std::vector<std::wstring> file_names;

            if (file_names.empty())
            {
                file_names.reserve(10);
                file_names.push_back(L"avatar_180180.jpg");
                file_names.push_back(L"avatar_120120.jpg");
                file_names.push_back(L"avatar_100100.jpg");
                file_names.push_back(L"avatar_9090.jpg");
                file_names.push_back(L"avatar_6464.jpg");
                file_names.push_back(L"avatar_5050.jpg");
                file_names.push_back(L"avatar_4545.jpg");
                file_names.push_back(L"avatar_3232.jpg");
                file_names.push_back(L"avatar_2222.jpg");
            }

            std::wstring avatars_folder = _profile_folder + L"\\Avatars\\" + (LPCWSTR) _login_data.GetProtocolUID() + L"###ICQ";

            for (const auto& _file : file_names)
            {
                if (read_avatar_from_file(avatars_folder + L"\\" + _file, _data))
                    return true;
            }

            return false;
        }


        std::string get_nick(MRABase& _base, const MAKFC_CLoginData& _login_data)
        {
            std::string nick = _login_data.GetLogin().NetStrA(CP_UTF8);

            tstring tnick;
            if (_base.ReadString((LPCWSTR) GetDatabaseKey(_login_data), L"", L"ICQMyNick", tnick))
            {
                nick = MAKFC_CString(tnick.c_str()).NetStrA(CP_UTF8);
            }
            else
            {
                MAKFC_CContactInfo rci;
                if (_base.RCI_Get((LPCWSTR) GetDatabaseKey(_login_data), (LPCWSTR) _login_data.GetLogin(), rci))
                {
                    if (!rci.nickname.IsEmpty())
                    {
                        nick = rci.nickname.NetStrA(CP_UTF8);
                    }
                    else if (!rci.firstname.IsEmpty() || !rci.lastname.IsEmpty())
                    {
                        nick = std::string(rci.firstname.NetStrA(CP_UTF8)) + " " + std::string(rci.lastname.NetStrA(CP_UTF8));
                    }
                } 
            }
                        
            return nick;
        }

        
        void exported_data::read_accounts(MRABase& _base)
        {
            accounts_list_.reserve(10);
            
            CHandle key_file(::CreateFile(get_key_file_name().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
            if (key_file.m_h == INVALID_HANDLE_VALUE)
                return;

            BLOWFISHKEY key = {0};
            DWORD dwRead = 0;
            if (!::ReadFile(key_file, &key, sizeof(BLOWFISHKEY), &dwRead, NULL) || dwRead != sizeof(BLOWFISHKEY))
                return;

            LOGINRECORDS records = LOGIN_ConvertPasswords(key);

            for (const auto& login_data : records)
            {
                auto login_data_conv = login_data;
                login_data_conv.SetLogin(login_data.GetLogin().Mid(4), login_data.GetProtocolUID(), login_data.m_nLoginType);
                                                               
                auto auth_par = load_auth_params_from_db(_base, login_data_conv);
                if (auth_par)
                {
                    auto converted_params = convert_from_8x(*auth_par, login_data_conv);
                    converted_params->nick_ = get_nick(_base, login_data_conv);
                    converted_params->database_key_ = GetDatabaseKey(login_data_conv);
                    read_avatar(login_data_conv, get_profile_folder(), converted_params->avatar_);

                    accounts_list_.push_back(converted_params);
                }

                for (const MAKFC_CLoginData* login_data_child : login_data.m_imLogins)
                {
                    MAKFC_CLoginData login_data_child_conv(*login_data_child);
                    login_data_child_conv.UnPackLogin();

                    auth_par = load_auth_params_from_db(_base, login_data_child_conv);
                    if (auth_par)
                    {
                        auto converted_params_child = convert_from_8x(*auth_par, login_data_child_conv);
                        converted_params_child->nick_ = get_nick(_base, login_data_child_conv);
                        converted_params_child->database_key_ = GetDatabaseKey(login_data_child_conv);
                        read_avatar(login_data_child_conv, get_profile_folder(), converted_params_child->avatar_);
                        accounts_list_.push_back(converted_params_child);
                    }
                }
            }


        }

#define HOTKEY_ENTER		1
#define HOTKEY_DBENTER		2
#define HOTKEY_ALTS			4
#define HOTKEY_CTRLENTER	8


        void exported_data::read_settings(MRABase& _base)
        {
            auto settings = std::make_shared<settings_8x>();

            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, STR_CS_ICQ_MRA_KEY, KEY_READ) != ERROR_SUCCESS)
                return;

            DWORD val = 0;
            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_show_cl_in_taskbar", val))
                settings->show_in_taskbar_ = !!val;

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"sound_scheme", val))
                settings->enable_sounds_ = !!val;

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_autosave_incoming_files", val))
                settings->auto_save_files_ = !!val;

            if (!accounts_list_.empty())
            {
                std::wstring path;
                if (_base.ReadString((*accounts_list_.begin())->database_key_, L"", L"SAVEPATH_DEFAULTW", path))
                    settings->path_file_save_ = MAKFC_CString(path.c_str()).Trim(L"\\").NetStrA(CP_UTF8);
                
                if (_base.ReadDWORD((*accounts_list_.begin())->database_key_, L"", L"ENABLE_PREVIEW", val))
                    settings->enable_preview_ = !!val;
            }

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_sendhotkey", val))
            {
                if (val & HOTKEY_CTRLENTER)
                    settings->send_hotkey_ = Qt::Key_Control;
                else if (val & HOTKEY_DBENTER)
                    settings->send_hotkey_ = Qt::Key_Enter;
                else
                    settings->send_hotkey_ = 0;
            }

            wchar_t lang[1024];
            val = 1023;
            if (ERROR_SUCCESS == icq_key.QueryStringValue(L"lang", lang, &val))
            {
                settings->language_ = MAKFC_CString(lang).NetStrA(CP_UTF8);
                if (settings->language_ == "ua")
                    settings->language_ = "uk";
                else if (settings->language_ == "cz")
                    settings->language_ = "cs";

            }

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_messagetooltips", val))
                settings->notify_messages_ = !!val;

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_autorun", val))
                settings->auto_run_ = !!val;

            settings_ = settings;
        }

        void exported_data::read(bool _accounts, bool _settings)
        {
            MRABase base;
            if (!base.Open(get_options_database_filename()))
                return;

            if (_accounts)
                read_accounts(base);

            if (_settings)
                read_settings(base);
        }
        
        const accounts_list& exported_data::get_accounts() const
        {
            return accounts_list_;
        }
        
        void exported_data::set_exported_account(std::shared_ptr<wim_account> _account)
        {
            exported_account_ = _account;
        }

        void exported_data::store_exported_account(const QString& _file_name)
        {
            if (!exported_account_)
                return;

            rapidjson::Document doc(rapidjson::Type::kObjectType);

            auto& a = doc.GetAllocator();

            doc.AddMember("login", exported_account_->login_, a);
            doc.AddMember("aimid", exported_account_->aimid_, a);
            doc.AddMember("atoken", exported_account_->token_, a);
            doc.AddMember("sessionkey", exported_account_->session_key_, a);
            doc.AddMember("devid", exported_account_->devid_, a);
            doc.AddMember("expiredin", exported_account_->exipired_in_, a);
            doc.AddMember("timeoffset", exported_account_->time_offset_, a);
            doc.AddMember("aimsid", exported_account_->aim_sid_, a);
            doc.AddMember("fetchurl", exported_account_->fetch_url_, a);
            
            
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            std::string json_string = buffer.GetString();

            QFile file(_file_name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return;
                
            if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
            {
            }

            file.close();
        }

        void exported_data::store_exported_settings(const QString& _file_name)
        {
            if (!settings_)
                return;

            rapidjson::Document doc(rapidjson::Type::kObjectType);

            auto& a = doc.GetAllocator();

            doc.AddMember(settings_show_in_taskbar, settings_->show_in_taskbar_, a);
            doc.AddMember(settings_sounds_enabled, settings_->enable_sounds_, a);
            doc.AddMember(settings_download_files_automatically, settings_->auto_save_files_, a);
            if (!settings_->path_file_save_.empty())
                doc.AddMember(settings_download_directory, settings_->path_file_save_, a);
            doc.AddMember(settings_key1_to_send_message, settings_->send_hotkey_, a);
            doc.AddMember(settings_show_video_and_images, settings_->enable_preview_, a);
            doc.AddMember(settings_language, settings_->language_, a);
            doc.AddMember(settings_notify_new_messages, settings_->notify_messages_, a);
                        
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            std::string json_string = buffer.GetString();

            QFile file(_file_name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return;

            if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
            {
            }

            file.close();
        }

        std::shared_ptr<settings_8x> exported_data::get_settings()
        {
            return settings_;
        }

        exported_data& get_exported_data()
        {
            static exported_data exp_data;

            return exp_data;
        }


        void set_8x_update_downloaded()
        {
            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, STR_CS_ICQ_MRA_KEY, KEY_SET_VALUE) != ERROR_SUCCESS)
                return;

            auto res = icq_key.SetDWORDValue(STR_REG_INSTALLER_DOWNLOADED, 1);
        }
    }
}