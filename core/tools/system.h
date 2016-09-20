#pragma once

#define CORE_TOOLS_SYSTEM_NS_BEGIN namespace core { namespace tools { namespace system {
#define CORE_TOOLS_SYSTEM_NS_END } } }

CORE_TOOLS_SYSTEM_NS_BEGIN

bool is_dir_writable(const std::wstring &_dir_path_str);

bool delete_file(const std::wstring& _file_name);

bool move_file(const std::wstring& _old_file, const std::wstring& _new_file);

bool copy_file(const std::wstring& _old_file, const std::wstring& _new_file);

bool compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2);

std::wstring get_file_directory(const std::wstring& file);

std::wstring get_file_name(const std::wstring& file);

std::wstring create_temp_file_path();

#ifndef _WIN32
std::wstring get_user_profile();
#endif // WIN32

std::string generate_guid();

std::string generate_internal_id();

unsigned long get_current_thread_id();

std::wstring get_user_downloads_dir();

std::string to_upper(std::string str);

size_t get_memory_size_mb();

bool is_exist(const std::wstring& path);

bool is_exist(const boost::filesystem::wpath & path);

bool create_directory(const std::wstring& path);

bool create_directory(const boost::filesystem::wpath& path);

bool create_empty_file(const std::wstring &_path);

std::string get_os_version_string();

CORE_TOOLS_SYSTEM_NS_END