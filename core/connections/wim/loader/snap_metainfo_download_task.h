#pragma once

#include "generic_loader_task.h"

#include "loader_handlers.h"

CORE_WIM_NS_BEGIN

class snap_metainfo_download_task final : public generic_loader_task
{
public:
    snap_metainfo_download_task(
        const int64_t _id,
        const std::string& _contact_aimid,
        const wim_packet_params &_params,
        const proxy_settings &_proxy_settings,
        const std::string &_ttl_id,
        const std::wstring &_cache_dir,
        disk_cache::disk_cache_sptr &_cache,
        std::shared_ptr<download_snap_metainfo_handler> _result_handler);

    virtual tasks_runner_slot get_slot() const override;

    virtual void on_result(const loader_errors _error) override;

    virtual loader_errors run() override;

private:
    std::shared_ptr<download_snap_metainfo_handler> result_handler_;

    const std::string ttl_id_;

    const std::wstring cache_dir_;

    snaps::snap_metainfo_uptr snap_metainfo_;
};

CORE_WIM_NS_END