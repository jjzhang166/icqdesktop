#include "stdafx.h"

#include "../../../../corelib/collection_helper.h"

#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../log/log.h"
#include "../../../tools/strings.h"
#include "../../../utils.h"

#include "loader_errors.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "tasks_runner_slot.h"

#include "image_download_task.h"

CORE_WIM_NS_BEGIN

namespace
{
    bool should_use_anonymizer(const std::string &_uri);
}

image_download_task::image_download_task(
    const int64_t _id,
    const std::string& _contact_aimid,
    const wim_packet_params &_params,
    const proxy_settings &_proxy_settings,
    const std::string& _image_uri,
    const std::wstring& _image_local_path,
    const std::wstring& _cache_dir,
    disk_cache::disk_cache_sptr &_cache,
    const bool _sign_url,
    std::shared_ptr<download_image_handler> _result_handler)
    : generic_loader_task(_id, _contact_aimid, _params, _proxy_settings, _cache)
    , image_uri_(_image_uri)
    , image_local_path_(_image_local_path)
    , result_handler_(_result_handler)
    , sign_url_(_sign_url)
    , cache_dir_(_cache_dir)
{
    assert(!image_local_path_.empty());
    assert(!image_uri_.empty());
    assert(!cache_dir_.empty());
}

image_download_task::~image_download_task()
{
}

loader_errors image_download_task::run()
{
    assert(!image_preview_data_);
    image_preview_data_ = std::make_shared<tools::binary_stream>();

    __INFO(
        "snippets",
        "runnning task\n"
        "    type=<image>\n"
        "    id=<%1%>\n"
        "    uri=<%2%>",
        get_id() % image_uri_);

    if (image_preview_data_->load_from_file(image_local_path_))
    {
        __INFO(
            "snippets",
            "task succeed\n"
            "    type=<image>\n"
            "    id=<%1%>\n"
            "    uri=<%2%>\n"
            "    cache=<%3%>\n"
            "    source=<cache>",
            get_id() % image_uri_ % tools::from_utf16(image_local_path_));

        return loader_errors::success;
    }

    const auto progress_seq = get_id();
    assert(progress_seq > 0);

    const auto progress_handler =
        [progress_seq]
        (int64_t _bytes_total, int64_t _bytes_transferred, int32_t _pct_tranferred)
        {
            assert(_bytes_total > 0);
            assert(_bytes_transferred >= 0);
            assert(_pct_tranferred >= 0);
            assert(_pct_tranferred <= 100);

            g_core->excute_core_context(
                [progress_seq, _bytes_total, _bytes_transferred, _pct_tranferred]
                {
                    coll_helper coll(g_core->create_collection(), true);

                    coll.set<int64_t>("bytes_total", _bytes_total);
                    coll.set<int64_t>("bytes_transferred", _bytes_transferred);
                    coll.set<int32_t>("pct_transferred", _pct_tranferred);

                    g_core->post_message_to_gui("image/download/progress", progress_seq, coll.get());
                });
        };

    const auto stop_handler = std::bind(&generic_loader_task::is_cancelled, this);

    core::http_request_simple request(get_proxy_settings(), utils::get_user_agent(), stop_handler, progress_handler);

    auto request_uri = get_patched_image_uri();

    const auto should_anonymize = should_use_anonymizer(request_uri);
    if (should_anonymize)
    {
        const auto ext_params = preview_proxy::format_get_url_content_params(request_uri);
        request_uri = sign_loader_uri(preview_proxy::uri::get_url_content(), get_wim_params(), ext_params);
    }

    request.set_url(request_uri);
    request.set_need_log(false);
    request.set_connect_timeout(1000);

    if (!request.get())
    {
        if (is_cancelled())
        {
            return loader_errors::cancelled;
        }

        return loader_errors::network_error;
    }

    if (request.get_response_code() != 200)
    {
        return loader_errors::http_error;
    }

    image_preview_data_->swap(*request.get_response());

    if (!image_preview_data_->save_2_file(image_local_path_))
    {
        return loader_errors::save_2_file;
    }

    image_preview_data_->reset_out();

    __INFO(
        "snippets",
        "task succeed\n"
        "    type=<image>\n"
        "    id=<%1%>\n"
        "    image-uri=<%2%>\n"
        "    request-uri=<%3%>\n"
        "    saved-to=<%4%>\n"
        "    anonymizer=<%5%>\n"
        "    source=<uri>",
        get_id() %
        image_uri_ %
        request_uri %
        tools::from_utf16(image_local_path_) %
        logutils::yn(should_anonymize));

    return loader_errors::success;
}

std::string image_download_task::to_log_str() const
{
    std::string result;
    result.reserve(512);

    result += "image(";
    result += image_uri_;
    result += ")";

    return result;
}

tools::binary_stream& image_download_task::get_image_preview_data()
{
    assert(image_preview_data_);

    return *image_preview_data_;
}

tasks_runner_slot image_download_task::get_slot() const
{
    assert(!image_local_path_.empty());

    const auto uri_hash = std::hash<std::string>()(image_uri_);

    const auto slot_index = (uri_hash % 3);

    static const tasks_runner_slot slots[] = {
        tasks_runner_slot::images_a,
        tasks_runner_slot::images_b,
        tasks_runner_slot::images_c
    };

    return slots[slot_index];
}

void image_download_task::on_before_suspend()
{
    image_preview_data_.reset();

    generic_loader_task::on_before_suspend();
}

void image_download_task::on_result(const loader_errors _error)
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

std::string image_download_task::get_patched_image_uri() const
{
    // workarounds location for various uri issues is here

    assert(!image_uri_.empty());

    auto result = image_uri_;

    const auto is_dropbox = (
        (result.find("dropbox.com/") != std::string::npos) &&
        boost::ends_with(result, "?dl=0"));
    if (is_dropbox)
    {
        boost::replace_last(result, "?dl=0", "?dl=1");
    }

    return result;
}

namespace
{
    bool should_use_anonymizer(const std::string &_uri)
    {
        if (boost::algorithm::contains(_uri, "files.icq.com/preview"))
        {
            return false;
        }

        if (boost::algorithm::contains(_uri, "imgsmail.ru/imgpreview"))
        {
            return false;
        }

        if (boost::algorithm::starts_with(_uri, "https://www.dropbox.com/"))
        {
            return false;
        }

        return true;
    }
}

CORE_WIM_NS_END