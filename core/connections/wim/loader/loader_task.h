#ifndef __WIM_LOADER_TASK_H_
#define __WIM_LOADER_TASK_H_

#pragma once

namespace core
{
    namespace wim
    {
        class loader;

        struct wim_packet_params;

        class loader_task
        {
            const std::string id_;
            std::unique_ptr<wim_packet_params> wim_params_;
            int32_t error_;

        public:

            const wim_packet_params& get_wim_params();

            loader_task(const std::string& _id, const wim_packet_params& _params);
            virtual ~loader_task();

            virtual void on_result(int32_t _error) = 0;
            virtual void on_progress() = 0;
            virtual void resume(loader& _loader);

            const std::string& get_id() const;
            void set_last_error(int32_t _error);
            int32_t get_last_error() const;
        };

    }
}

#endif //__WIM_LOADER_TASK_H_