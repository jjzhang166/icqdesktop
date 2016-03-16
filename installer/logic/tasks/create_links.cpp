#include "stdafx.h"
#include "create_links.h"
#include "../tools.h"
#include "../../utils/links/shell_link.h"

#include <comutil.h>

#define STR_APP_NAME_SHORTCUT_EN L"ICQ.lnk"
#define STR_APP_NAME_EN L"ICQ"
#define STR_APP_USER_MODEL_ID L"ICQ.Client"

namespace installer
{
	namespace logic
	{
		installer::error create_links()
		{
            ::CoInitializeEx(0, COINIT_MULTITHREADED);

			installer::error err;

			CAtlString icq_exe = (const wchar_t*) get_icq_exe().replace('/', '\\').utf16();
			CAtlString installer_exe = (const wchar_t*) get_installer_exe().replace('/', '\\').utf16();
			CAtlString icq_exe_short = (const wchar_t*) get_icq_exe_short().utf16();

			CAtlString program_dir, start, desktop, quick_launch;

			// get spaecial folders
			{
				if (!::SHGetSpecialFolderPath(0, program_dir.GetBuffer(4096), CSIDL_PROGRAMS, 0))
					return installer::error(errorcode::get_special_folder);
				program_dir.ReleaseBuffer();
				program_dir += CAtlString(L"\\") + (const wchar_t*) get_product_display_name().utf16();

				if (!::SHGetSpecialFolderPath(0, start.GetBuffer(4096), CSIDL_STARTMENU, 0))
					return installer::error(errorcode::get_special_folder);
				start.ReleaseBuffer();

				if (!::SHGetSpecialFolderPath(0, desktop.GetBuffer(4096), CSIDL_DESKTOPDIRECTORY, 0))
					return installer::error(errorcode::get_special_folder);
				desktop.ReleaseBuffer();

				quick_launch = links::GetQuickLaunchDir();
			}

			links::RemoveFromMFUList(icq_exe_short);

			if (::GetFileAttributes(program_dir) == (DWORD)-1 && !::CreateDirectory(program_dir, NULL))
				return installer::error(errorcode::get_special_folder);

			int iIconIndex = 0;

			CAtlString str = program_dir + L"\\"STR_APP_NAME_SHORTCUT_EN;
			if (S_OK != links::CreateLink(icq_exe, L"", str, STR_APP_NAME_EN, iIconIndex))
			{
				assert(!"link creation was failed!");
			}

			str = program_dir + L"\\Uninstall "STR_APP_NAME_SHORTCUT_EN;
			if (S_OK != links::CreateLink(installer_exe, L"-uninstall", str, L"Uninstall "STR_APP_NAME_EN, 1))
			{
				assert(!"link creation was failed!");
			}

			str = start + L"\\"STR_APP_NAME_SHORTCUT_EN;
			if (S_OK != links::CreateLink(icq_exe, L"", str, STR_APP_NAME_EN,	iIconIndex))
			{
				assert(!"link creation was failed!");
			}

			str = links::GetQuickLaunchDir();
			str += L"\\"STR_APP_NAME_SHORTCUT_EN;
			CComPtr<IShellLink> psl;
			psl.Attach(links::CreateShellLink(icq_exe,(LPCTSTR) L"", STR_APP_NAME_EN, iIconIndex));
			if (psl == NULL)
			{
				assert(!"link creation was failed!");
			}

			links::SetShellLinkTitle(psl, STR_APP_NAME_EN);
			links::SetShellLinkAppID(psl, STR_APP_USER_MODEL_ID);
			links::SaveShellLink(psl, str);

			str = desktop + L"\\"STR_APP_NAME_SHORTCUT_EN;
			psl.Attach(links::CreateShellLink(icq_exe,(LPCTSTR) L"", STR_APP_NAME_EN, iIconIndex));
			if (psl == NULL)
			{
				assert(!"link creation was failed!");
			}

			links::SetShellLinkTitle(psl, STR_APP_NAME_EN);
			links::SetShellLinkAppID(psl, STR_APP_USER_MODEL_ID);
			links::SaveShellLink(psl, str);

			links::PinToTaskbar(str);
			links::PinToStartmenu(str);

			::CoUninitialize();

			return err;
		}

		installer::error delete_links()
		{
			::CoInitializeEx(0, COINIT_MULTITHREADED);

			installer::error err;

			CAtlString icq_exe = (const wchar_t*) get_icq_exe().replace('/', '\\').utf16();
			CAtlString installer_exe = (const wchar_t*) get_installer_exe().replace('/', '\\').utf16();
			CAtlString icq_exe_short = (const wchar_t*) get_icq_exe_short().utf16();

			CAtlString program_dir, start, desktop, quick_launch;

			// get special folders
			{
				if (!::SHGetSpecialFolderPath(0, program_dir.GetBuffer(4096), CSIDL_PROGRAMS, 0))
					return installer::error(errorcode::get_special_folder);
				program_dir.ReleaseBuffer();
				program_dir += CAtlString(L"\\") + (const wchar_t*) get_product_display_name().utf16();

				if (!::SHGetSpecialFolderPath(0, start.GetBuffer(4096), CSIDL_STARTMENU, 0))
					return installer::error(errorcode::get_special_folder);
				start.ReleaseBuffer();

				if (!::SHGetSpecialFolderPath(0, desktop.GetBuffer(4096), CSIDL_DESKTOPDIRECTORY, 0))
					return installer::error(errorcode::get_special_folder);
				desktop.ReleaseBuffer();

				quick_launch = links::GetQuickLaunchDir();
			}

			links::RemoveFromMFUList(icq_exe_short);

			CAtlString str = program_dir + L"\\"STR_APP_NAME_SHORTCUT_EN;
			links::RemoveLink(str);

			str = program_dir + L"\\Uninstall "STR_APP_NAME_SHORTCUT_EN;
			links::RemoveLink(str);

			str = start + _T("\\") + STR_APP_NAME_SHORTCUT_EN;
			links::RemoveLink(str);

			str = links::GetQuickLaunchDir();
			str += L"\\"STR_APP_NAME_SHORTCUT_EN;
			links::RemoveLink(str);

			str = desktop;
			str += L"\\"STR_APP_NAME_SHORTCUT_EN;
			links::RemoveLink(str);

			return err;

			::CoUninitialize();
		}
	}
}