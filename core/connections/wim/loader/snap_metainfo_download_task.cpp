#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../log/log.h"
#include "../../../utils.h"

#include "loader_errors.h"
#include "loader_helpers.h"
#include "snap_metainfo.h"
#include "tasks_runner_slot.h"

#include "snap_metainfo_download_task.h"

CORE_WIM_NS_BEGIN

snap_metainfo_download_task::snap_metainfo_download_task(
    const int64_t _id,
    const std::string& _contact_aimid,
    const wim_packet_params &_params,
    const proxy_settings &_proxy_settings,
    const std::string &_ttl_id,
    const std::wstring &_cache_dir,
    disk_cache::disk_cache_sptr &_cache,
    std::shared_ptr<download_snap_metainfo_handler> _result_handler)
    : generic_loader_task(_id, _contact_aimid, _params, _proxy_settings, _cache)
    , ttl_id_(_ttl_id)
    , result_handler_(_result_handler)
    , cache_dir_(_cache_dir)
{
    assert(!ttl_id_.empty());
    assert(result_handler_);
}

tasks_runner_slot snap_metainfo_download_task::get_slot() const
{
    assert(!ttl_id_.empty());

    const auto id_hash = std::hash<std::string>()(ttl_id_);

    const auto is_slot_a = ((id_hash % 2) == 0);

    return
        is_slot_a ?
            tasks_runner_slot::snap_metainfo_a :
            tasks_runner_slot::snap_metainfo_b;
}

void snap_metainfo_download_task::on_result(const loader_errors _error)
{
    assert(result_handler_);
    assert(result_handler_->on_result);

    const auto success = (_error == loader_errors::success);

    assert(!success || snap_metainfo_);

    if (!result_handler_ || !result_handler_->on_result)
    {
        return;
    }

    result_handler_->on_result((int32_t)_error, snap_metainfo_);
}

loader_errors snap_metainfo_download_task::run()
{
    const auto cached_meta_path = get_path_in_cache(cache_dir_, ttl_id_, path_type::snap_meta);
    assert(!cached_meta_path.empty());

    snap_metainfo_ = load_snap_meta_from_file(cached_meta_path, ttl_id_);
    if (snap_metainfo_)
    {
        __TRACE(
            "snaps",
            "snap metainfo successfully loaded\n"
            "    source=<cache>\n"
            "    ttl_id=<%1%>",
            ttl_id_);

        return loader_errors::success;
    }

    const auto &wim_params = get_wim_params();
    const auto &proxy_settings = get_proxy_settings();

    core::http_request_simple request(
        proxy_settings,
        utils::get_user_agent(wim_params.aimid_),
        wim_params.stop_handler_);

    const auto ext_params = snaps::format_get_metainfo_params(ttl_id_);
    const auto signed_request_uri = sign_loader_uri(snaps::uri::get_metainfo(), wim_params, ext_params);
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

    snap_metainfo_ = snaps::parse_json(json.data());
    if (!snap_metainfo_)
    {
        return loader_errors::invalid_json;
    }

    response->reset_out();
    response->save_2_file(cached_meta_path);

    return loader_errors::success;
}

CORE_WIM_NS_END