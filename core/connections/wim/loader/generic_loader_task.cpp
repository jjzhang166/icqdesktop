#include "stdafx.h"

#include "tasks_runner_slot.h"

#include "generic_loader_task.h"

CORE_WIM_NS_BEGIN

generic_loader_task::generic_loader_task(
    const int64_t _id,
    const std::string& _contact_aimid,
    const wim_packet_params &_params,
    const proxy_settings &_proxy_settings,
    const disk_cache::disk_cache_sptr &_cache)
    : id_(_id)
    , contact_aimid_(_contact_aimid)
    , proxy_settings_(_proxy_settings)
    , wim_params_(_params)
    , cache_(_cache)
    , is_cancelled_(false)
{
    assert(id_ > 0);
    assert(!cache_.expired());
    assert(!contact_aimid_.empty());
}

generic_loader_task::~generic_loader_task()
{
}

void generic_loader_task::cancel()
{
    is_cancelled_ = true;
}

const std::string& generic_loader_task::get_contact_aimid() const
{
    assert(!contact_aimid_.empty());

    return contact_aimid_;
}

int64_t generic_loader_task::get_id() const
{
    assert(id_ > 0);
    return id_;
}

tasks_runner_slot generic_loader_task::get_slot() const
{
    return tasks_runner_slot::generic;
}

void generic_loader_task::on_before_resume(const wim_packet_params &_wim_params, const proxy_settings &_proxy_settings, const bool _is_genuine)
{
    UNUSED_ARG(_is_genuine);

    wim_params_ = _wim_params;
    proxy_settings_ = _proxy_settings;
}

void generic_loader_task::on_before_suspend()
{

}

std::string generic_loader_task::to_log_str() const
{
    return "generic";
}

disk_cache::disk_cache_wptr generic_loader_task::get_cache() const
{
    return cache_;
}

const proxy_settings& generic_loader_task::get_proxy_settings() const
{
    return proxy_settings_;
}

const wim_packet_params& generic_loader_task::get_wim_params() const
{
    return wim_params_;
}

bool generic_loader_task::is_cancelled() const
{
    return is_cancelled_;
}

bool generic_loader_task::is_prefetching() const
{
    return (get_id() >= INT32_MAX);
}

CORE_WIM_NS_END