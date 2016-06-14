#include "stdafx.h"

#include "loader.h"
#include "../../../async_task.h"
#include "../wim_packet.h"
#include "upload_task.h"
#include "download_task.h"
#include "loader_handlers.h"
#include "loader_errors.h"
#include "preview_proxy.h"
#include "web_file_info.h"
#include "../../../http_request.h"
#include "../../../tools/md5.h"
#include "../../../tools/system.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../../utils.h"

using namespace core;
using namespace wim;

namespace
{

    std::string sign_loader_uri(const std::string &_host, const wim_packet_params &_params, const Str2StrMap &_extra = Str2StrMap());

    std::wstring build_local_path(const std::wstring& _cache_dir, const std::string& _image_url, const bool _is_preview);

}

loader::loader()
    :   threads_(new async_executer(2))
{
}


loader::~loader()
{
}

void loader::add_task(std::shared_ptr<loader_task> _task)
{
    if (tasks_.find(_task->get_id()) != tasks_.end())
    {
        assert(!"task with the same id already exist");
        return;
    }

    tasks_[_task->get_id()] = _task;
}

void loader::remove_task(std::shared_ptr<loader_task> _task)
{
    remove_task(_task->get_id());
}

void loader::remove_task(const std::string &_id)
{
    assert(!_id.empty());

    auto iter_task = tasks_.find(_id);
    if (iter_task == tasks_.end())
    {
        assert(!"task not found");
        return;
    }

    tasks_.erase(iter_task);
}

void loader::on_task_result(std::shared_ptr<loader_task> _task, int32_t _error)
{
    _task->on_result(_error);

    remove_task(_task);
}

void loader::on_task_progress(std::shared_ptr<loader_task> _task)
{
    _task->on_progress();
}

bool loader::has_task(const std::string &_id)
{
    assert(!_id.empty());

    return (tasks_.find(_id) != tasks_.end());
}

void loader::resume()
{
    for (auto iter = tasks_.begin(); iter != tasks_.end(); ++iter)
    {
        if (iter->second->get_last_error() != 0)
        {
            iter->second->set_last_error(0);
            iter->second->resume(*this);
        }
    }
}

void loader::setPlayed(const std::string& _file_url, const std::wstring& previews_folder, bool played, const wim_packet_params& _params)
{
    auto task = std::make_shared<download_task>(
        boost::lexical_cast<std::string>(0),
        _params,
        _file_url,
        std::wstring(),
        std::wstring(),
        previews_folder
        );

    add_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    threads_->run_async_function([wr_task, played]
    {
        const auto task = wr_task.lock();
        if (!task)
            return 0;

        task->set_played(played);

        return 0;
    })->on_result_ = [wr_this, wr_task](int32_t _error)
    {
        const auto task = wr_task.lock();
        if (!task)
            return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->remove_task(task);
    };
}

void loader::send_task_ranges_async(std::weak_ptr<upload_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    threads_->run_async_function([_wr_task]
    {
        auto task = _wr_task.lock();
        if (!task)
            return -1;

        return task->send_next_range();

    })->on_result_ = [wr_this, _wr_task](int32_t _error)
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

            if (_error != loader_errors::network_error)
                ptr_this->on_task_result(task, _error);

            return;
        }

        ptr_this->on_task_progress(task);

        if (!task->is_end())
        {
            ptr_this->send_task_ranges_async(task);
        }
        else
        {
            ptr_this->on_task_result(task, 0);
			g_core->insert_event(core::stats::stats_event_names::filesharing_sent_success);
        }
    };
}


void loader::load_task_ranges_async(std::weak_ptr<download_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    threads_->run_async_function([_wr_task]
    {
        auto task = _wr_task.lock();
        if (!task)
            return -1;

        int32_t err = task->load_next_range();

        if (err == 0)
        {
            if (task->is_end())
                err = task->on_finish();
        }

        return err;

    })->on_result_ = [wr_this, _wr_task](int32_t _error)
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

            if (_error != loader_errors::network_error)
                ptr_this->on_task_result(task, _error);

            return;
        }

        ptr_this->on_task_progress(task);

        if (!task->is_end())
        {
            ptr_this->load_task_ranges_async(task);
        }
        else
        {
            ptr_this->on_task_result(task, 0);
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
    add_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<upload_task> wr_task = task;

    threads_->run_async_function([wr_task]()->int32_t
    {
        auto task = wr_task.lock();
        if (!task)
            return -1;

        return task->open_file();

    })->on_result_ = [wr_this, wr_task](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto task = wr_task.lock();
        if (!task)
            return;

        if (_error != 0)
        {
            ptr_this->on_task_result(task, _error);
            return;
        }

        ptr_this->on_task_progress(task);

        ptr_this->threads_->run_async_function([wr_task]
        {
            auto task = wr_task.lock();
            if (!task)
                return -1;

            return task->get_gate();

        })->on_result_ = [wr_this, wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                ptr_this->on_task_result(task, _error);
                return;
            }

            ptr_this->on_task_progress(task);

            ptr_this->send_task_ranges_async(task);
		};
	};

    return task->get_handler();
}


std::shared_ptr<download_progress_handler> loader::download_file_sharing(
    int64_t _seq,
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
        boost::lexical_cast<std::string>(_seq),
        _params,
        _file_url,
        _files_folder,
        _previews_folder,
        _filename
    );
    task->set_handler(std::make_shared<download_progress_handler>());
    add_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    threads_->run_async_function([wr_task]
    {
        const auto task = wr_task.lock();
        if (!task)
        {
            return 0;
        }

        if (!task->load_metainfo_from_local_cache())
        {
            return -1;
        }

        if (!task->is_downloaded_file_exists())
        {
            return -1;
        }

        return 0;

    })->on_result_ = [wr_this, wr_task, _function](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto task = wr_task.lock();
        if (!task)
            return;

        const auto is_local_copy_test_mode = (_function == file_sharing_function::check_local_copy_exists);
        if ((_error == 0) || is_local_copy_test_mode)
        {
            task->copy_if_needed();
            ptr_this->on_task_result(task, _error);
            return;
        }

        ptr_this->threads_->run_async_function([task]
        {
            return task->download_metainfo();

        })->on_result_ = [wr_this, wr_task, _function](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            const auto is_download_file_metainfo_mode = (_function == file_sharing_function::download_file_metainfo);
            if ((_error != 0) || is_download_file_metainfo_mode)
            {
                ptr_this->on_task_result(task, _error);
                return;
            }

			g_core->insert_event(stats::stats_event_names::filesharing_download_success);
            ptr_this->on_task_progress(task);

            ptr_this->threads_->run_async_function([wr_task]
            {
                auto task = wr_task.lock();
                if (!task)
                    return -1;

                return task->open_temporary_file();

            })->on_result_ = [wr_this, wr_task](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                auto task = wr_task.lock();
                if (!task)
                    return;

                if (_error != 0)
                {
                    ptr_this->on_task_result(task, _error);
                    return;
                }

                ptr_this->load_task_ranges_async(task);
            };

        };
    };

    return task->get_handler();
}

std::shared_ptr<async_task_handlers> loader::download_file(
    const std::string& _file_url,
    const std::wstring& _file_name,
    const bool _keep_alive,
    const wim_packet_params& _params)
{
    auto handler = std::make_shared<async_task_handlers>();

    auto user_proxy = g_core->get_user_proxy_settings();
    threads_->run_async_function([_params, _file_url, _file_name, user_proxy, _keep_alive]()->int32_t
    {
        core::http_request_simple request(user_proxy, utils::get_user_agent(), _params.stop_handler_);

        request.set_url(_file_url);
        request.set_need_log(false);
        if (_keep_alive)
            request.set_keep_alive();

        if (!request.get() || request.get_response_code() != 200)
            return loader_errors::network_error;

        if (!request.get_response()->save_2_file(_file_name))
            return loader_errors::save_2_file;

        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<download_image_handler> loader::download_image_file(
    const int64_t _seq,
    const std::string& _image_url,
    const std::wstring& _local_path,
    const bool _sign_url,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_image_url.empty());
    assert(!_local_path.empty());

    auto handler = std::make_shared<download_image_handler>();
    auto preview_data = std::make_shared<core::tools::binary_stream>();
    auto user_proxy = g_core->get_user_proxy_settings();

    auto file_extension = std::make_shared<std::wstring>();
    
    threads_->run_async_function(
        [_params, _image_url, _sign_url, _local_path, preview_data, user_proxy, file_extension]
        {
            for (auto i = 0; i < 2; i++)
            {
                if (!core::tools::system::is_exist(_local_path) || i > 0)
                {
                    core::http_request_simple request(user_proxy, utils::get_user_agent(), _params.stop_handler_);

                    const auto &url = (_sign_url ? sign_loader_uri(_image_url, _params) : _image_url);

                    request.set_url(url);
                    request.set_need_log(false);

                    if (!request.get() || request.get_response_code() != 200)
                    {
                        return loader_errors::network_error;
                    }

                    /*
                    if (platform::is_apple())
                    {
                        auto header = core::tools::to_lower_case(request.get_header()->read<std::string>());
                        auto header_fields = core::tools::to_array(header, "\r\n");
                        for (auto header_field: header_fields)
                        {
                            auto header_field_parts = core::tools::to_array(header_field, ":");
                            if (header_field_parts.size() == 2 && header_field_parts[0].find("content-type") < header_field_parts[0].length())
                            {
                                header_field_parts[1] = core::tools::trim_left<std::string>(header_field_parts[1], " ");
                                header_field_parts[1] = core::tools::trim_right<std::string>(header_field_parts[1], " ");
                                auto content_type_parts = core::tools::to_array(header_field_parts[1], "/");
                                if (content_type_parts.size() == 2)
                                {
                                    file_extension->assign(L"." + core::tools::from_utf8(content_type_parts[1]));
                                }
                                break;
                            }
                        }
                    }
                    */

                    preview_data->swap(*request.get_response());

                    if (!preview_data->save_2_file(_local_path + file_extension->c_str()))
                        return loader_errors::save_2_file;

                    preview_data->reset_out();

                    break;
                }
                else if (preview_data->load_from_file(_local_path))
                {
                    break;
                }
            }

            return loader_errors::success;
        }
    )->on_result_ =
        [handler, preview_data, _local_path, file_extension]
        (int32_t _error)
        {
            if (_error == 0 && preview_data->available() == 0)
            {
                assert(!"logical error");
            }

            if (handler->on_result)
            {
                handler->on_result(_error, preview_data, core::tools::from_utf16(_local_path + file_extension->c_str()));
            }
        };

    return handler;
}

std::shared_ptr<download_image_handler> loader::download_preview(
    const int64_t _seq,
    const std::string& _image_url,
    const std::wstring& _cache_dir,
    const std::wstring& _local_path,
    const int32_t _preview_height,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_image_url.empty());
    assert(!_local_path.empty());
    assert(_preview_height > 100);
    assert(_preview_height < 1000);

    auto handler = std::make_shared<download_image_handler>();
    auto user_proxy = g_core->get_user_proxy_settings();
    auto preview_info = std::make_shared<preview_proxy::preview_info_uptr>();
    auto preview_data = std::make_shared<core::tools::binary_stream>();

    std::weak_ptr<loader> wr_this = shared_from_this();

    threads_->run_async_function(
        [_image_url, _local_path, _params, user_proxy, preview_info, preview_data]
        {
            if (core::tools::system::is_exist(_local_path))
            {
                preview_data->load_from_file(_local_path);

                return loader_errors::success;
            }

            core::http_request_simple request(
                user_proxy,
                utils::get_user_agent(_params.aimid_),
                _params.stop_handler_);

            const auto ext_params = preview_proxy::format_params(_image_url);
            const auto signed_request_uri = sign_loader_uri(preview_proxy::host(), _params, ext_params);
            request.set_url(signed_request_uri);

            request.set_need_log(false);
            request.set_keep_alive();

            if (!request.get() || (request.get_response_code() != 200))
            {
                return loader_errors::network_error;
            }

            auto response = request.get_response();
            auto response_size = response->available();

            const auto is_empty_response = (response_size == 0);
            if (is_empty_response)
            {
                return loader_errors::invalid_json;
            }

            std::vector<char> json;
            json.reserve(response_size + 1);

            const auto str = (char*)response->read(response_size);
            json.assign(str, str + response_size);
            json.push_back('\0');

            *preview_info = preview_proxy::parse_json(&json[0]);
            if (*preview_info)
            {
                return loader_errors::success;
            }

            return loader_errors::invalid_json;
        }
    )->on_result_ =
        [_seq, _cache_dir, _params, _local_path, preview_info, handler, wr_this, preview_data, _preview_height]
        (int32_t _error)
        {
            const auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            if (!handler->on_result)
            {
                return;
            }

            if (_error != loader_errors::success)
            {
                assert(!*preview_info);
                handler->on_result(_error, std::make_shared<tools::binary_stream>(), std::string());
                return;
            }

            const auto local_file_exists = (preview_data->available() > 0);
            if (local_file_exists)
            {
                handler->on_result(loader_errors::success, preview_data, tools::from_utf16(_local_path));
                return;
            }

            const auto preview_uri = (*preview_info)->get_preview_uri(0, _preview_height);

            assert(*preview_info);
            ptr_this->download_image(_seq, preview_uri, _cache_dir, _local_path, false, false, 0, _params)->on_result =
                [handler]
                (int32_t _error, std::shared_ptr<core::tools::binary_stream> _data, const std::string& _local_path)
                {
                    handler->on_result(loader_errors::success, _data, _local_path);
                };
        };

    return handler;
}

std::shared_ptr<download_image_handler> loader::download_image(
    const int64_t _seq,
    const std::string& _image_url,
    const std::wstring& _cache_dir,
    const std::wstring& _forced_local_path,
    const bool _sign_url,
    const bool _download_preview,
    const int32_t _preview_height,
    const wim_packet_params& _params)
{
    assert(_seq > 0);
    assert(!_image_url.empty());
    assert(!_cache_dir.empty());
    assert(_preview_height >= 0);
    assert(_preview_height < 1000);

    const auto &local_path =
        _forced_local_path.empty() ?
            build_local_path(_cache_dir, _image_url, _download_preview) :
            _forced_local_path;
    assert(!local_path.empty());

    if (_download_preview)
    {
        assert(_preview_height > 0);
        return download_preview(_seq, _image_url, _cache_dir, local_path, _preview_height, _params);
    }

    //assert(_preview_height == 0);
    return download_image_file(_seq, _image_url, local_path, _sign_url, _params);
}

void loader::abort_process(const std::string &_process_id)
{
    assert(!_process_id.empty());

    remove_task(_process_id);
}

namespace
{

    std::string sign_loader_uri(const std::string &_host, const wim_packet_params &_params, const Str2StrMap &_extra)
    {
        assert(!_host.empty());

        const time_t ts = (std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - _params.time_offset_);

        Str2StrMap p;
        p["a"] = _params.a_token_;
        p["k"] = _params.dev_id_;
        p["ts"] = tools::from_int64(ts);
        p["client"] = "icq";

        p.insert(_extra.begin(), _extra.end());

        const auto sha256 = wim_packet::escape_symbols(wim_packet::get_url_sign(_host, p, _params, false));
        p["sig_sha256"] = sha256;

        std::stringstream ss_url;
        ss_url << _host << "?" << wim_packet::format_get_params(p);

        return ss_url.str();
    }

    std::wstring build_local_path(const std::wstring& _cache_dir, const std::string& _image_url, const bool _is_preview)
    {
        assert(!_cache_dir.empty());
        assert(!_image_url.empty());

        boost::filesystem::wpath local_path(_cache_dir);

        auto filename = core::tools::from_utf8(
            core::tools::md5(
                _image_url.c_str(),
                (int)_image_url.length()));

        if (_is_preview)
        {
            filename += L"_preview";
        }
        
        if (platform::is_apple())
        {
            filename += L".jpg";
        }

        local_path /= filename;

        return local_path.wstring();
    }

}