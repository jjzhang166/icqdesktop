#pragma once

namespace core
{
    class async_executer;
    struct async_task_handlers;
    enum class file_sharing_function;

    namespace wim
    {
        struct wim_packet_params;
        class loader_task;
        class upload_task;
        class download_task;
        class upload_progress_handler;
        class download_progress_handler;
        class download_image_handler;

        typedef std::map<std::string, std::shared_ptr<loader_task>> loader_tasks_map;

        class loader : public std::enable_shared_from_this<loader>
        {
            std::unique_ptr<async_executer> threads_;

            loader_tasks_map tasks_;

            void add_task(std::shared_ptr<loader_task> _task);
            void remove_task(std::shared_ptr<loader_task> _task);
            void remove_task(const std::string &_id);
            void on_task_result(std::shared_ptr<loader_task> _task, int32_t _error);
            void on_task_progress(std::shared_ptr<loader_task> _task);

            std::shared_ptr<download_image_handler> download_image_file(
                const int64_t _seq,
                const std::string& _image_url,
                const std::wstring& _local_path,
                const bool _sign_url,
                const wim_packet_params& _params);

            std::shared_ptr<download_image_handler> download_preview(
                const int64_t _seq,
                const std::string& _image_url,
                const std::wstring& _cache_dir,
                const std::wstring& _local_path,
                const int32_t _preview_height,
                const wim_packet_params& _params);

        public:

            void send_task_ranges_async(std::weak_ptr<upload_task> _wr_task);
            void load_task_ranges_async(std::weak_ptr<download_task> _wr_task);

            std::shared_ptr<upload_progress_handler> upload_file_sharing(
                const std::string &_guid,
                const std::wstring& _file_name,
                const wim_packet_params& _params);

            std::shared_ptr<download_progress_handler> download_file_sharing(
                int64_t _seq,
                const std::string& _file_url,
                const file_sharing_function _function,
                const std::wstring& _files_folder,
                const std::wstring& _previews_folder,
                const std::wstring& _filename,
                const wim_packet_params& _params);

            std::shared_ptr<async_task_handlers> download_file(
                const std::string& _file_url,
                const std::wstring& _file_name,
                const bool _keep_alive,
                const wim_packet_params& _params);

            std::shared_ptr<download_image_handler> download_image(
                const int64_t _seq,
                const std::string& _image_url,
                const std::wstring& _cache_dir,
                const std::wstring& _forced_local_path,
                const bool _sign_url,
                const bool _download_preview,
                const int32_t _preview_height,
                const wim_packet_params& _params);

            void abort_process(const std::string &_process_id);

            bool has_task(const std::string &_id);

            void resume();

            void setPlayed(const std::string& _file_url, const std::wstring& previews_folder, bool played, const wim_packet_params& _params);

            loader();
            virtual ~loader();
        };
    }
}
