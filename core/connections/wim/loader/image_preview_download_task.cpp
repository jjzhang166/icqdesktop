#include "stdafx.h"

#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../tools/strings.h"
#include "../../../log/log.h"
#include "../../../utils.h"

#include "image_download_task.h"
#include "link_metainfo_download_task.h"
#include "loader_errors.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "tasks_runner_slot.h"

#include "image_preview_download_task.h"

CORE_WIM_NS_BEGIN

image_preview_download_task::image_preview_download_task(
    const int64_t _id,
    const std::string& _contact_aimid,
    const wim_packet_params &_params,
    const proxy_settings &_proxy_settings,
    const std::string& _image_uri,
    const std::wstring& _cache_dir,
    disk_cache::disk_cache_sptr &_cache,
    const int32_t _max_preview_width,
    const int32_t _max_preview_height,
    std::shared_ptr<download_image_handler> &_result_handler)
    : generic_loader_task(_id, _contact_aimid, _params, _proxy_settings, _cache)
    , image_uri_(_image_uri)
    , cache_dir_(_cache_dir)
    , max_preview_height_(_max_preview_height)
    , max_preview_width_(_max_preview_width)
    , result_handler_(_result_handler)
{
    assert(_max_preview_height >= 0);
    assert(_max_preview_width >= 0);

    image_local_path_ = get_path_in_cache(cache_dir_, image_uri_, path_type::link_preview);
}

image_preview_download_task::~image_preview_download_task()
{
}

loader_errors image_preview_download_task::run()
{
    __INFO(
        "snippets",
        "runnning task\n"
        "    type=<image_preview>\n"
        "    id=<%1%>\n"
        "    uri=<%2%>",
        get_id() % image_uri_);

    auto cache = get_cache().lock();
    if (!cache)
    {
        return loader_errors::orphaned;
    }

    link_metainfo_download_task download_metainfo_task(
        get_id(),
        get_contact_aimid(),
        get_wim_params(),
        get_proxy_settings(),
        image_uri_,
        cache_dir_,
        cache,
        false,
        std::make_shared<download_link_metainfo_handler>());

    const auto metainfo_error = download_metainfo_task.run();
    if (metainfo_error != loader_errors::success)
    {
        return metainfo_error;
    }

    const auto &preview_metainfo = download_metainfo_task.get_link_metainfo();

    send_metainfo(preview_metainfo);

    assert(!image_preview_data_);
    image_preview_data_ = std::make_shared<tools::binary_stream>();

    if (image_preview_data_->load_from_file(image_local_path_))
    {
        __INFO(
            "snippets",
            "task succeed\n"
            "    type=<image_preview>\n"
            "    id=<%1%>\n"
            "    uri=<%2%>\n"
            "    cache=<%3%>\n"
            "    source=<cache>",
            get_id() % image_uri_ % tools::from_utf16(image_local_path_));

        return loader_errors::success;
    }

    if (!preview_metainfo.has_preview_uri())
    {
        return loader_errors::no_link_preview;
    }

    const auto preview_uri = preview_metainfo.get_preview_uri(0, 0);

    image_download_task download_image_task(
        get_id(),
        get_contact_aimid(),
        get_wim_params(),
        get_proxy_settings(),
        preview_uri,
        image_local_path_,
        cache_dir_,
        cache,
        false,
        std::shared_ptr<download_image_handler>());

    const auto image_error = download_image_task.run();
    if (image_error != loader_errors::success)
    {
        return image_error;
    }

    *image_preview_data_ = std::move(download_image_task.get_image_preview_data());

    __INFO(
        "snippets",
        "task succeed\n"
        "    type=<image_preview>\n"
        "    id=<%1%>\n"
        "    uri=<%2%>\n"
        "    source=<uri>",
        get_id() % image_uri_);

    return loader_errors::success;
}

tasks_runner_slot image_preview_download_task::get_slot() const
{
    assert(!image_local_path_.empty());

    const auto uri_hash = std::hash<std::string>()(image_uri_);

    const auto slot_index = (uri_hash % 3);

    static const tasks_runner_slot slots[] = {
        tasks_runner_slot::previews_a,
        tasks_runner_slot::previews_b,
        tasks_runner_slot::previews_c
    };

    return slots[slot_index];
}

void image_preview_download_task::on_before_suspend()
{
    image_preview_data_.reset();

    generic_loader_task::on_before_suspend();
}

void image_preview_download_task::on_result(const loader_errors _error)
{
    assert(result_handler_);
    assert(result_handler_->on_image_result);

    const auto success = (_error == loader_errors::success);

    if (success)
    {
        result_handler_->on_image_result((int32_t)_error, image_preview_data_, image_uri_, tools::from_utf16(image_local_path_));
    }
    else
    {
        result_handler_->on_image_result((int32_t)_error, std::make_shared<tools::binary_stream>(), image_uri_, std::string());
    }
}

void image_preview_download_task::send_metainfo(const preview_proxy::link_meta &_meta) const
{
    const auto on_meta_result = result_handler_->on_meta_result;
    if (!on_meta_result)
    {
        return;
    }

    const auto preview_size = _meta.get_preview_size();

    const auto preview_width = std::get<0>(preview_size);
    const auto preview_height = std::get<1>(preview_size);
    assert(preview_width >= 0);
    assert(preview_height >= 0);

    const auto is_empty_preview = ((preview_width <= 0) || (preview_height <= 0));
    if (is_empty_preview)
    {
        return;
    }

    const auto download_uri = _meta.get_download_uri();

    const auto file_size = _meta.get_file_size();

    const auto task_id = get_id();

    g_core->excute_core_context(
        [on_meta_result, preview_width, preview_height, download_uri, task_id, file_size]
        {
            __INFO(
                "snippets",
                "sending image preview metainfo\n"
                "    id=<%1%>\n"
                "    size=<%2%,%3%>\n"
                "    download_uri=<%4%>",
                task_id % preview_width % preview_height % download_uri);

            on_meta_result(preview_width, preview_height, download_uri, file_size);
        });
}

CORE_WIM_NS_END