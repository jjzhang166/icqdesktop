#ifndef __LOADER_HANDLERS_H_
#define __LOADER_HANDLERS_H_

#pragma once

namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    struct icollection;

    namespace wim
    {
        class web_file_info;

        class upload_progress_handler
        {
        public:

            std::function<void(int32_t, const web_file_info& _info)>	on_result;
            std::function<void(const web_file_info& _info)>			on_progress;
        };


        class download_progress_handler
        {
        public:

            std::function<void(int32_t, const web_file_info& _info)>	on_result;
            std::function<void(const web_file_info& _info)>			on_progress;
        };

        class download_image_handler
        {
        public:

            std::function<void(int32_t _error, std::shared_ptr<core::tools::binary_stream>, const std::string&)>	on_result;

        };
    }
}


#endif // __LOADER_HANDLERS_H_