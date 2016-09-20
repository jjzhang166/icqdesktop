#pragma once

#include "generic_loader_task.h"

#include "loader_handlers.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

class link_meta;

typedef std::unique_ptr<link_meta> link_meta_uptr;

CORE_WIM_PREVIEW_PROXY_NS_END

CORE_WIM_NS_BEGIN

class image_preview_download_task final : public generic_loader_task
{
public:
    image_preview_download_task(
        const int64_t _id,
        const std::string& _contact_aimid,
        const wim_packet_params &_params,
        const proxy_settings &_proxy_settings,
        const std::string& _image_uri,
        const std::wstring& _cache_dir,
        disk_cache::disk_cache_sptr &_cache,
        const int32_t _max_preview_width,
        const int32_t _max_preview_height,
        std::shared_ptr<download_image_handler> &_result_handler);

    virtual ~image_preview_download_task() override;

    virtual tasks_runner_slot get_slot() const override;

    virtual void on_before_suspend() override;

    virtual void on_result(const loader_errors _error) override;

    virtual loader_errors run() override;

private:
    std::wstring image_local_path_;

    const std::wstring cache_dir_;

    const int32_t max_preview_height_;

    const int32_t max_preview_width_;

    const std::string image_uri_;

    std::shared_ptr<tools::binary_stream> image_preview_data_;

    std::shared_ptr<download_image_handler> result_handler_;

    void send_metainfo(const preview_proxy::link_meta &_meta) const;

};

CORE_WIM_NS_END