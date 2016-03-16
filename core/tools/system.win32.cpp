#include "stdafx.h"
#include <lm.h>

#include "system.h"
#include "win32/dll.h"
#include "../profiling/profiler.h"

#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "netapi32.lib")

namespace fs = boost::filesystem;

CORE_TOOLS_SYSTEM_NS_BEGIN

namespace
{
	std::wstring get_user_downloads_dir_xp();

	std::wstring get_user_downloads_dir_vista();
}

unsigned long get_current_thread_id()
{
	return ::GetCurrentThreadId();
}

bool is_dir_writable(const std::wstring &_dir_path_str)
{
    const fs::wpath dir_path(_dir_path_str);

    const auto is_dir = fs::is_directory(dir_path);
    assert(is_dir);

    if (!is_dir)
    {
        return false;
    }

    const auto test_path = (dir_path / generate_guid());

    {
        std::ofstream out(test_path.wstring());
        if (out.fail())
        {
            return false;
        }
    }

    fs::remove(test_path);

    return true;
}

bool move_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
	return !!::MoveFileEx(_old_file.c_str(), _new_file.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
}

bool copy_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    return !!::CopyFile(_old_file.c_str(), _new_file.c_str(), false);
}

bool compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2)
{
	if (_dir1.empty() || _dir2.empty())
		return false;

    return fs::equivalent(fs::path(_dir1), fs::path(_dir2));
}

std::wstring get_file_directory(const std::wstring& file)
{
    fs::wpath p(file);
    return p.parent_path().wstring();
}

std::wstring get_file_name(const std::wstring& file)
{
    fs::wpath p(file);
    return p.filename().wstring();
}

std::wstring get_user_profile()
{
	wchar_t buffer[MAX_PATH + 1];

	const auto error = ::SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, buffer);
	if (FAILED(error))
	{
		return std::wstring();
	}

	return buffer;
}

std::string generate_guid()
{
	std::string guid_string;

	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	guid_string = boost::lexical_cast<std::string>(uuid);

	return guid_string;
}

std::string generate_internal_id()
{
    static int internal_id = 0;
    std::stringstream guid_string;

    guid_string << generate_guid() << "-" << ++internal_id; 

    return guid_string.str();
}

std::wstring get_user_downloads_dir()
{
	static std::wstring cached_path;

	if (!cached_path.empty())
	{
		return cached_path;
	}

	cached_path = get_user_downloads_dir_vista();

	if (!cached_path.empty())
	{
		return cached_path;
	}

	cached_path = get_user_downloads_dir_xp();

	return cached_path;
}

std::string to_upper(std::string str)
{
    return boost::locale::to_upper(str);
}

namespace
{
	std::wstring get_user_downloads_dir_xp()
	{
		WCHAR path[MAX_PATH] = { 0 };

		const auto error = ::SHGetFolderPath(nullptr, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, nullptr, 0, Out path);
		if (FAILED(error))
		{
			return std::wstring();
		}

		assert(fs::is_directory(path));
		return path;
	}

	std::wstring get_user_downloads_dir_vista()
	{
		PWSTR path = nullptr;

		static auto proc = tools::win32::import_proc<decltype(&::SHGetKnownFolderPath)>(L"Shell32.dll", "SHGetKnownFolderPath");
		if (!proc)
		{
			return std::wstring();
		}

		const auto error = proc->get()(FOLDERID_Downloads, 0, nullptr, Out &path);
		if (FAILED(error))
		{
			return std::wstring();
		}

		std::wstring result(path);
		assert(fs::is_directory(result));

		::CoTaskMemFree(path);

		return result;
	}
}


bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor)
{
    bool bRetCode = false;
    LPBYTE pinfoRawData = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
    {
        WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
        major = pworkstationInfo->wki100_ver_major;
        minor = pworkstationInfo->wki100_ver_minor;
        ::NetApiBufferFree(pinfoRawData);
        bRetCode = true;
    }
    return bRetCode;
}

std::string get_os_version_string()
{
    std::string     winver;
    OSVERSIONINFOEX osver;
    SYSTEM_INFO     sysInfo;
    typedef void(__stdcall *GETSYSTEMINFO) (LPSYSTEM_INFO);

    __pragma(warning(push))
        __pragma(warning(disable:4996))
        memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx((LPOSVERSIONINFO)&osver);
    __pragma(warning(pop))
        DWORD major = 0;
    DWORD minor = 0;
    if (GetWinMajorMinorVersion(major, minor))
    {
        osver.dwMajorVersion = major;
        osver.dwMinorVersion = minor;
    }
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
    {
        OSVERSIONINFOEXW osvi;
        ULONGLONG cm = 0;
        cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMinorVersion = 3;
        if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
        {
            osver.dwMinorVersion = 3;
        }
    }

    GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
    if (getSysInfo == NULL)  getSysInfo = ::GetSystemInfo;
    getSysInfo(&sysInfo);

    if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
    if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
        &&  sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)  winver = "Windows XP x64";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
    if (osver.dwMajorVersion < 5)   winver = "unknown";

    if (osver.wServicePackMajor != 0)
    {
        std::string sp;
        char buf[128] = { 0 };
        sp = " Service Pack ";
        sprintf_s(buf, sizeof(buf), "%hd", osver.wServicePackMajor);
        sp.append(buf);
        winver += sp;
    }

    return winver;
}

CORE_TOOLS_SYSTEM_NS_END