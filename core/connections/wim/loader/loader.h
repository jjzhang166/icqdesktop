#pragma once

#include "../../../namespaces.h"
#include "../../../http_request.h"

CORE_NS_BEGIN

class async_executer;
struct async_task_handlers;
enum class file_sharing_function;

typedef std::unique_ptr<async_executer> async_executer_uptr;

CORE_NS_END

CORE_DISK_CACHE_NS_BEGIN

class disk_cache;

typedef std::shared_ptr<disk_cache> disk_cache_sptr;

CORE_DISK_CACHE_NS_END

CORE_WIM_NS_BEGIN

struct wim_packet_params;
class fs_loader_task;
class upload_task;
class download_task;
struct upload_progress_handler;
struct download_progress_handler;
struct download_image_handler;
struct download_link_metainfo_handler;
struct download_snap_metainfo_handler;
struct get_file_direct_uri_handler;
class loader_task;
enum class tasks_runner_slot;

typedef std::shared_ptr<loader_task> loader_task_sptr;

typedef std::weak_ptr<loader_task> loader_task_wptr;

class loader : public std::enable_shared_from_this<loader>
{
    struct tasks_runner;

    typedef std::shared_ptr<fs_loader_task> fs_task_sptr;

    typedef std::list<fs_task_sptr> fs_tasks_queue;

    typedef fs_tasks_queue::const_iterator const_fs_tasks_iter;

    typedef std::unique_ptr<tasks_runner> tasks_runner_uptr;

    static const_fs_tasks_iter find_task_by_id(const fs_tasks_queue &_tasks, const std::string &_id);

    fs_tasks_queue file_sharing_tasks_;

    std::unique_ptr<async_executer> file_sharing_threads_;

    std::unordered_map<tasks_runner_slot, tasks_runner_uptr> tasks_runners_;

    disk_cache::disk_cache_sptr cache_;

    std::string priority_contact_;

    void add_file_sharing_task(std::shared_ptr<fs_loader_task> _task);

    void remove_file_sharing_task(const std::string &_id);

    void on_file_sharing_task_result(std::shared_ptr<fs_loader_task> _task, int32_t _error);

    void on_file_sharing_task_progress(std::shared_ptr<fs_loader_task> _task);

    void add_task(loader_task_sptr _task);

    void initialize_tasks_runners();

    void run_next_task(const tasks_runner_slot _slot);

    void resume_task(
        const int64_t _id,
        const wim_packet_params &_wim_params);

    bool cancel_task(
        const int64_t _id,
        tasks_runner &_runner);

public:
    void send_task_ranges_async(std::weak_ptr<upload_task> _wr_task);

    void load_file_sharing_task_ranges_async(std::weak_ptr<download_task> _wr_task);

    std::shared_ptr<upload_progress_handler> upload_file_sharing(
        const std::string &_guid,
        const std::wstring& _file_name,
        const wim_packet_params& _params);

    std::shared_ptr<download_progress_handler> download_file_sharing(
        const int64_t _seq,
        const std::string& _file_url,
        const file_sharing_function _function,
        const std::wstring& _files_folder,
        const std::wstring& _previews_folder,
        const std::wstring& _filename,
        const wim_packet_params& _params);

    std::shared_ptr<get_file_direct_uri_handler>  get_file_direct_uri(
        const int64_t _seq,
        const std::string& _file_url,
        const wim_packet_params& _params);

    std::shared_ptr<async_task_handlers> download_file(
        const std::string& _file_url,
        const std::wstring& _file_name,
        const bool _keep_alive,
        const wim_packet_params& _params,
        http_request_simple::progress_function _progress_func = nullptr);

    std::shared_ptr<download_image_handler> download_image_preview(
        const int64_t _seq,
        const std::string& _contact_aimid,
        const std::string& _image_url,
        const int32_t _preview_width_max,
        const int32_t _preview_height_max,
        const std::wstring& _cache_dir,
        const wim_packet_params& _params);

    std::shared_ptr<download_image_handler> download_image(
        const int64_t _seq,
        const std::string& _contact_aimid,
        const std::string& _image_url,
        const std::wstring& _cache_dir,
        const std::wstring& _forced_local_path,
        const bool _sign_url,
        const wim_packet_params& _params);

    std::shared_ptr<download_link_metainfo_handler> download_link_metainfo(
        const int64_t _seq,
        const std::string& _contact_aimid,
        const std::string& _url,
        const std::wstring& _cache_dir,
        const bool _sign_url,
        const wim_packet_params& _params);

    std::shared_ptr<download_snap_metainfo_handler> download_snap_metainfo(
        const int64_t _seq,
        const std::string& _contact_aimid,
        const std::string &_ttl_id,
        const std::wstring& _cache_dir,
        const wim_packet_params& _params);

    void cancel_task(const int64_t _seq);

    void abort_file_sharing_process(const std::string &_process_id);

    bool has_file_sharing_task(const std::string &_id);

    void raise_task_priority(const int64_t _task_id);

    void raise_contact_tasks_priority(const std::string &_contact_aimid);

    void resume_file_sharing_tasks();

    void resume_suspended_tasks(const wim_packet_params& _params);

    void set_played(const std::string& _file_url, const std::wstring& _previews_folder, bool _played, const wim_packet_params& _params);

    loader(const std::wstring &_cache_dir);

    virtual ~loader();

};

CORE_WIM_NS_END