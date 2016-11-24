#pragma once

#include "generic_loader_task.h"

#include "loader_handlers.h"

CORE_WIM_NS_BEGIN

class image_download_task final : public generic_loader_task
{
public:
    image_download_task(
        const int64_t _id,
        const std::string& _contact_aimid,
        const wim_packet_params &_params,
        const proxy_settings &_proxy_settings,
        const std::string& _image_uri,
        const std::wstring& _image_local_path,
        const std::wstring& _cache_dir,
        disk_cache::disk_cache_sptr &_cache,
        const bool _sign_url,
        std::shared_ptr<download_image_handler> _result_handler);

    virtual ~image_download_task() override;

    virtual tasks_runner_slot get_slot() const override;

    virtual void on_before_suspend() override;

    virtual void on_result(const loader_errors _error) override;

    virtual loader_errors run() override;

    virtual std::string to_log_str() const override;

    tools::binary_stream& get_image_preview_data();

private:
    std::string get_patched_image_uri() const;

    const std::wstring cache_dir_;

    const std::wstring image_local_path_;

    const std::string image_uri_;

    const bool sign_url_;

    std::shared_ptr<tools::binary_stream> image_preview_data_;

    std::shared_ptr<download_image_handler> result_handler_;

};

CORE_WIM_NS_END