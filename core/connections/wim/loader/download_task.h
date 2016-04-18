#ifndef __WIM_DOWNLOAD_TASK_H_
#define __WIM_DOWNLOAD_TASK_H_

#pragma once

#include "loader_task.h"

namespace core
{
    namespace wim
    {
        struct wim_packet_params;
        class download_progress_handler;
        class web_file_info;

        class download_task : public loader_task, public std::enable_shared_from_this<download_task>
        {
            std::unique_ptr<web_file_info>		info_;
            std::ofstream						file_stream_;

            std::wstring						files_folder_;
            std::wstring						previews_folder_;
            std::wstring						file_name_temp_;
            std::wstring                        filename_;

            std::shared_ptr<download_progress_handler>	handler_;

            std::shared_ptr<web_file_info> make_info();

            bool get_file_id(const std::string& _file_url, std::string& _file_id) const;
            std::wstring get_info_file_name() const;

            virtual void resume(loader& _loader) override;

        public:

            // core thread functions

            download_task(
                const std::string &_id,
                const wim_packet_params& _params,
                const std::string& _file_url,
                const std::wstring& _files_folder,
                const std::wstring& _previews_folder,
                const std::wstring& _filename);

            virtual ~download_task();

            void set_handler(std::shared_ptr<download_progress_handler> _handler);
            std::shared_ptr<download_progress_handler> get_handler();

            virtual void on_result(int32_t _error) override;
            virtual void on_progress() override;

            bool is_end();

            bool load_metainfo_from_local_cache();
            bool is_downloaded_file_exists();

            void set_played(bool played);

            // download thread functions
            int32_t download_metainfo();
            int32_t open_temporary_file();
            int32_t load_next_range();
            int32_t get_preview_2k();
            int32_t get_preview();
            int32_t on_finish();
            int32_t copy_if_needed();

        };

    }
}

#endif //__WIM_DOWNLOAD_TASK_H_