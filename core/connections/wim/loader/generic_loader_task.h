#pragma  once

#include "loader_task.h"

#include "../../../proxy_settings.h"

#include "../wim_packet.h"

CORE_DISK_CACHE_NS_BEGIN

class disk_cache;

typedef std::weak_ptr<disk_cache> disk_cache_wptr;

typedef std::shared_ptr<disk_cache> disk_cache_sptr;

CORE_DISK_CACHE_NS_END

CORE_WIM_NS_BEGIN

class generic_loader_task : public loader_task
{
public:
    generic_loader_task(
        const int64_t _id,
        const std::string& _contact_aimid,
        const wim_packet_params &_params,
        const proxy_settings &_proxy_settings,
        const disk_cache::disk_cache_sptr &_cache);

    virtual ~generic_loader_task() override = 0;

    virtual void cancel() override;

    virtual const std::string& get_contact_aimid() const override;

    virtual int64_t get_id() const override;

    virtual tasks_runner_slot get_slot() const override;

    bool is_cancelled() const;

    virtual void on_before_resume(
        const wim_packet_params &_wim_params,
        const proxy_settings &_proxy_settings,
        const bool _is_genuine) override;

    virtual void on_before_suspend() override;

    virtual std::string to_log_str() const final override;

protected:
    disk_cache::disk_cache_wptr get_cache() const;

    const proxy_settings& get_proxy_settings() const;

    const wim_packet_params& get_wim_params() const;

    bool is_prefetching() const;

private:
    int64_t id_;

    std::atomic<bool> is_cancelled_;

    std::string contact_aimid_;

    wim_packet_params wim_params_;

    proxy_settings proxy_settings_;

    disk_cache::disk_cache_wptr cache_;

};

CORE_WIM_NS_END