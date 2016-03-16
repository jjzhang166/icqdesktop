#include "stdafx.h"
#include "proxy_settings.h"
#include "../external/curl/include/curl.h"

namespace core
{
    proxy_settings_manager::proxy_settings_manager()
        : switched_(false)
    {
    }

    proxy_settings_manager::~proxy_settings_manager()
    {
    }

    proxy_settings proxy_settings_manager::get()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        return switched_ ? registry_settings_ : settings_;
    }

    proxy_settings proxy_settings_manager::get_registry_settings()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        init_from_registry();
        return registry_settings_;
    }

    void proxy_settings_manager::switch_settings()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        init_from_registry();
        if (registry_settings_.use_proxy_)
            switched_ = true;
        else
            switched_ = false;
    }

    void proxy_settings_manager::init_from_registry()
    {
        registry_settings_.use_proxy_ = false;
#ifdef _WIN32
        CRegKey key;
        if (key.Open(HKEY_CURRENT_USER, CAtlString(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")) == ERROR_SUCCESS)
        {
            DWORD proxy_used = 0;
            if (key.QueryDWORDValue(L"ProxyEnable", proxy_used) == ERROR_SUCCESS)
            {
                if (proxy_used == 0)
                    return;

                wchar_t buffer[MAX_PATH];
                unsigned long len = MAX_PATH;
                if (key.QueryStringValue(L"ProxyServer", buffer, &len) == ERROR_SUCCESS)
                {
                    registry_settings_.proxy_server_ = std::wstring(buffer);
                    if (registry_settings_.proxy_server_.empty())
                        return;

                    registry_settings_.use_proxy_ = true;
                    registry_settings_.proxy_type_ = CURLPROXY_HTTP;

                    if (key.QueryStringValue(L"ProxyUser", buffer, &len) == ERROR_SUCCESS)
                    {
                        registry_settings_.login_ = std::wstring(buffer);
                        if (registry_settings_.login_.empty())
                            return;

                        registry_settings_.need_auth_ = true;

                        if (key.QueryStringValue(L"ProxyPass", buffer, &len) == ERROR_SUCCESS)
                        {
                            registry_settings_.password_ = std::wstring(buffer);
                        }
                    }
                }
            }
        }
#endif //_WIN32
    }
}