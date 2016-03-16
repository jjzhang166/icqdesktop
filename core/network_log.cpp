#include "stdafx.h"
#include "network_log.h"
#include "async_task.h"

namespace core
{
    const int64_t max_file_size = 1024*1024*10;
    const int64_t max_logs_size = max_file_size*5;

    network_log::network_log(const boost::filesystem::wpath& _logs_directory)
        :   write_thread_(new async_executer()),
            file_context_(new log_file_context(_logs_directory))
    {
    }

    network_log::~network_log()
    {
        write_thread_.reset();
    }

    bool create_logs_directory(const boost::filesystem::wpath& _logs_directory)
    {
        if (!boost::filesystem::is_directory(_logs_directory))
        {
            if (!boost::filesystem::create_directories(_logs_directory))
                return false;
        }

        return true;
    }

    int64_t get_log_index(const boost::filesystem::wpath& _logs_directory)
    {
        using namespace boost::xpressive;

        static auto re = sregex::compile("(?P<index>\\d+)\\.net\\.txt");

        int64_t max_index = -1;

        boost::filesystem::directory_iterator dir_entry(_logs_directory);
        boost::filesystem::directory_iterator dir_end;
        for (; dir_entry != dir_end; ++dir_entry )
        {
            const auto file_name_path = dir_entry->path().filename();
            const auto file_name_raw = file_name_path.c_str();
#ifdef _WIN32
            const std::string filename = tools::from_utf16(file_name_raw);
#else
            const std::string filename = file_name_raw;
#endif
            smatch match;
            if (!regex_match(filename, match, re))
            {
                continue;
            }

            const auto &index_str = match["index"].str();
            const auto index = boost::lexical_cast<int64_t>(index_str);
            assert(index >= 0);
            max_index = std::max(max_index, index);
        }

        return (max_index + 1);
    }

    boost::filesystem::wpath get_file_path(int64_t _index, const boost::filesystem::wpath& _logs_directory)
    {
        boost::wformat area_filename(L"%016lld.%s.%s");
        area_filename % _index % L"net" % L"txt";

        boost::filesystem::wpath path = _logs_directory;
        path.append(area_filename.str());

        return path;
    }

    void clean_logs(const boost::filesystem::wpath& _logs_directory)
    {
        using namespace boost::xpressive;

        static auto re = sregex::compile("(?P<index>\\d+)\\.net\\.txt");

        int64_t max_index = -1;

        std::map<int64_t, std::string> files;

        boost::filesystem::directory_iterator dir_entry(_logs_directory);
        boost::filesystem::directory_iterator dir_end;
        for (; dir_entry != dir_end; ++dir_entry )
        {
            const auto file_name_path = dir_entry->path().filename();
            const auto file_name_raw = file_name_path.c_str();
#ifdef _WIN32
            const std::string filename = tools::from_utf16(file_name_raw);
#else
            const std::string filename = file_name_raw;
#endif
            smatch match;
            if (!regex_match(filename, match, re))
            {
                continue;
            }

            const auto &index_str = match["index"].str();
            const auto index = boost::lexical_cast<int64_t>(index_str);

            files[index] = filename;
        }

        int64_t size = 0;

        for (auto iter = files.rbegin(); iter != files.rend(); ++iter)
        {
            boost::filesystem::wpath file_path = _logs_directory;
            file_path.append(tools::from_utf8(iter->second));

            if (boost::filesystem::exists(file_path))
            {
                if (size > max_logs_size)
                {
                    boost::filesystem::remove(file_path);
                    continue;
                }

                size += boost::filesystem::file_size(file_path);
            }
        }
    }

    void network_log::write_data(const tools::binary_stream& _data)
    {
        if (!_data.available())
        {
            assert(false);
            return;
        }

        const auto current_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        const auto current_thread_id = boost::this_thread::get_id();

        auto bs_data = std::make_shared<tools::binary_stream>(_data);
        auto file_context = file_context_;
        auto file_names_history = &file_names_history_;

        write_thread_->run_async_function([current_time, current_thread_id, bs_data, file_context, file_names_history]()->int32_t
        {
            if (!file_context->file_stream_)
            {
                if (file_context->file_index_ < 0)
                {
                    if (!create_logs_directory(file_context->logs_directory_))
                        return -1;

                    file_context->file_index_ = get_log_index(file_context->logs_directory_);
                }
                else
                {
                    ++file_context->file_index_;
                }
                
                std::ios_base::openmode open_mode = std::fstream::binary | std::fstream::out | std::fstream::app;

                const auto file_path = get_file_path(file_context->file_index_, file_context->logs_directory_);

                file_context->file_stream_.reset(new boost::filesystem::ofstream(file_path, open_mode));
                if (!file_context->file_stream_->good())
                {
                    file_context->file_stream_.reset();
                    return -1;
                }
                else
                {
                    file_names_history->first = file_names_history->second;
                    file_names_history->second = file_path.wstring();
                }
            }

            std::stringstream ss_header;
            const auto now_c = std::chrono::system_clock::to_time_t(current_time);

            tm now_tm = { 0 };
#ifdef _WIN32
            localtime_s(&now_tm, &now_c);
#else
            localtime_r(&now_c, &now_tm);
#endif

            const auto fraction = (current_time.time_since_epoch().count() % 1000);
            ss_header << "[" << std::put_time<char>(&now_tm, "%c") << "." << fraction << "].[" << current_thread_id << "] \n";
            
            uint32_t data_size = bs_data->available();

            file_context->file_stream_->write((const char*) ss_header.str().c_str(), ss_header.str().length());
            file_context->file_stream_->write((const char*) bs_data->read(data_size), data_size);
            file_context->file_stream_->write((const char*) "\n", sizeof(char));
                        
            file_context->file_stream_->flush();

            auto file_size = file_context->file_stream_->tellp();
            if (file_size > max_file_size)
            {
                file_context->file_stream_->close();
                file_context->file_stream_.reset();

                clean_logs(file_context->logs_directory_);
            }

            return 0;
        });
    }



}
