#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../log/log.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

#include "../../../disk_cache/disk_cache.h"
#include "../../../disk_cache/cache_entity_type.h"

#include "loader_errors.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "tasks_runner_slot.h"

#include "link_metainfo_download_task.h"

CORE_WIM_NS_BEGIN

link_metainfo_download_task::link_metainfo_download_task(
    const int64_t _id,
    const std::string& _contact_aimid,
    const wim_packet_params &_params,
    const proxy_settings &_proxy_settings,
    const std::string& _uri,
    const std::wstring& _cache_dir,
    disk_cache::disk_cache_sptr &_cache,
    const bool _sign_uri,
    std::shared_ptr<download_link_metainfo_handler> _result_handler)
    : generic_loader_task(_id, _contact_aimid, _params, _proxy_settings, _cache)
    , uri_(_uri)
    , cache_dir_(_cache_dir)
    , sign_uri_(_sign_uri)
    , result_handler_(_result_handler)
{
    meta_local_path_ = get_path_in_cache(cache_dir_, uri_, path_type::link_meta);
    assert(!meta_local_path_.empty());
}

link_metainfo_download_task::~link_metainfo_download_task()
{
}

tasks_runner_slot link_metainfo_download_task::get_slot() const
{
    assert(!meta_local_path_.empty());

    const auto uri_hash = std::hash<std::string>()(uri_);

    const auto is_slot_a = ((uri_hash % 2) == 0);

    return
        is_slot_a ?
            tasks_runner_slot::metainfo_a :
            tasks_runner_slot::metainfo_b;
}

void link_metainfo_download_task::on_result(const loader_errors _error)
{
    assert(result_handler_);
    assert(result_handler_->on_result);

    const auto success = (_error == loader_errors::success);

    assert(!success || link_metainfo_);

    if (!result_handler_ || !result_handler_->on_result)
    {
        return;
    }

    result_handler_->on_result((int32_t)_error, link_metainfo_);
}

loader_errors link_metainfo_download_task::run()
{
    __INFO(
        "snippets",
        "runnning task\n"
        "    type=<link_metainfo>\n"
        "    id=<%1%>\n"
        "    uri=<%2%>",
        get_id() % uri_);

    auto cache = get_cache().lock();
    if (!cache)
    {
        return loader_errors::orphaned;
    }

    if (load_from_cache(*cache))
    {
        return loader_errors::success;
    }

    link_metainfo_ = load_link_meta_from_file(meta_local_path_ , uri_);
    if (link_metainfo_)
    {
        __INFO(
            "snippets",
            "task succeed\n"
            "    type=<link_metainfo>\n"
            "    id=<%1%>\n"
            "    uri=<%2%>\n"
            "    cache=<%3%>\n"
            "    source=<cache>",
            get_id() % uri_ % tools::from_utf16(meta_local_path_));

        return loader_errors::success;
    }

    const auto &wim_params = get_wim_params();
    const auto &proxy_settings = get_proxy_settings();

    core::http_request_simple request(
        proxy_settings,
        utils::get_user_agent(wim_params.aimid_),
        wim_params.stop_handler_);

    const auto ext_params = preview_proxy::format_get_preview_params(uri_);
    const auto signed_request_uri = sign_loader_uri(preview_proxy::uri::get_preview(), wim_params, ext_params);
    request.set_url(signed_request_uri);

    request.set_need_log(false);
    request.set_keep_alive();
    request.set_connect_timeout(1000);

    if (!request.get())
    {
        return loader_errors::network_error;
    }

    if (request.get_response_code() != 200)
    {
        return loader_errors::http_error;
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

    link_metainfo_ = preview_proxy::parse_json(json.data(), uri_);
    if (!link_metainfo_)
    {
        return loader_errors::invalid_json;
    }

    response->reset_out();
    response->save_2_file(meta_local_path_);

    __INFO(
        "snippets",
        "task succeed\n"
        "    type=<link_metainfo>\n"
        "    id=<%1%>\n"
        "    uri=<%2%>\n"
        "    source=<snippets server>",
        get_id() % uri_);

    return loader_errors::success;
}

const preview_proxy::link_meta& link_metainfo_download_task::get_link_metainfo() const
{
    assert(link_metainfo_);

    return *link_metainfo_;
}

bool link_metainfo_download_task::load_from_cache(disk_cache::disk_cache &_cache)
{
    return false;
}

CORE_WIM_NS_END