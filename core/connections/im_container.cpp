#include "stdafx.h"
#include "im_container.h"
#include "login_info.h"
#include "im_login.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "wim/wim_im.h"
#include "wim/wim_packet.h"
#include "../Voip/VoipManager.h"

#include "../core.h"
#include "../utils.h"
#include "../archive/contact_archive.h"
#include "search_contacts_params.h"
#include "../statistics.h"
#include "../themes/themes.h"
#include "../proxy_settings.h"

using namespace core;

im_container::im_container(voip_manager::VoipManager& voip_manager)
    :   voip_manager_(voip_manager),
    logins_(new im_login_list(utils::get_product_data_path() + L"/settings/ims.stg"))
{
}


im_container::~im_container()
{
}

void core::im_container::create()
{
    if (create_ims())
    {
        connect_ims();
    }
}

void im_container::connect_ims()
{
    for (auto iter = ims_.begin(); iter < ims_.end(); iter++)
        (*iter)->connect();
}

void im_container::update_login(im_login_id& _login)
{
    logins_->update(_login);
}

bool core::im_container::create_ims()
{
    if (logins_->load())
    {
    }

    im_login_id login("", default_im_id);

    if (logins_->get_first_login(login))
    {
    }

    ims_.push_back(std::make_shared<wim::im>(login, voip_manager_));

    return !ims_.empty();
}

std::string core::im_container::get_first_login() const
{
    im_login_id login("", default_im_id);
    logins_->get_first_login(login);
    return login.get_login();
}

void core::im_container::on_message_from_gui(const char * _message, int64_t _seq, coll_helper& _params)
{
    std::string message_string = _message;

    if (message_string == "login_by_password")
        on_login_by_password(_seq, _params);
    else if (message_string == "login_get_sms_code")
        on_login_get_sms_code(_seq, _params);
    else if (message_string == "login_by_phone")
        on_login_by_phone(_seq, _params);
    else if (message_string == "logout")
        on_logout();
    else if (message_string == "connect_after_migration")
        on_connect_after_migration();
    else if (message_string == "avatars/get")
        on_get_contact_avatar(_seq, _params);
    else if (message_string == "send_message")
        on_send_message(_seq, _params);
    else if (message_string == "message/typing")
        on_message_typing(_seq, _params);
    else if (message_string == "feedback/send")
        on_feedback(_seq, _params);
    else if (message_string == "set_state")
        on_set_state(_seq, _params);
    else if (message_string == "archive/index/get")
        on_get_archive_index(_seq, _params);
    else if (message_string == "archive/buddies/get")
        on_get_archive_messages_buddies(_seq, _params);
    else if (message_string == "archive/messages/get")
        on_get_archive_messages(_seq, _params);
    else if (message_string == "search")
        on_search(_seq, _params);
    else if (message_string == "dialogs/add")
        on_add_opened_dialog(_seq, _params);
    else if (message_string == "dialogs/remove")
        on_remove_opened_dialog(_seq, _params);
    else if (message_string == "dialogs/set_first_message")
        on_set_first_message(_seq, _params);
    else if (message_string == "dialogs/hide")
        on_hide_chat(_seq, _params);
    else if (message_string == "dialogs/mute")
        on_mute_chat(_seq, _params);
    else if (message_string == "dlg_state/set_last_read")
        on_set_last_read(_seq, _params);
    else if (message_string == "voip_call")
        on_voip_call_message(_seq, _params);
    else if (message_string == "files/upload")
        on_upload_file_sharing(_seq, _params);
    else if (message_string == "files/upload/abort")
        on_abort_file_sharing_uploading(_seq, _params);
    else if (message_string == "files/download")
        on_download_file(_seq, _params);
    else if (message_string == "files/download/abort")
        on_abort_file_downloading(_seq, _params);
    else if (message_string == "preview/download")
        on_download_preview(_seq, _params);
    else if (message_string == "stickers/meta/get")
        on_get_stickers_meta(_seq, _params);
    else if (message_string == "stickers/sticker/get")
        on_get_sticker(_seq, _params);
    else if (message_string == "chats/info/get")
        on_get_chat_info(_seq, _params);
    else if (message_string == "contacts/search")
        on_search_contacts(_seq, _params);
    else if (message_string == "contacts/search2")
        on_search_contacts2(_seq, _params);
    else if (message_string == "contacts/profile/get")
        on_profile(_seq, _params);
    else if (message_string == "contacts/add")
        on_add_contact(_seq, _params);
    else if (message_string == "contacts/remove")
        on_remove_contact(_seq, _params);
    else if (message_string == "contacts/block")
        on_spam_contact(_seq, _params);
    else if (message_string == "contacts/ignore")
        on_ignore_contact(_seq, _params);
    else if (message_string == "contacts/get_ignore")
        on_get_ignore_contacts(_seq, _params);
    else if (message_string == "dlg_state/hide")
        on_hide_dlg_state(_seq, _params);
    else if (message_string == "remove_members")
        on_remove_members(_seq, _params);
    else if (message_string == "add_members")
        on_add_members(_seq, _params);
    else if (message_string == "add_chat")
        on_add_chat(_seq, _params);
    else if (message_string == "modify_chat")
        on_modify_chat(_seq, _params);
    else if (message_string == "sign_url")
        on_sign_url(_seq, _params);
    else if (message_string == "stats")
        on_stats(_seq, _params);
    else if (message_string == "themes/meta/get")
        on_get_themes_meta(_seq, _params);
    else if (message_string == "themes/theme/get")
        on_get_theme(_seq, _params);
    else if (message_string == "files/set_url_played")
        on_url_played(_seq, _params);
    else if (message_string == "files/speech_to_text")
        on_speech_to_text(_seq, _params);
    else if (message_string == "favorite")
        on_favorite(_seq, _params);
    else if (message_string == "unfavorite")
        on_unfavorite(_seq, _params);
    else if (message_string == "load_flags")
        on_get_flags(_seq, _params);
}

void core::im_container::fromInternalProxySettings2Voip(const core::proxy_settings& proxySettings, voip_manager::VoipProxySettings& voipProxySettings) {
    using namespace voip_manager;

    if (!proxySettings.use_proxy_) {
        voipProxySettings.type          = VoipProxySettings::kProxyType_None;
    } else {
        switch (proxySettings.proxy_type_) {
        case 0:  voipProxySettings.type = VoipProxySettings::kProxyType_Http;
        case 4:  voipProxySettings.type = VoipProxySettings::kProxyType_Socks4;
        case 5:  voipProxySettings.type = VoipProxySettings::kProxyType_Socks5;
        case 6:  voipProxySettings.type = VoipProxySettings::kProxyType_Socks4a;
        default: voipProxySettings.type = VoipProxySettings::kProxyType_None;
        }
    }

    voipProxySettings.serverUrl    = proxySettings.proxy_server_;
    voipProxySettings.userName     = proxySettings.login_;
    voipProxySettings.userPassword = proxySettings.password_;
}

void core::im_container::on_voip_call_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    const std::string type = _params.get_value_as_string("type");
    if (!im && type != "voip_reset") {
        return;
    }

	if (type == "voip_call_start") {
		const std::string call_type = _params.get_value_as_string("call_type");
		const std::string mode      = _params.get_value_as_string("mode");
        const std::string contact   = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_start(contact, call_type == "video", mode == "attach");
        };
	} else if (type == "voip_add_window") {
	    voip_manager::WindowParams windowParams;
        windowParams.hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        windowParams.isPrimary = _params.get_value_as_bool("mode");
        windowParams.isSystem  = _params.get_value_as_bool("system_mode");
        windowParams.scale     = (float)_params.get_value_as_double("scale");

        windowParams.watermark.bitmap.data = NULL;
        windowParams.watermark.bitmap.size = 0;
        windowParams.watermark.bitmap.w    = 0;
        windowParams.watermark.bitmap.h    = 0;

        core::istream* stream = NULL;
        if (_params.is_value_exist("watermark")) {
            stream = _params.get_value_as_stream("watermark");
            const auto h = _params.get_value_as_int   ("watermark_height");
            const auto w = _params.get_value_as_int   ("watermark_width");
            assert(stream);
            if (stream) {
                const auto stream_size = stream->size();
                assert(stream_size);
                if (stream_size > 0 && h > 0 && w > 0) {
                    windowParams.watermark.hwnd = windowParams.hwnd;
                    windowParams.watermark.bitmap.data = (void*)stream->read(stream->size());
                    windowParams.watermark.bitmap.size = stream_size;
                    windowParams.watermark.bitmap.w = w;
                    windowParams.watermark.bitmap.h = h;
                }
            }
        }
        im->on_voip_add_window(windowParams);
        if (stream) {
            stream->reset();
        }
    } else if (type == "voip_remove_window") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        im->on_voip_remove_window(hwnd);
    } else if (type == "voip_call_stop") {
        im->on_voip_call_stop();
    } else if (type == "voip_call_volume_change") {
        const int vol = _params.get_value_as_int("volume");
        im->on_voip_volume_change(vol);
    } else if (type == "audio_playback_mute_switch") {
        im->on_voip_mute_switch();
    } else if (type == "voip_call_media_switch") {
        const bool video = _params.get_value_as_bool("video");
        im->on_voip_switch_media(video);
    } else if (type == "voip_sounds_mute") {
        const bool mute = _params.get_value_as_bool("mute");
        im->on_voip_mute_incoming_call_sounds(mute);
    } else if (type == "voip_call_decline") {
        const std::string mode = _params.get_value_as_string("mode");
        im->on_voip_call_end(_params.get_value_as_string("contact"), mode == "busy");
    } else if (type == "converted_avatar") {
        on_voip_avatar_msg(im, _params);
    } else if (type == "backgroung_update") {
        on_voip_background_msg(im, _params);
	} else if (type == "voip_call_accept") {
		const std::string mode    = _params.get_value_as_string("mode");
        const std::string contact = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_accept(contact, mode == "video");
        }
	} else if (type == "device_change") {
		const std::string dev_type = _params.get_value_as_string("dev_type");
		const std::string uid = _params.get_value_as_string("uid");
		im->on_voip_device_changed(dev_type, uid);
	} else if (type == "request_calls") {
		im->on_voip_call_request_calls();
	} else if (type == "update") {
		im->on_voip_update();
    } else if (type == "voip_set_window_offsets") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        int l = _params.get_value_as_int("left");
        int t = _params.get_value_as_int("top");
        int r = _params.get_value_as_int("right");
        int b = _params.get_value_as_int("bottom");

        im->on_voip_window_set_offsets(hwnd, l, t, r, b);
    } else if (type == "voip_reset") {
        if (!im) {
#ifndef STRIP_VOIP
            voip_manager_.get_voip_manager()->reset();
#endif
        } else {
            im->on_voip_reset();
        }
    } else {
        assert(false);
    }
}

void core::im_container::on_voip_background_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    core::istream* stream = _params.get_value_as_stream("background");
    const auto h          = _params.get_value_as_int("background_height");
    const auto w          = _params.get_value_as_int("background_width");
    void* hwnd            = (void*)(uintptr_t)_params.get_value_as_int("window_handle");

    assert(stream);
    if (stream) {
        const auto stream_size = stream->size();

        assert(stream_size);
        if (stream_size > 0 && h > 0 && w > 0) {
            im->on_voip_window_update_background(hwnd, stream->read(stream->size()), stream_size, w, h);
            stream->reset();
        }
    }
}

void core::im_container::on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    typedef void (base_im::*__loader_func)(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w);

    auto __load_avatar = [&_params, &im] (const std::string& prefix, __loader_func func) {
        const std::string contact = _params.get_value_as_string("contact");
        const int size = _params.get_value_as_int("size");

        assert(!contact.empty() && size);
        if (contact.empty() || !size) {
            return;
        }

        assert(!!im);
        if (!im) {
            return;
        }

        if (!_params.is_value_exist((prefix + "avatar").c_str())) {
            return;
        }

        core::istream* stream = _params.get_value_as_stream((prefix + "avatar").c_str());
        const auto h          = _params.get_value_as_int((prefix + "avatar_height").c_str());
        const auto w          = _params.get_value_as_int((prefix + "avatar_width").c_str());

        assert(stream);
        if (stream) {
            const auto stream_size = stream->size();

            assert(stream_size);
            if (stream_size > 0) {
                core::base_im& ptr = *im.get();
                (ptr.*func)(contact, stream->read(stream->size()), stream_size, h, w);
                stream->reset();
            }
        }
    };

    __load_avatar("",                     &base_im::on_voip_user_update_avatar);
    __load_avatar("rem_camera_offline_",  &base_im::on_voip_user_update_avatar_no_video);
    __load_avatar("sign_normal_",         &base_im::on_voip_user_update_avatar_text);
    __load_avatar("sign_header_",         &base_im::on_voip_user_update_avatar_text_header);
    __load_avatar("loc_camera_offline_",  &base_im::on_voip_user_update_avatar_camera_off);
    __load_avatar("loc_camera_disabled_", &base_im::on_voip_user_update_avatar_no_camera);
}

void im_container::on_get_archive_index(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_archive_index(_seq, _params.get_value_as_string("contact"), -1, -1);
}


void im_container::on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto ids_list = std::make_shared<core::archive::msgids_list>();

    core::ifptr<core::ihheaders_list> val_headers(_params.get_value_as_hheaders("ids"), false);

    for (auto hdr = val_headers->first(); hdr; hdr = val_headers->next())
        ids_list->push_back(hdr->id_);


    im->get_archive_messages_buddies(_seq, _params.get_value_as_string("contact"), ids_list);
}


void im_container::on_get_archive_messages(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_archive_messages(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_int64("from"),
        _params.get_value_as_int64("count"));
}

void core::im_container::on_message_typing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    im->send_message_typing(_seq, _params.get_value_as_string("contact"));
}

void core::im_container::on_send_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto type = message_type::base;

    std::string message;
    if (_params->is_value_exist("message"))
        message = _params.get_value_as_string("message");

    bool is_sms = false;
    if (_params->is_value_exist("is_sms"))
    {
        is_sms = _params.get_value_as_bool("is_sms");
        type = message_type::sms;
    }

    bool is_sticker = false;
    if (_params->is_value_exist("sticker"))
    {
        is_sticker = _params.get_value_as_bool("sticker");
        if (is_sticker)
        {
            std::stringstream ss_message;
            ss_message << "ext:" << _params.get_value_as_int("sticker/set_id") << ":" << "sticker:" << _params.get_value_as_int("sticker/id");

            message = ss_message.str();
        }

        type = message_type::sticker;
    }

    assert(!(is_sms && is_sticker));

    im->send_message_to_contact(
        _seq,
        _params.get_value_as_string("contact"),
        message,
        type,
        "");
}

void core::im_container::on_feedback(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string url;
    std::map<std::string, std::string> fields;
    std::vector<std::string> files;

    url = _params.get_value_as_string("url");

    fields.insert(std::make_pair("fb.screen_resolution", _params.get_value_as_string("fb.screen_resolution")));
    fields.insert(std::make_pair("fb.referrer", _params.get_value_as_string("fb.referrer")));
    fields.insert(std::make_pair("fb.question.3004", _params.get_value_as_string("fb.question.3004")));
    fields.insert(std::make_pair("fb.question.159", _params.get_value_as_string("fb.question.159")));
    fields.insert(std::make_pair("fb.question.178", _params.get_value_as_string("fb.question.178")));
    fields.insert(std::make_pair("fb.question.3005", _params.get_value_as_string("fb.question.3005")));
    fields.insert(std::make_pair("fb.user_name", _params.get_value_as_string("fb.user_name")));
    fields.insert(std::make_pair("fb.message", _params.get_value_as_string("fb.message")));
    fields.insert(std::make_pair("fb.communication_email", _params.get_value_as_string("fb.communication_email")));
    fields.insert(std::make_pair("Lang", _params.get_value_as_string("Lang")));
    fields.insert(std::make_pair("attachements_count", _params.get_value_as_string("attachements_count")));

    if (_params.is_value_exist("fb.attachement"))
    {
        core::iarray* array = _params.get_value_as_array("fb.attachement");
        for (int i = 0; i < array->size(); ++i)
        {
            files.push_back(array->get_at(i)->get_as_string());
        }
    }

    im->send_feedback(_seq, url, fields, files);
}

void core::im_container::on_set_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto state = profile_state::online;
    std::string sstate = _params.get_value_as_string("state");
    if (sstate == "dnd")
        state = profile_state::dnd;
    else if (sstate == "invisible")
        state = profile_state::invisible;

    im->set_state(_seq, state);
}

void core::im_container::on_remove_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_remove"));
}

void core::im_container::on_add_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_add"));
}

void core::im_container::on_add_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> members;

    auto member_array = _params.get_value_as_array("m_chat_members");
    for (int i = 0; member_array && i < member_array->size(); ++i)
    {
        members.push_back(member_array->get_at(i)->get_as_string());
    }

    im->add_chat(
        _seq,
        _params.get_value_as_string("m_chat_name"),
        members);
}

void core::im_container::on_modify_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->modify_chat(
        _seq,
        _params.get_value_as_string("aimid"), _params.get_value_as_string("m_chat_name"));
}

void core::im_container::on_login_by_password(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));

    auto is_login = _params.get_value_as_bool("is_login");


    if (is_login)
    {
        std::shared_ptr<base_im> im = std::make_shared<wim::im>(im_login_id(""), voip_manager_);
        ims_.clear();
        im->login(_seq, info);
        ims_.push_back(im);
    }
    else
    {
        auto from_im = get_im(_params);
        from_im->start_attach_uin(_seq, info, from_im->make_wim_params());
    }
}

void core::im_container::on_login_get_sms_code(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));

    std::shared_ptr<base_im> im;

    auto is_login = _params.get_value_as_bool("is_login");
    if (is_login)
    {
        ims_.clear();
        im = std::make_shared<wim::im>(im_login_id(""), voip_manager_);
    }
    else
    {
        im = get_im(_params);
    }

    im->login_normalize_phone(_seq, _params.get_value_as_string("country"), _params.get_value_as_string("phone"), _params.get_value_as_string("locale"), is_login);

    if (is_login)
        ims_.push_back(im);
}

void core::im_container::on_login_by_phone(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));
    info.set_sms_code(_params.get_value_as_string("sms_code"));

    if (ims_.empty())
    {
        assert(!"ims empty");
        return;
    }

    bool is_login = _params.get_value_as_bool("is_login");
    if (is_login)
        (*ims_.begin())->login_by_phone(_seq, info);
    else
        (*ims_.begin())->start_attach_phone(_seq, info);
}

void core::im_container::on_connect_after_migration()
{
    create();
}

void core::im_container::on_logout()
{
    assert(!ims_.empty());
    if (ims_.empty())
        return;

    std::weak_ptr<core::im_container> wr_this(shared_from_this());

    auto __onlogout = [wr_this] ()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->start_session_stats();
        g_core->post_message_to_gui("need_login", 0, nullptr);

        (*ptr_this->ims_.begin())->erase_auth_data();

        ptr_this->ims_.clear();
    };

#ifndef STRIP_VOIP
    if (voip_manager_.get_call_manager()->call_get_count())
#else
    if (0)
#endif
    {
        auto __doLogout = [wr_this, __onlogout] ()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }
            (*ptr_this->ims_.begin())->logout(__onlogout);;
        }; 
#ifndef STRIP_VOIP
        voip_manager_.get_call_manager()->call_stop_smart(__doLogout);
#endif //STRIP_VOIP
    }
    else
    {
        (*ims_.begin())->logout(__onlogout);
    }
}

void core::im_container::logout()
{
    assert(!ims_.empty());
    if (ims_.empty())
        return;

    g_core->post_message_to_gui("need_login", 0, nullptr);

    ims_.clear();
}

void core::im_container::on_sign_url(int64_t _seq, coll_helper& _params)
{
    (*ims_.begin())->sign_url(_seq, _params.get_value_as_string("url"));
}

void core::im_container::on_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    core::iarray* prop_array = _params.get_value_as_array("props");
    for (int i = 0; i < prop_array->size(); ++i)
    {
        core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
        auto prop_name = value.get_value_as_string("name");
        auto prop_value = value.get_value_as_string("value");
        props.push_back(std::make_pair(prop_name, prop_value));
    }

    g_core->insert_event((core::stats::stats_event_names)_params.get_value_as_int("event"), props);
}

void core::im_container::on_url_played(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_played(_params.get_value_as_string("url"), _params.get_value_as_bool("played"));
}

void core::im_container::on_speech_to_text(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->speech_to_text(_seq, _params.get_value_as_string("url"), _params.get_value_as_string("locale"));
}

std::shared_ptr<base_im> core::im_container::get_im(coll_helper& _params) const
{
    // temporary, for many im
    return get_im_by_id(0);
}

std::shared_ptr<base_im> core::im_container::get_im_by_id(int32_t _id) const
{
    if (ims_.empty())
    {
        return std::shared_ptr<base_im>();
    }

    return (*ims_.begin());
}

void core::im_container::on_get_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"));
}

void core::im_container::on_add_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_remove_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_set_first_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_first_message(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void core::im_container::on_search(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> searchPatterns;
    core::iarray* searchArray = _params.get_value_as_array("search_patterns");
    for (int i = 0; i < searchArray->size(); ++i)
    {
        core::coll_helper value(searchArray->get_at(i)->get_as_collection(), false);
        searchPatterns.push_back(value.get_value_as_string("pattern"));
    }

    im->search(searchPatterns);
}

void core::im_container::on_set_last_read(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_hide_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_chat(_params.get_value_as_string("contact"));
}

void im_container::on_mute_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mute_chat(_params.get_value_as_string("contact"), _params.get_value_as_bool("mute"));
}

void im_container::on_upload_file_sharing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->upload_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("file"));
}

void im_container::on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->abort_file_sharing_upload(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("process_seq"));
}

void im_container::on_download_file(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing(
        _seq,
        _params.get<std::string>("contact"),
        _params.get<std::string>("url"),
        _params.get<std::string>("download_dir"),
        _params.is_value_exist("filename") ? _params.get<std::string>("filename") : std::string(),
        _params.get<file_sharing_function>("function"));
}

void im_container::on_download_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_preview(
        _seq,
        _params.get<std::string>("uri"),
        _params.get<std::string>("destination", ""),
        _params.get<bool>("sign_url", false));
}

void im_container::on_abort_file_downloading(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->abort_file_sharing_download(
        _seq,
        _params.get_value_as_int64("process_seq"));
}

void im_container::on_get_stickers_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_meta(_seq, _params.get_value_as_string("size"));
}

void im_container::on_get_themes_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_themes_meta(_seq, (ThemesScale)_params.get_value_as_int("themes_scale"));
}

void im_container::on_get_theme(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme(_seq, _params.get_value_as_int("theme_id"));
}

void im_container::on_get_sticker(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_sticker(_seq,
        _params.get<int32_t>("set_id"),
        _params.get<int32_t>("sticker_id"),
        _params.get<core::sticker_size>("size"));
}

void im_container::on_get_chat_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_chat_info(_seq,
        _params.get<std::string>("aimid"),
        _params.get<int32_t>("limit"));
}

void im_container::on_search_contacts(int64_t _seq, coll_helper& _params)
{
    core::search_params filters;
    filters.unserialize(_params);

    auto im = get_im(_params);
    if (!im)
        return;

    im->search_contacts(_seq, filters);
}

void im_container::on_search_contacts2(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    im->search_contacts2(_seq, _params.get<std::string>("keyword"), _params.get<std::string>("phonenumber"), _params.get<std::string>("tag"));
}

void im_container::on_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_profile(_seq, _params.get_value_as_string("aimid"));
}

void im_container::on_hide_dlg_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_dlg_state(_params.get_value_as_string("aimid"));
}

void im_container::on_add_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("group"),
        _params.get_value_as_string("message"));
}

void im_container::on_remove_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_spam_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->spam_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_ignore_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->ignore_contact(
        _seq,
        _params.get_value_as_string("contact"), _params.get_value_as_bool("ignore"));
}

void im_container::on_get_ignore_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ignore_list(_seq);
}

void im_container::on_favorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->favorite(_params.get_value_as_string("contact"));
}

void im_container::on_unfavorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->unfavorite(_params.get_value_as_string("contact"));
}

themes::theme* im_container::get_theme(coll_helper& _params)
{
    auto im = get_im(_params);
    int theme_id = _params.get_value_as_int("id");
    themes::theme *the_theme = im->get_theme_from_cache(theme_id);
    return the_theme;
}

void im_container::on_get_flags(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->load_flags(_seq);
}
