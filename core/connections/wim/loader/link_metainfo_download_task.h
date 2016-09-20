#pragma once

#include "generic_loader_task.h"

#include "loader_handlers.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

class link_meta;

typedef std::unique_ptr<link_meta> link_meta_uptr;

CORE_WIM_PREVIEW_PROXY_NS_END

CORE_WIM_NS_BEGIN

class link_metainfo_download_task final : public generic_loader_task
{
public:
    link_metainfo_download_task(
        const int64_t _id,
        const std::string& _contact_aimid,
        const wim_packet_params &_params,
        const proxy_settings &_proxy_settings,
        const std::string& _uri,
        const std::wstring& _cache_dir,
        disk_cache::disk_cache_sptr &_cache,
        const bool _sign_uri,
        std::shared_ptr<download_link_metainfo_handler> _result_handler);

    virtual ~link_metainfo_download_task() override;

    virtual tasks_runner_slot get_slot() const override;

    virtual void on_result(const loader_errors _error) override;

    virtual loader_errors run() override;

    const preview_proxy::link_meta& get_link_metainfo() const;

private:
    const std::wstring cache_dir_;

    const bool sign_uri_;

    const std::string uri_;

    std::wstring meta_local_path_;

    preview_proxy::link_meta_uptr link_metainfo_;

    std::shared_ptr<download_link_metainfo_handler> result_handler_;

    bool load_from_cache(disk_cache::disk_cache &_cache);

};

CORE_WIM_NS_END