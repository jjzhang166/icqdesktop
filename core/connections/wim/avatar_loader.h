#ifndef __AVATAR_LOADER_H_
#define __AVATAR_LOADER_H_

#pragma once


#include "wim_packet.h"

namespace coretools
{
    class binary_stream;
}

namespace core
{
    class async_executer;

    namespace wim
    {
        struct avatar_context
        {
            int32_t avatar_size_;
            std::string avatar_type_;
            std::string contact_;
            time_t write_time_;
            std::wstring im_data_path_;
            std::wstring avatar_file_path_;
            core::tools::binary_stream avatar_data_;
            bool avatar_exist_;
            wim_packet_params wim_params_;

            avatar_context(const wim_packet_params& _params, int32_t _avatar_size, std::string _contact, std::wstring _im_data_path) 
                :
                wim_params_(_params),
                avatar_size_(_avatar_size), 
                contact_(_contact),
                im_data_path_(_im_data_path),
                write_time_(0),
                avatar_exist_(false){}
        };

        struct avatar_load_handlers
        {
            std::function<void(std::shared_ptr<avatar_context>)> completed_;
            std::function<void(std::shared_ptr<avatar_context>)> updated_;
            std::function<void(std::shared_ptr<avatar_context>, int32_t)> failed_;

            avatar_load_handlers() : 
                completed_(nullptr), 
                updated_(nullptr), 
                failed_(nullptr)
            {
            }
        };

        class avatar_task
        {
            std::shared_ptr<avatar_context> context_;
            std::shared_ptr<avatar_load_handlers> handlers_;

        public:

            std::shared_ptr<avatar_context> get_context() const;
            std::shared_ptr<avatar_load_handlers> get_handlers() const;

            avatar_task(
                std::shared_ptr<avatar_context> _context, 
                std::shared_ptr<avatar_load_handlers> _handlers);
        };

        class avatar_loader : public std::enable_shared_from_this<avatar_loader>
        {
            std::shared_ptr<async_executer> local_thread_;
            std::shared_ptr<async_executer> server_thread_;

            std::list<std::shared_ptr<avatar_task>> failed_tasks_;

            const std::wstring get_avatar_path(const std::wstring& _im_data_path, const std::string& _contact, const std::string _avatar_type);
            const std::string get_avatar_type_by_size(int32_t _size) const;

            void load_avatar_from_server(std::shared_ptr<avatar_context> _context, std::shared_ptr<avatar_load_handlers> _handlers);

        public:

            avatar_loader();
            virtual ~avatar_loader();

            void resume();

            std::shared_ptr<avatar_load_handlers> get_contact_avatar_async(std::shared_ptr<avatar_context> _context);
        };

    }
}


#endif //__AVATAR_LOADER_H_