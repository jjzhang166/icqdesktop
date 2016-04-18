#ifndef __UPLOAD_TASK_H_
#define __UPLOAD_TASK_H_

#pragma once

#include "loader_task.h"

namespace core
{
    namespace wim
    {
        class upload_progress_handler;
        class web_file_info;


        class upload_task : public loader_task, public std::enable_shared_from_this<upload_task>
        {
            std::wstring				file_name_;
            std::wstring				file_name_short_;
            std::ifstream				file_stream_;
            int64_t						file_size_;
            int64_t						bytes_sent_;

            std::string					upload_host_;
            std::string					upload_url_;

            core::tools::binary_stream	out_buffer_;

            int64_t						session_id_;

            std::string					file_url_;

            std::shared_ptr<upload_progress_handler>	handler_;

            int32_t read_data_from_file();
            int32_t send_data_to_server();

            virtual void resume(loader& _loader) override;

        public:

            upload_task(const std::string &_id, const wim_packet_params& _params, const std::wstring& _file_name);
            virtual ~upload_task();

            bool is_end();

            int32_t get_gate();
            int32_t open_file();
            int32_t send_next_range();

            std::string get_file_url() const;

            void set_handler(std::shared_ptr<upload_progress_handler> _handler);
            std::shared_ptr<upload_progress_handler> get_handler();

            virtual void on_result(int32_t _error) override;
            virtual void on_progress() override;

            std::shared_ptr<web_file_info> make_info();
        };

    }
}

#endif // !__UPLOAD_TASK_H_
