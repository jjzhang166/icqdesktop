#include "stdafx.h"


#include <algorithm>


#include "avatar_loader.h"
#include "../../async_task.h"

#include "packets/request_avatar.h"

using namespace core;
using namespace wim;

//////////////////////////////////////////////////////////////////////////
// avatar_task
//////////////////////////////////////////////////////////////////////////
avatar_task::avatar_task(
    std::shared_ptr<avatar_context> _context, 
    std::shared_ptr<avatar_load_handlers> _handlers)
    :   context_(_context),
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




//////////////////////////////////////////////////////////////////////////
avatar_loader::avatar_loader()
    :	local_thread_(new async_executer()),
    server_thread_(new async_executer())
{
}


avatar_loader::~avatar_loader()
{
}

const std::string avatar_loader::get_avatar_type_by_size(int32_t _size) const
{
    if (_size >= 128)
    {
        return "floorLargeBuddyIcon";
    }
    else if (_size > 90)
    {
        return "floorBigBuddyIcon";
    }

    return "buddyIcon";
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
    boost::filesystem::wpath path_for_file(_context->avatar_file_path_);
    if (!boost::filesystem::exists(path_for_file))
        return false;

    _context->write_time_ = last_write_time(path_for_file);
    if (!_context->avatar_data_.load_from_file(_context->avatar_file_path_))
        return false;

    _context->avatar_exist_ = true;

    return true;
}

void avatar_loader::load_avatar_from_server(
    std::shared_ptr<avatar_context> _context, 
    std::shared_ptr<avatar_load_handlers> _handlers)
{
    time_t write_time = _context->avatar_exist_ ? _context->write_time_ : 0;

    auto packet = std::make_shared<request_avatar>(_context->wim_params_, _context->contact_, _context->avatar_type_, write_time);

    std::weak_ptr<avatar_loader> wr_this = shared_from_this();

    server_thread_->run_async_task(packet)->on_result_ = [wr_this, _handlers, _context, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto avatar_data = packet->get_data();

            ptr_this->local_thread_->run_async_function([avatar_data, _context]()->int32_t
            {
                uint32_t size = avatar_data->available();
                assert(size);
                if (size == 0)
                    return wpie_error_empty_avatar_data;

                _context->avatar_data_.write(avatar_data->read(size), size);
                avatar_data->reset_out();

                avatar_data->save_2_file(_context->avatar_file_path_);
                return 0;

            })->on_result_ = [avatar_data, wr_this, _handlers, _context](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == 0)
                {
                    if (_context->avatar_exist_)
                        _handlers->updated_(_context);
                    else
                        _handlers->completed_(_context);
                }
                else
                {
                    _handlers->failed_(_context, _error);
                }
            };
        }
        else
        {
            if (_error == wim_protocol_internal_error::wpie_network_error)
            {
                ptr_this->failed_tasks_.push_back(std::make_shared<avatar_task>(_context, _handlers));
                return;
            }
            

            if (_context->avatar_exist_)
            {
                long http_error = packet->get_http_code();
                if (http_error == 304) {}
            }
            else
            {
                _handlers->failed_(_context, _error);
            }
        }
    };
}

std::shared_ptr<avatar_load_handlers> avatar_loader::get_contact_avatar_async(std::shared_ptr<avatar_context> _context)
{
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
            handlers->completed_(_context);

        ptr_this->load_avatar_from_server(_context, handlers);
    };

    return handlers;
}

void avatar_loader::resume()
{
    if (failed_tasks_.empty())
        return;

    for (auto _task : failed_tasks_)
        load_avatar_from_server(_task->get_context(), _task->get_handlers());

    failed_tasks_.clear();
}
