#include "stdafx.h"
#include "write_registry.h"
#include "../tools.h"
#include "../../../common.shared/version_info_constants.h"
#include <regex>
#include "../exported_data.h"

namespace installer
{
    namespace logic
    {
        installer::error write_to_uninstall_key()
        {
            CAtlString uninstall_command = (LPCWSTR)(QString("\"") + get_installer_exe().replace('/', '\\') + "\"" + " -uninstall").utf16();
            CAtlString display_name = (LPCWSTR)(get_product_display_name() + " (" + QT_TRANSLATE_NOOP("write_to_uninstall", "version") + " " + VERSION_INFO_STR + ")").utf16();
            CAtlString exe = (LPCWSTR) get_icq_exe().replace('/', '\\').utf16();
            CAtlString install_path = (LPCWSTR) get_install_folder().replace('/', '\\').utf16();

            CRegKey key_current_version;
            if (ERROR_SUCCESS != key_current_version.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software\\Microsoft\\Windows\\CurrentVersion");

            CRegKey key_uninstall;
            if (ERROR_SUCCESS != key_uninstall.Create(key_current_version, L"Uninstall"))
                return installer::error(errorcode::create_registry_key, QString("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));

            CRegKey key_icq;
            if (ERROR_SUCCESS != key_icq.Create(key_uninstall, L"icq.desktop"))
                return installer::error(errorcode::create_registry_key, QString("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\icq.desktop"));

            QString version = VERSION_INFO_STR;
            QString major_version, minor_version;
            QStringList version_list = version.split('.');
            if (version_list.size() > 2)
            {
                major_version = version_list[0];
                minor_version = version_list[1];
            }


            if (
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayName", display_name) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayIcon", CAtlString(L"\"") + exe + L"\"") ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayVersion", (LPCWSTR) version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"VersionMajor", (LPCWSTR) major_version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"VersionMinor", (LPCWSTR) minor_version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"Publisher", (LPCWSTR) get_company_name().utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"InstallLocation", install_path) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"UninstallString", uninstall_command)
                )
            {
                return installer::error(errorcode::set_registry_value, "set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\icq.desktop\\...");
            }

            return installer::error();
        }

        CAtlString get_default_referer()
        {
            return (const wchar_t*) get_product_name().utf16();
        }

        CAtlString get_referer()
        {
            CAtlString referer = get_default_referer();

            wchar_t buffer[1025] = {0};

            if (::GetModuleBaseName(::GetCurrentProcess(), 0, buffer, 1024))
            {
                CAtlString module_name = buffer;

                std::wcmatch mr; 
                std::wregex rx(L"[^a-zA-Z0-9_-]");
                if (std::regex_search((const wchar_t*) module_name, mr, rx))
                {
                    CAtlString omit = mr[0].first;
                    module_name = module_name.Mid(0, module_name.Find(omit));
                }

                CAtlString referer_prefix = L"_rfr";
                int rfr_pos = module_name.Find(referer_prefix);
                if (rfr_pos != -1)
                {
                    referer = module_name.Mid(rfr_pos + referer_prefix.GetLength());
                }
            }

            return referer;
        }

        void write_referer(CRegKey& _key_product)
        {
            CAtlString referer = get_referer();

            _key_product.SetStringValue(L"referer", referer);

            ULONG len = 1024;
            wchar_t buffer[1025];
            if (_key_product.QueryStringValue(L"referer_first", buffer, &len) != ERROR_SUCCESS)
                _key_product.SetStringValue(L"referer_first", referer);
        }

        installer::error write_registry()
        {
            installer::error err;

            QString exe_path = get_icq_exe();
            exe_path = exe_path.replace('/', '\\');

            if (!get_exported_data().get_settings() || get_exported_data().get_settings()->auto_run_)
            {
                CRegKey key_software_run;
                if (ERROR_SUCCESS != key_software_run.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE))
                    return installer::error(errorcode::open_registry_key, "open registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Run");

                if (ERROR_SUCCESS != key_software_run.SetStringValue((const wchar_t*) get_product_name().utf16(), (const wchar_t*)(QString("\"") + exe_path + "\"").utf16()))
                    return installer::error(errorcode::set_registry_value, "set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Run icq.desktop");
            }


            CRegKey key_software;

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software");

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, QString("create registry key: ") + get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"path", (const wchar_t*) exe_path.utf16()))
                return installer::error(errorcode::set_registry_value, "set registry value: Software\\icq.desktop path");

            write_referer(key_product);

            err = write_to_uninstall_key();
            if (!err.is_ok())
                return err;

            return err;
        }

        installer::error clear_registry()
        {
            CAtlString key_product = CAtlString(L"Software\\") + (const wchar_t*) get_product_name().utf16();

            ::SHDeleteKey(HKEY_CURRENT_USER, key_product);
            ::SHDeleteKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\icq.desktop");
            ::SHDeleteValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", (const wchar_t*) get_product_name().utf16());

            return installer::error();
        }

        installer::error write_update_version()
        {
            CRegKey key_software;

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software");

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, QString("create registry key: ") + get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"update_version", (const wchar_t*) QString(VERSION_INFO_STR).utf16()))
                return installer::error(errorcode::set_registry_value, "set registry value: Software\\icq.desktop update_version");

            return installer::error();
        }
    }
}