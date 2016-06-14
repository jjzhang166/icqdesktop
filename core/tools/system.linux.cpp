#include "stdafx.h"
#include "system.h"

bool core::tools::system::is_dir_writable(const std::wstring &_dir_path_str)
{
    return true;
}

std::string core::tools::system::generate_guid()
{
    std::string guid_string;

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    guid_string = boost::lexical_cast<std::string>(uuid);

    return guid_string;
}

std::string core::tools::system::generate_internal_id()
{
    static int internal_id = 0;
    std::stringstream guid_string;

    guid_string << generate_guid() << "-" << ++internal_id; 

    return guid_string.str();
}

bool core::tools::system::move_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    boost::filesystem::path from(_old_file);
    boost::filesystem::path target(_new_file);
    boost::system::error_code error;
    boost::filesystem::rename(from, target, error);

    if (!error)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool core::tools::system::copy_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    boost::filesystem::path from(_old_file);
    boost::filesystem::path target(_new_file);
    boost::system::error_code error;
    boost::filesystem::copy(from, target, error);

    if (!error)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool core::tools::system::compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2)
{
    if (_dir1.empty() || _dir2.empty())
        return false;

    boost::system::error_code error;
    return boost::filesystem::equivalent(boost::filesystem::path(_dir1), boost::filesystem::path(_dir2), error);
}

std::wstring core::tools::system::get_file_directory(const std::wstring& file)
{
    boost::filesystem::wpath p(file);
    return p.parent_path().wstring();
}

std::wstring core::tools::system::get_file_name(const std::wstring& file)
{
    boost::filesystem::wpath p(file);
    return p.filename().wstring();
}

std::wstring core::tools::system::get_user_profile()
{
    std::string result = getenv("HOME");

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(result);

    return wide;
}

unsigned long core::tools::system::get_current_thread_id()
{
    std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long threadNumber = 0;
    sscanf(threadId.c_str(), "%lx", &threadNumber);
    return threadNumber;
}

std::wstring core::tools::system::get_user_downloads_dir()
{
    std::string result = getenv("HOME");
    result.append("/Downloads");

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(result);

    return wide;
}

std::string core::tools::system::to_upper(std::string str)
{
    std::string upper = str;
    boost::to_upper(upper);
    return upper;
}

std::string core::tools::system::get_os_version_string()
{
    // TODO : use actual value here
    return "Linux";
}
