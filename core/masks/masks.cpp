#include "stdafx.h"

#include "../async_task.h"
#include "../core.h"
#include "../../corelib/collection_helper.h"

#include "../tools/strings.h"
#include "../tools/system.h"

#include "../connections/wim/wim_im.h"
#include "../connections/wim/wim_packet.h"
#include "../connections/wim/loader/tasks_runner_slot.h"
#include "../connections/wim/loader/loader.h"

#include "masks.h"

core::masks::masks(std::weak_ptr<wim::im> _im, const std::wstring& _root_path, unsigned _version)
    : im_(_im)
    , root_(_root_path)
    , version_(_version)
    , state_(state::not_loaded)
{
    init();
}

void core::masks::init()
{
    if (!tools::system::is_exist(root_))
    {
        if (!tools::system::create_directory(root_))
            return;
    }
    else
    {
        for(auto& entry : boost::make_iterator_range(
            boost::filesystem::directory_iterator(root_), boost::filesystem::directory_iterator()))
        {
            const auto n = tools::to_uint64(entry.path().filename().wstring());
            if (n != version_)
                boost::filesystem::remove_all(entry);
        }
    }

    tools::system::create_directory_if_not_exists(get_working_dir());
}

boost::filesystem::path core::masks::get_working_dir() const
{
    return root_ / std::to_string(version_);
}

boost::filesystem::path core::masks::get_json_path() const
{
    return get_working_dir() / ".info";
}

boost::filesystem::path core::masks::get_mask_dir(const std::string& _name) const
{
    return get_working_dir() / _name;
}

boost::filesystem::path core::masks::get_mask_version_path(const std::string& _name) const
{
    return get_mask_dir(_name) / ".version";
}

boost::filesystem::path core::masks::get_mask_preview_path(const std::string& _name, const mask_info& _info) const
{
    const auto file_name = boost::filesystem::path(_info.preview_).filename();
    return get_mask_dir(_name) / file_name;
}

boost::filesystem::path core::masks::get_mask_archive_path(const std::string& _name, const mask_info& _info) const
{
    const auto file_name = boost::filesystem::path(_info.archive_).filename();
    return get_mask_dir(_name) / file_name;
}

boost::filesystem::path core::masks::get_mask_content_dir(const std::string& _name) const
{
    return get_mask_dir(_name) / "content";
}

boost::filesystem::path core::masks::get_mask_json_path(const std::string& _name) const
{
    return get_mask_content_dir(_name) / "mask.json";
}

boost::filesystem::path core::masks::get_model_dir() const
{
    return get_working_dir() / ".model";
}

boost::filesystem::path core::masks::get_model_version_path() const
{
    return get_model_dir() / ".version";
}

boost::filesystem::path core::masks::get_model_sentry_path() const
{
    return get_model_dir() / ".done";
}

boost::filesystem::path core::masks::get_model_archive_path() const
{
    return get_working_dir() / ".model.zip";
}

bool core::masks::save_version(const boost::filesystem::path& _path, uint64_t _version)
{
    auto file = std::ofstream(_path.string());
    if (!file)
        return false;

    file << _version;
    return true;
}

void core::masks::post_message_to_gui(int64_t _seq, const std::string& _message) const
{
    coll_helper coll(g_core->create_collection(), true);
    g_core->post_message_to_gui(_message.c_str(), _seq, coll.get());
}

void core::masks::post_message_to_gui(int64_t _seq, const std::string& _message, const boost::filesystem::path& _local_path) const
{
    coll_helper coll(g_core->create_collection(), true);
    coll.set_value_as_string("local_path", tools::from_utf16(_local_path.wstring()));
    g_core->post_message_to_gui(_message.c_str(), _seq, coll.get());
}

void core::masks::get_mask_id_list(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    state_ = state::loading;

    if (im->has_created_call())
    {
        post_message_to_gui(_seq, "masks/update/retry");
        return;
    }

    const std::string json_url = "https://www.icq.com/masks/list/v" + std::to_string(version_);
    const auto json_path = get_json_path().wstring();

    im->get_loader().download_file(json_url, json_path, true, im->make_wim_params())->on_result_ = 
        [this, im, _seq, json_path](int32_t _error)
        {
            if (_error)
            {
                state_ = state::not_loaded;
                return;
            }

            if (im->has_created_call())
            {
                post_message_to_gui(_seq, "masks/update/retry");
                return;
            }

            std::string source;
            if (!tools::system::read_file(json_path, source))
                return;

            rapidjson::Document json;
            if (json.Parse(source.c_str()).HasParseError())
                return;

            mask_by_name_.clear();
            mask_list_.clear();

            base_url_ = json["base_url"].GetString();

            const auto& masks = json["masks"];
            for (auto i = 0u, size = masks.Size(); i != size; ++i)
            {
                const auto& elem = masks[i];

                const auto name = elem["name"].GetString();
                const auto last_modified = elem["lastModified"].GetUint64();

                mask_by_name_[name] = mask_list_.size();
                mask_list_.emplace_back(
                    name, elem["archive"].GetString(), elem["image"].GetString(), last_modified);

                const auto mask_dir = get_mask_dir(name);
                if (!tools::system::is_exist(mask_dir))
                {
                    if (tools::system::create_directory(mask_dir))
                    {
                        save_version(get_mask_version_path(name), last_modified);
                    }
                }
                else
                {
                    std::string source;
                    if (tools::system::read_file(get_mask_version_path(name), source))
                    {
                        const auto version = tools::to_uint64(source);
                        if (version < last_modified)
                        {
                            tools::system::clean_directory(mask_dir);
                            save_version(get_mask_version_path(name), last_modified);
                        }
                    }
                }
            }

            coll_helper coll(g_core->create_collection(), true);
            core::ifptr<core::iarray> arr(coll->create_array());

            for (const auto& mask : mask_list_)
            {
                core::ifptr<core::ivalue> val(coll->create_value());
                const auto& name = mask.name_;
                val->set_as_string(name.c_str(), name.length());
                arr->push_back(val.get());
            }

            coll.set_value_as_array("mask_id_list", arr.get());
            g_core->post_message_to_gui("masks/get_id_list/result", _seq, coll.get());

            const auto& model = json["model"];
            const auto last_modified = model["lastModified"].GetUint64();
            const auto model_dir = get_model_dir();
            if (!tools::system::is_exist(model_dir))
            {
                if (tools::system::create_directory(model_dir))
                {
                    save_version(get_model_version_path(), last_modified);
                }
            }
            else
            {
                std::string source;
                if (tools::system::read_file(get_model_version_path(), source))
                {
                    const auto version = tools::to_uint64(source);
                    if (version < last_modified)
                    {
                        tools::system::clean_directory(model_dir);
                        save_version(get_model_version_path(), last_modified);
                    }
                }
            }

            model_url_ = base_url_ + model["archive"].GetString();
        };
}

void core::masks::get_mask_preview(int64_t _seq, const std::string& mask_id)
{
    auto im = im_.lock();
    if (!im)
        return;

    const auto mask_pos = mask_by_name_.find(mask_id);
    if (mask_pos == mask_by_name_.end())
    {
        assert(!"call get_mask_id_list first!");
        return;
    }

    const auto& mask = mask_list_[mask_pos->second];
    const auto preview_path = get_mask_preview_path(mask_id, mask);
    if (tools::system::is_exist(preview_path))
    {
        post_message_to_gui(_seq, "masks/preview/result", preview_path);
    }
    else
    {
        const std::string preview_url = base_url_ + mask.preview_;

        im->get_loader().download_file(preview_url, preview_path.wstring(), true, im->make_wim_params())->on_result_ =
            [this, _seq, preview_path](int32_t _error)
            {
                if (_error)
                    return;

                post_message_to_gui(_seq, "masks/preview/result", preview_path);
            };
    }
}

void core::masks::get_mask_model(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    if (!tools::system::is_exist(get_model_sentry_path()))
    {
        im->get_loader().download_file(model_url_, get_model_archive_path().wstring(), true, im->make_wim_params())->on_result_ =
            [this, _seq](int32_t _error)
            {
                if (_error)
                    return;

                if (!tools::system::unzip(get_model_archive_path(), get_model_dir()))
                    return;

                tools::system::delete_file(get_model_archive_path().wstring());

                std::ofstream sentry_file(get_model_sentry_path().string());

                on_model_loading(_seq);
            };
    }
    else
    {
        on_model_loading(_seq);
    }
}

void core::masks::get_mask(int64_t _seq, const std::string& mask_id)
{
    auto im = im_.lock();
    if (!im)
        return;

    const auto mask_pos = mask_by_name_.find(mask_id);
    if (mask_pos == mask_by_name_.end())
    {
        assert(!"call get_mask_id_list first!");
        return;
    }

    const auto json_path = get_mask_json_path(mask_id);
    if (tools::system::is_exist(json_path))
    {
        post_message_to_gui(_seq, "masks/get/result", json_path);
    }
    else
    {
        const auto& mask = mask_list_[mask_pos->second];
        const std::string archive_url = base_url_ + mask.archive_;
        const auto archive_path = get_mask_archive_path(mask_id, mask);

        auto progress = [_seq](int64_t /*_bytes_total*/, int64_t /*_bytes_transferred*/, int32_t _percent)
            {
                coll_helper coll(g_core->create_collection(), true);
                coll.set_value_as_uint("percent", _percent);
                g_core->post_message_to_gui("masks/progress", _seq, coll.get());
            };

        im->get_loader().download_file(archive_url, archive_path.wstring(), true, im->make_wim_params(), progress)->on_result_ =
            [this, _seq, mask_id, archive_path, json_path](int32_t _error)
            {
                if (_error)
                    return;

                if (!tools::system::unzip(archive_path, get_mask_content_dir(mask_id)))
                    return;

                tools::system::delete_file(archive_path.wstring());

                post_message_to_gui(_seq, "masks/get/result", json_path);
            };
    }
}

void core::masks::on_model_loading(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    const auto path = tools::from_utf16(get_model_dir().wstring());
    im->voip_set_model_path(path);

    coll_helper coll(g_core->create_collection(), true);
    g_core->post_message_to_gui("masks/model/result", _seq, coll.get());

    state_ = state::loaded;
}

void core::masks::on_connection_restored()
{
#ifndef __linux__
    if (state_ == state::not_loaded)
        post_message_to_gui(0, "masks/update/retry");
#endif //__linux__
}

core::masks::mask_info::mask_info()
    : downloaded_(false)
{
}

core::masks::mask_info::mask_info(
    const std::string& _name, const std::string& _archive, const std::string& _preview, time_t _last_modified)
    : name_(_name)
    , downloaded_(false)
    , archive_(_archive)
    , preview_(_preview)
    , last_modified_(_last_modified)
{
}
