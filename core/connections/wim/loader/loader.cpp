#include "stdafx.h"

#include "../../../async_task.h"
#include "../wim_packet.h"
#include "upload_task.h"
#include "download_task.h"
#include "loader_handlers.h"
#include "loader_errors.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "snap_metainfo.h"
#include "web_file_info.h"
#include "../../../http_request.h"
#include "../../../tools/md5.h"
#include "../../../tools/system.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../../utils.h"
#include "../../../log/log.h"
#include "../../../disk_cache/disk_cache.h"
#include "../../../profiling/profiler.h"

#include "image_download_task.h"
#include "image_preview_download_task.h"
#include "link_metainfo_download_task.h"
#include "snap_metainfo_download_task.h"
#include "tasks_runner_slot.h"

#include "loader.h"

using namespace core;
using namespace wim;

namespace
{
    bool is_suspendable_error(const loader_errors _error);
}

struct loader::tasks_runner : boost::noncopyable
{
    tasks_runner(const int32_t _threads_num);

    async_executer_uptr runner_;

    atomic_bool runner_active_;

    std::list<loader_task_sptr> tasks_;

    std::list<loader_task_sptr> suspended_tasks_;

    loader_task_sptr current_task_;
};

loader::tasks_runner::tasks_runner(const int32_t _threads_num)
    : runner_(new async_executer(_threads_num))
    , runner_active_(false)
{
}

loader::loader(const std::wstring &_cache_dir)
    : cache_(disk_cache::disk_cache::make(_cache_dir))
    , file_sharing_threads_(new async_executer(1))
{
    initialize_tasks_runners();
}

loader::~loader()
{
}

void loader::add_file_sharing_task(std::shared_ptr<fs_loader_task> _task)
{
    assert(_task);

    const auto &task_id = _task->get_id();
    assert(!task_id.empty());

    auto iter_task = find_task_by_id(file_sharing_tasks_, task_id);
    if (iter_task != file_sharing_tasks_.end())
    {
        assert(!"task with the same id already exist");
        return;
    }

    file_sharing_tasks_.push_back(_task);
}

void loader::remove_file_sharing_task(const std::string &_id)
{
    assert(!_id.empty());

    auto iter_task = find_task_by_id(file_sharing_tasks_, _id);
    if (iter_task == file_sharing_tasks_.end())
    {
        return;
    }

    file_sharing_tasks_.erase(iter_task);
}

loader::const_fs_tasks_iter loader::find_task_by_id(const fs_tasks_queue &_tasks, const std::string &_id)
{
    auto task_iter = std::find_if(
        _tasks.cbegin(),
        _tasks.cend(),
        [&_id]
        (const fs_task_sptr &_item)
        {
            assert(_item);
            return (_item->get_id() == _id);
        });

    return task_iter;
}

void loader::on_file_sharing_task_result(std::shared_ptr<fs_loader_task> _task, int32_t _error)
{
    _task->on_result(_error);

    remove_file_sharing_task(_task->get_id());
}

void loader::on_file_sharing_task_progress(std::shared_ptr<fs_loader_task> _task)
{
    _task->on_progress();
}

bool loader::has_file_sharing_task(const std::string &_id)
{
    assert(!_id.empty());

    return (find_task_by_id(file_sharing_tasks_, _id) != file_sharing_tasks_.cend());
}

void loader::raise_task_priority(const int64_t _task_id)
{
    assert(_task_id > 0);
    assert(g_core->is_core_thread());

    for (auto &p : tasks_runners_)
    {
        auto &runner = *p.second;

        auto &tasks = runner.tasks_;

        auto iter = tasks.begin();

        while (iter != tasks.end())
        {
            const auto is_different_id = ((*iter)->get_id() != _task_id);
            if (is_different_id)
            {
                ++iter;
                continue;
            }

            const auto is_first = (iter == tasks.begin());
            if (!is_first)
            {
                tasks.emplace_front(std::move(*iter));
                iter = tasks.erase(iter);
            }

            return;
        }
    }
}

void loader::raise_contact_tasks_priority(const std::string &_contact_aimid)
{
    assert(!_contact_aimid.empty());
    assert(g_core->is_core_thread());

    __TRACE(
        "prefetch",
        "raising contact tasks priority\n"
        "    prev_priority=<%1%>\n"
        "    next_priority=<%2%>",
        priority_contact_ % _contact_aimid);

    priority_contact_ = _contact_aimid;

    for (auto &p : tasks_runners_)
    {
        auto &runner = *p.second;
        auto &tasks = runner.tasks_;

        if (tasks.empty())
        {
            continue;
        }

        std::vector<loader_task_sptr> contact_tasks;
        contact_tasks.reserve(tasks.size());

        for (auto iter = tasks.begin(); iter != tasks.end(); )
        {
            const auto is_different_contact = ((*iter)->get_contact_aimid() != _contact_aimid);
            if (is_different_contact)
            {
                ++iter;
                continue;
            }

            auto &task = *iter;

            contact_tasks.emplace_back(std::move(task));

            iter = tasks.erase(iter);
        }

        if (contact_tasks.empty())
        {
            continue;
        }

        __TRACE(
            "prefetch",
            "contact tasks priority raised\n"
            "    count=<%1%>\n"
            "    runner_slot=<%2%>",
            contact_tasks.size() % p.first);

        tasks.insert(
            tasks.begin(),
            std::make_move_iterator(contact_tasks.begin()),
            std::make_move_iterator(contact_tasks.end()));
    }
}

void loader::resume_file_sharing_tasks()
{
    for (const auto &task : file_sharing_tasks_)
    {
        if (task->get_last_error() != 0)
        {
            task->set_last_error(0);
            task->resume(*this);
        }
    }
}

void loader::resume_suspended_tasks(const wim_packet_params& _params)
{
    assert(g_core->is_core_thread());

    for (const auto slot : get_all_tasks_runner_slots())
    {
        auto &runner = *tasks_runners_[slot];

        auto &suspended_tasks = runner.suspended_tasks_;

        if (suspended_tasks.empty())
        {
            continue;
        }

        const auto user_proxy = g_core->get_user_proxy_settings();

        for (auto &task : suspended_tasks)
        {
            assert(task);

            task->on_before_resume(_params, user_proxy, false);

            add_task(std::move(task));
        }

        suspended_tasks.clear();
    }
}

void loader::set_played(const std::string& _file_url, const std::wstring& _previews_folder, bool _played, const wim_packet_params& _params)
{
    auto task = std::make_shared<download_task>(
        "0",
        _params,
        _file_url,
        std::wstring(),
        std::wstring(),
        _previews_folder);

    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task, _played]
        {
            const auto task = wr_task.lock();
            if (!task)
                return 0;

            task->set_played(_played);

            return 0;
        }
    )->on_result_ =
        [wr_this, wr_task](int32_t _error)
        {
            const auto task = wr_task.lock();
            if (!task)
                return;

            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->remove_file_sharing_task(task->get_id());
        };
}

void loader::send_task_ranges_async(std::weak_ptr<upload_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    file_sharing_threads_->run_async_function(
        [_wr_task]
        {
            auto task = _wr_task.lock();
            if (!task)
                return -1;

            return (int32_t)task->send_next_range();
        }
    )->on_result_ =
        [wr_this, _wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = _wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                task->set_last_error(_error);

                if (_error != (int32_t)loader_errors::network_error)
                    ptr_this->on_file_sharing_task_result(task, _error);

                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            if (!task->is_end())
            {
                ptr_this->send_task_ranges_async(task);
            }
            else
            {
                ptr_this->on_file_sharing_task_result(task, 0);
                g_core->insert_event(core::stats::stats_event_names::filesharing_sent_success);
            }
        };
}


void loader::load_file_sharing_task_ranges_async(std::weak_ptr<download_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    file_sharing_threads_->run_async_function(
        [_wr_task]
        {
            auto task = _wr_task.lock();
            if (!task)
                return -1;

            auto err = task->load_next_range();

            if (err == loader_errors::success)
            {
                if (task->is_end())
                    err = task->on_finish();
            }

            return (int32_t)err;

        }
    )->on_result_ =
        [wr_this, _wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = _wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                task->set_last_error(_error);

                if (_error != (int32_t)loader_errors::network_error)
                {
                    ptr_this->on_file_sharing_task_result(task, _error);
                }

                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            if (!task->is_end())
            {
                ptr_this->load_file_sharing_task_ranges_async(task);
            }
            else
            {
                ptr_this->on_file_sharing_task_result(task, 0);
            }
        };
}

std::shared_ptr<upload_progress_handler> loader::upload_file_sharing(
    const std::string &_guid,
    const std::wstring& _file_name,
    const wim_packet_params& _params)
{
    assert(!_guid.empty());
    assert(!_file_name.empty());

    auto task = std::make_shared<upload_task>(_guid, _params, _file_name);
    task->set_handler(std::make_shared<upload_progress_handler>());
    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<upload_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task]
        {
            auto task = wr_task.lock();
            if (!task)
                return -1;

            return (int32_t)task->open_file();

        }
    )->on_result_ =
        [wr_this, wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                ptr_this->on_file_sharing_task_result(task, _error);
                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            ptr_this->file_sharing_threads_->run_async_function(
                [wr_task]
                {
                    auto task = wr_task.lock();
                    if (!task)
                        return (int32_t)loader_errors::undefined;

                    return (int32_t)task->get_gate();
                }
            )->on_result_ =
                [wr_this, wr_task](int32_t _error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    auto task = wr_task.lock();
                    if (!task)
                        return;

                    if (_error != 0)
                    {
                        ptr_this->on_file_sharing_task_result(task, _error);
                        return;
                    }

                    ptr_this->on_file_sharing_task_progress(task);

                    ptr_this->send_task_ranges_async(task);
                };
        };

    return task->get_handler();
}


std::shared_ptr<download_progress_handler> loader::download_file_sharing(
    const int64_t _seq,
    const std::string& _file_url,
    const file_sharing_function _function,
    const std::wstring& _files_folder,
    const std::wstring& _previews_folder,
    const std::wstring& _filename,
    const wim_packet_params& _params)
{
    assert(_function > file_sharing_function::min);
    assert(_function < file_sharing_function::max);
    assert(_function != file_sharing_function::download_preview_metainfo);

    auto task = std::make_shared<download_task>(
        std::to_string(_seq),
        _params,
        _file_url,
        _files_folder,
        _previews_folder,
        _filename
    );
    task->set_handler(std::make_shared<download_progress_handler>());
    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task, _function]
        {
            const auto task = wr_task.lock();
            if (!task)
            {
                return 0;
            }

            // return -1 (failure) if there are no metainfo file or there are no downloaded fs link in the cache

            if (!task->load_metainfo_from_local_cache())
            {
                return -1;
            }

            const auto check_file_existence = (
                (_function == file_sharing_function::download_file) ||
                (_function == file_sharing_function::check_local_copy_exists));
            if (check_file_existence && !task->is_downloaded_file_exists())
            {
                return -1;
            }

            return 0;
        }
    )->on_result_ =
        [wr_this, wr_task, _function](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            const auto is_local_copy_test_mode = (_function == file_sharing_function::check_local_copy_exists);
            const auto is_file_and_metainfo_ready = (_error == 0);
            if (is_file_and_metainfo_ready || is_local_copy_test_mode)
            {
                task->copy_if_needed();
                ptr_this->on_file_sharing_task_result(task, _error);
                return;
            }

            ptr_this->file_sharing_threads_->run_async_function(
                [wr_task]
                {
                    auto task = wr_task.lock();
                    if (!task)
                    {
                        return (int32_t)loader_errors::orphaned;
                    }

                    return (int32_t)task->download_metainfo();
                }
            )->on_result_ =
                [wr_this, wr_task, _function](int32_t _error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    auto task = wr_task.lock();
                    if (!task)
                        return;

                    const auto is_download_file_metainfo_mode = (_function == file_sharing_function::download_file_metainfo);

                    const auto success = (_error == 0);

                    if (success && is_download_file_metainfo_mode)
                    {
                        task->serialize_metainfo();
                    }

                    if (!success || is_download_file_metainfo_mode)
                    {
                        ptr_this->on_file_sharing_task_result(task, _error);
                        return;
                    }

                    g_core->insert_event(stats::stats_event_names::filesharing_download_success);

                    ptr_this->on_file_sharing_task_progress(task);

                    ptr_this->file_sharing_threads_->run_async_function(
                        [wr_task]
                        {
                            auto task = wr_task.lock();
                            if (!task)
                                return -1;

                            return (int32_t)task->open_temporary_file();

                        }
                    )->on_result_ =
                        [wr_this, wr_task]
                        (int32_t _error)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            auto task = wr_task.lock();
                            if (!task)
                                return;

                            if (_error != 0)
                            {
                                ptr_this->on_file_sharing_task_result(task, _error);
                                return;
                            }

                            ptr_this->load_file_sharing_task_ranges_async(task);
                        };

                };
        };

    return task->get_handler();
}


std::shared_ptr<get_file_direct_uri_handler> loader::get_file_direct_uri(const int64_t _seq, const std::string& _file_url, const wim_packet_params& _params)
{
    auto handler = std::make_shared<get_file_direct_uri_handler>();

    auto url = std::make_shared<std::string>();

    file_sharing_threads_->run_async_function([url, _params, _file_url]()->int32_t
    {
        int32_t error = -1;

        download_task task(tools::system::generate_guid(), _params, _file_url, L"", L"", L"");

        if (task.download_metainfo() == loader_errors::success)
        {
            error = 0;

            *url = task.get_file_direct_url();
        }

        return error;
    })->on_result_ = [handler, url](int32_t _error)
    {
        handler->on_result(_error, *url);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> loader::download_file(
    const std::string& _file_url,
    const std::wstring& _file_name,
    const bool _keep_alive,
    const wim_packet_params& _params,
    http_request_simple::progress_function _progress_func)
{
    auto handler = std::make_shared<async_task_handlers>();

    auto user_proxy = g_core->get_user_proxy_settings();

    file_sharing_threads_->run_async_function(
        [_params, _file_url, _file_name, _keep_alive, user_proxy, _progress_func]
        {
            core::http_request_simple request(user_proxy, utils::get_user_agent(), _params.stop_handler_, _progress_func);

            request.set_url(_file_url);
            request.set_need_log(false);
            if (_keep_alive)
                request.set_keep_alive();

            if (!request.get() || request.get_response_code() != 200)
                return (int32_t)loader_errors::network_error;

            if (!request.get_response()->save_2_file(_file_name))
                return (int32_t)loader_errors::save_2_file;

            return (int32_t)loader_errors::success;
        }
    )->on_result_ =
        [handler](int32_t _error)
        {
            if (handler->on_result_)
                handler->on_result_(_error);
        };

    return handler;
}

std::shared_ptr<download_image_handler> loader::download_image(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const std::string& _image_uri,
    const std::wstring& _cache_dir,
    const std::wstring& _forced_local_path,
    const bool _sign_uri,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(!_image_uri.empty());

    auto handler = std::make_shared<download_image_handler>();
    auto user_proxy = g_core->get_user_proxy_settings();

    const auto &image_local_path =
        _forced_local_path.empty() ?
            get_path_in_cache(_cache_dir, _image_uri, path_type::file) :
            _forced_local_path;
    assert(!image_local_path.empty());

    std::unique_ptr<image_download_task> task(new image_download_task(
        _seq,
        _contact_aimid,
        _params,
        g_core->get_user_proxy_settings(),
        _image_uri,
        image_local_path,
        _cache_dir,
        cache_,
        _sign_uri,
        handler));

    add_task(std::move(task));

    return handler;
}

void loader::add_task(loader_task_sptr _task)
{
    assert(_task);
    assert(g_core->is_core_thread());

    const auto task_slot = _task->get_slot();
    assert(task_slot > tasks_runner_slot::min);
    assert(task_slot < tasks_runner_slot::max);

    auto &runner = *tasks_runners_[task_slot];

    __TRACE(
        "snippets",
        "added task in runner queue\n"
        "    id=<%1%>\n"
        "    slot=<%2%>\n"
        "    queue_size=<%3%> (before)\n"
        "    tasks_runner_active=<%4%>",
        _task->get_id() %
        task_slot %
        runner.tasks_.size() %
        logutils::yn(runner.runner_active_));

    const auto is_priority_contact_task = (_task->get_contact_aimid() == priority_contact_);

    if (is_priority_contact_task)
    {
        auto insert_pos = runner.tasks_.begin();

        while (insert_pos != runner.tasks_.end())
        {
            const auto &pos_task = **insert_pos++;

            const auto is_priority_contact_pos = (pos_task.get_contact_aimid() != priority_contact_);
            if (is_priority_contact_pos)
            {
                break;
            }
        }

        runner.tasks_.emplace(insert_pos, std::move(_task));
    }
    else
    {
        runner.tasks_.emplace_back(std::move(_task));
    }

    if (runner.runner_active_)
    {
        return;
    }

    runner.runner_active_ = true;

    run_next_task(task_slot);
}

void loader::initialize_tasks_runners()
{
    assert(tasks_runners_.empty());

    for (const auto slot : get_all_tasks_runner_slots())
    {
        tasks_runners_.emplace(
            slot,
            tasks_runner_uptr(new tasks_runner(1)));
    }
}

void loader::run_next_task(const tasks_runner_slot _slot)
{
    assert(_slot > tasks_runner_slot::min);
    assert(_slot < tasks_runner_slot::max);

    auto &runner = *tasks_runners_[_slot];

    assert(!runner.current_task_);
    assert(runner.runner_active_);
    assert(g_core->is_core_thread());

    if (runner.tasks_.empty())
    {
        __TRACE(
            "snippets",
            "tasks runner stopped\n"
            "    slot=<%1%>",
            _slot);

        runner.current_task_.reset();
        runner.runner_active_ = false;
        return;
    }

    runner.current_task_ = std::move(runner.tasks_.front());
    runner.tasks_.pop_front();

    __TRACE(
        "snippets",
        "task sent to thread pool\n"
        "    id=<%1%>\n"
        "    slot=<%2%>",
        runner.current_task_->get_id() % _slot);

    std::weak_ptr<loader> wr_this = shared_from_this();

    auto error = std::make_shared<loader_errors>(loader_errors::undefined);

    auto task = runner.current_task_;

    runner.runner_->run_async_function(
        [error, task]
        {
            assert(error);

            *error = task->run();

            return 0;
        }
    )->on_result_ = [wr_this, error, _slot]
        (int32_t)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            auto &runner = *ptr_this->tasks_runners_[_slot];

            auto &current_task = runner.current_task_;

            assert(current_task);
            assert(runner.runner_active_);
            assert(error);
            assert(*error != loader_errors::undefined);

            if (is_suspendable_error(*error))
            {
                current_task->on_before_suspend();
                runner.suspended_tasks_.emplace_back(std::move(current_task));
                assert(!current_task);
            }
            else
            {
                current_task->on_result(*error);
                current_task.reset();
            }

            ptr_this->run_next_task(_slot);
        };
}

void loader::resume_task(
    const int64_t _id,
    const wim_packet_params &_wim_params)
{
    assert(_id > 0);
    assert(g_core->is_core_thread());

    for (auto &p : tasks_runners_)
    {
        auto &runner = *p.second;

        auto &suspended_tasks = runner.suspended_tasks_;

        auto task_iter = std::find_if(
            suspended_tasks.cbegin(),
            suspended_tasks.cend(),
            [&_id]
            (const loader_task_sptr &_item)
            {
                assert(_item);
                return (_item->get_id() == _id);
            });

        const auto is_task_found = (task_iter != suspended_tasks.end());
        if (!is_task_found)
        {
            continue;
        }

        auto task = *task_iter;
        assert(task);

        suspended_tasks.erase(task_iter);

        add_task(std::move(task));

        const auto user_proxy = g_core->get_user_proxy_settings();
        task->on_before_resume(_wim_params, user_proxy, true);

        return;
    }

    assert(!"attempting to resume missing task");
}

bool loader::cancel_task(
    const int64_t _id,
    tasks_runner &_runner)
{
    assert(_id > 0);

    auto task_cancelled = false;

    auto &current_task = _runner.current_task_;
    const auto cancel_current_task = (current_task && (current_task->get_id() == _id));
    if (cancel_current_task)
    {
        current_task->cancel();

        task_cancelled = true;
    }

    auto &tasks = _runner.tasks_;

    auto iter = tasks.begin();
    while (iter != tasks.end())
    {
        const auto &task = **iter;

        const auto is_same_id = (task.get_id() == _id);
        if (is_same_id)
        {
            iter = tasks.erase(iter);

            task_cancelled = true;
        }
        else
        {
            ++iter;
        }
    }

    return task_cancelled;
}

std::shared_ptr<download_image_handler> loader::download_image_preview(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const std::string& _image_uri,
    const int32_t _preview_width_max,
    const int32_t _preview_height_max,
    const std::wstring& _cache_dir,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(!_image_uri.empty());
    assert(!_cache_dir.empty());
    assert(_preview_height_max >= 0);
    assert(_preview_height_max < 1000);
    assert(_preview_width_max >= 0);
    assert(_preview_width_max < 1000);

    auto handler = std::make_shared<download_image_handler>();

    std::unique_ptr<image_preview_download_task> task(
        new image_preview_download_task(
            _seq,
            _contact_aimid,
            _params,
            g_core->get_user_proxy_settings(),
            _image_uri,
            _cache_dir,
            cache_,
            _preview_width_max,
            _preview_height_max,
            handler));

    add_task(std::move(task));

    return handler;
}

std::shared_ptr<download_link_metainfo_handler> loader::download_link_metainfo(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const std::string& _url,
    const std::wstring& _cache_dir,
    const bool _sign_url,
    const wim_packet_params& _params)
{
    assert(!_contact_aimid.empty());

    auto handler = std::make_shared<download_link_metainfo_handler>();

    std::unique_ptr<link_metainfo_download_task> task(
        new link_metainfo_download_task(
            _seq,
            _contact_aimid,
            _params,
            g_core->get_user_proxy_settings(),
            _url,
            _cache_dir,
            cache_,
            _sign_url,
            handler));

    add_task(std::move(task));

    return handler;
}

std::shared_ptr<download_snap_metainfo_handler> loader::download_snap_metainfo(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const std::string &_ttl_id,
    const std::wstring& _cache_dir,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(!_ttl_id.empty());
    assert(!_cache_dir.empty());

    auto handler = std::make_shared<download_snap_metainfo_handler>();

    std::unique_ptr<snap_metainfo_download_task> task(
        new snap_metainfo_download_task(
            _seq,
            _contact_aimid,
            _params,
            g_core->get_user_proxy_settings(),
            _ttl_id,
            _cache_dir,
            cache_,
            handler));

    add_task(std::move(task));

    return handler;
}

void loader::cancel_task(const int64_t _seq)
{
    assert(_seq > 0);

    for (const auto slot : get_all_tasks_runner_slots())
    {
        auto &runner = *tasks_runners_[slot];

        cancel_task(_seq, runner);
    }
}

void loader::abort_file_sharing_process(const std::string &_process_id)
{
    assert(!_process_id.empty());

    remove_file_sharing_task(_process_id);
}

namespace
{
    bool is_suspendable_error(const loader_errors _error)
    {
        return (_error == loader_errors::network_error) ||
               (_error == loader_errors::suspend);
    }
}
