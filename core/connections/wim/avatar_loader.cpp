#include "stdafx.h"


#include <algorithm>


#include "avatar_loader.h"
#include "../../async_task.h"
#include "../../tools/system.h"

#include "packets/request_avatar.h"

using namespace core;
using namespace wim;

//////////////////////////////////////////////////////////////////////////
// avatar_task
//////////////////////////////////////////////////////////////////////////
avatar_task::avatar_task(
    int64_t _task_id,
    std::shared_ptr<avatar_context> _context, 
    std::shared_ptr<avatar_load_handlers> _handlers)
    :   task_id_(_task_id),
        context_(_context),
        handlers_(_handlers)
{
}

std::shared_ptr<avatar_context> avatar_task::get_context() const
{
    return context_;
}

std::shared_ptr<avatar_load_handlers> avatar_task::get_handlers() const
{
    return handlers_;
}

int64_t avatar_task::get_id() const
{
    return task_id_;
}




//////////////////////////////////////////////////////////////////////////
avatar_loader::avatar_loader()
    :   local_thread_(new async_executer()),
        server_thread_(new async_executer()),
        working_(false),
        network_error_(false),
        task_id_(0)
{
}


avatar_loader::~avatar_loader()
{
}

const std::string avatar_loader::get_avatar_type_by_size(int32_t _size) const
{
    if (_size > 128)
    {
        return "floorLargeBuddyIcon";
    }
    else if (_size > 64)
    {
        return "floorBigBuddyIcon";
    }

    return "ceilBigBuddyIcon";
}

const std::wstring avatar_loader::get_avatar_path(const std::wstring& _im_data_path, const std::string& _contact, const std::string _avatar_type)
{
    std::string lower_avatar_type = _avatar_type;
    std::transform(lower_avatar_type.begin(), lower_avatar_type.end(), lower_avatar_type.begin(), ::tolower);

    std::wstring file_name = core::tools::from_utf8(_contact);
    std::replace(file_name.begin(), file_name.end(), L'|', L'_');

    return (_im_data_path + L"/" + L"avatars" + L"/" + file_name + L"/" + core::tools::from_utf8(lower_avatar_type) + L"_.jpg");
}

bool load_avatar_from_file(std::shared_ptr<avatar_context> _context)
{
    if (!core::tools::system::is_exist(_context->avatar_file_path_))
        return false;

    boost::filesystem::wpath path(_context->avatar_file_path_);
    if (!_context->force_)
        _context->write_time_ = last_write_time(path);
    if (!_context->avatar_data_.load_from_file(_context->avatar_file_path_))
        return false;

    _context->avatar_exist_ = true;

    return true;
}


void avatar_loader::execute_task(std::shared_ptr<avatar_task> _task, std::function<void(int32_t)> _on_complete)
{
    time_t write_time = _task->get_context()->avatar_exist_ ? _task->get_context()->write_time_ : 0;

    if (!wim_params_)
    {
        assert(false);
        _on_complete(wim_protocol_internal_error::wpie_network_error);

        return;
    }

    auto packet = std::make_shared<request_avatar>(
        *wim_params_, 
        _task->get_context()->contact_, 
        _task->get_context()->avatar_type_, 
        write_time);

    std::weak_ptr<avatar_loader> wr_this = shared_from_this();

    server_thread_->run_async_task(packet)->on_result_ = [wr_this, _task, packet, _on_complete](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto avatar_data = packet->get_data();

            ptr_this->local_thread_->run_async_function([avatar_data, _task]()->int32_t
            {
                uint32_t size = avatar_data->available();
                assert(size);
                if (size == 0)
                    return wpie_error_empty_avatar_data;

                _task->get_context()->avatar_data_.write(avatar_data->read(size), size);
                avatar_data->reset_out();

                avatar_data->save_2_file(_task->get_context()->avatar_file_path_);
                return 0;

            })->on_result_ = [avatar_data, wr_this, _on_complete, _task](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == 0)
                {
                    if (_task->get_context()->avatar_exist_)
                        _task->get_handlers()->updated_(_task->get_context());
                    else
                        _task->get_handlers()->completed_(_task->get_context());
                }
                else
                {
                    _task->get_handlers()->failed_(_task->get_context(), _error);
                }

                _on_complete(_error);
            };
        }
        else
        {
            if (_task->get_context()->avatar_exist_)
            {
                long http_error = packet->get_http_code();
                if (http_error == 304) {}
            }
            else
            {
                _task->get_handlers()->failed_(_task->get_context(), _error);
            }

            _on_complete(_error);
        }
    };

}


void avatar_loader::run_tasks_loop()
{
    working_ = true;

    assert(!network_error_);

    auto task = get_next_task();
    if (!task)
    {
        working_ =  false;
        return;
    }

    std::weak_ptr<avatar_loader> wr_this = shared_from_this();

 
    execute_task(task, [wr_this, task](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == wim_protocol_internal_error::wpie_network_error)
        {
            ptr_this->network_error_ = true;
            ptr_this->working_ = false;

            return;
        }

        ptr_this->remove_task(task);
        ptr_this->run_tasks_loop();
    });
}


void avatar_loader::remove_task(std::shared_ptr<avatar_task> _task)
{
    requests_queue_.remove_if([_task](const std::shared_ptr<avatar_task>& _current_task)->bool
    {
        return (_current_task->get_id() == _task->get_id());
    });
}

void avatar_loader::add_task(std::shared_ptr<avatar_task> _task)
{
    requests_queue_.push_front(_task);
}

std::shared_ptr<avatar_task> avatar_loader::get_next_task()
{
    if (requests_queue_.empty())
    {
        return std::shared_ptr<avatar_task>();
    }

    return requests_queue_.front();
}

void avatar_loader::load_avatar_from_server(
    std::shared_ptr<avatar_context> _context, 
    std::shared_ptr<avatar_load_handlers> _handlers)
{
    std::weak_ptr<avatar_loader> wr_this = shared_from_this();

    add_task(std::make_shared<avatar_task>(++task_id_, _context, _handlers));

    if (!working_ && !network_error_)
    {
        run_tasks_loop();
    }

    return;
}

std::shared_ptr<avatar_load_handlers> avatar_loader::get_contact_avatar_async(const wim_packet_params& _params, std::shared_ptr<avatar_context> _context)
{
    if (!wim_params_)
    {
        wim_params_ = std::make_shared<wim_packet_params>(_params);
    }
    else
    {
        *wim_params_ = _params;
    }

    auto handlers = std::make_shared<avatar_load_handlers>();

    _context->avatar_type_ = get_avatar_type_by_size(_context->avatar_size_);
    _context->avatar_file_path_ = get_avatar_path(_context->im_data_path_, _context->contact_, _context->avatar_type_);
    std::weak_ptr<avatar_loader> wr_this = shared_from_this();

    local_thread_->run_async_function([_context, handlers]()->int32_t
    {
        return (load_avatar_from_file(_context) ? 0 : -1);

    })->on_result_ = [wr_this, handlers, _context](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            handlers->completed_(_context);
        }

        ptr_this->load_avatar_from_server(_context, handlers);
    };

    return handlers;
}

void avatar_loader::resume(const wim_packet_params& _params)
{
    if (!wim_params_)
    {
        wim_params_ = std::make_shared<wim_packet_params>(_params);
    }
    else
    {
        *wim_params_ = _params;
    }

    if (!network_error_)
        return;

    network_error_ = false;

    assert(!working_);
    if (!working_)
    {
        run_tasks_loop();
    }
}

void avatar_loader::show_contact_avatar(const std::string& _contact, const int32_t _avatar_size)
{
    for (auto iter = requests_queue_.begin(); iter != requests_queue_.end(); ++iter)
    {
        if ((*iter)->get_context()->contact_ == _contact && (*iter)->get_context()->avatar_size_ == _avatar_size)
        {
            auto task = *iter;

            requests_queue_.erase(iter);

            requests_queue_.push_back(task);

            break;
        }
    }
}