#include "stdafx.h"

#include "../utils.h"

#include "../Voip/VoipManager.h"

#include "../tools/md5.h"
#include "../tools/system.h"

#include "../archive/contact_archive.h"
#include "im_login.h"

// stickers
#include "../stickers/stickers.h"

#include "../themes/themes.h"

#include "base_im.h"
#include "../async_task.h"

using namespace core;

namespace fs = boost::filesystem;

namespace {
	void __on_voip_user_bitmap(voip_manager::VoipManager& vm, const std::string& contact, voip2::AvatarType type, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
#ifndef STRIP_VOIP
		voip_manager::UserBitmap bmp;
		bmp.bitmap.data = (void*)data;
		bmp.bitmap.size = size;
		bmp.bitmap.w    = w;
		bmp.bitmap.h    = h;
		bmp.contact     = contact;
		bmp.type        = type;

		vm.get_window_manager()->window_set_bitmap(bmp);
#endif
	}
}

base_im::base_im(const im_login_id& _login, voip_manager::VoipManager& _voip_manager)
    :   voip_manager_(_voip_manager),
        id_(_login.get_id())
{
}


base_im::~base_im() {
}

void base_im::set_id(int32_t _id)
{
    id_ = _id;
}

int32_t base_im::get_id() const
{
    return id_;
}

std::shared_ptr<stickers::face> base_im::get_stickers()
{
    if (!stickers_)
        stickers_.reset(new stickers::face(get_stickers_path()));

    return stickers_;
}

std::shared_ptr<themes::face> base_im::get_themes()
{
    if (!themes_)
        themes_.reset(new themes::face(get_themes_path()));
    
    return themes_;
}

void base_im::connect()
{

}

std::wstring core::base_im::get_contactlist_file_name()
{
	return (get_im_data_path() + L"/" + L"contacts" + L"/" + L"cache.cl");
}

std::wstring core::base_im::get_my_info_file_name()
{
	return (get_im_data_path() + L"/" + L"info" + L"/" + L"cache");
}

std::wstring base_im::get_active_dilaogs_file_name()
{
	return (get_im_data_path() + L"/" + L"dialogs" + L"/" + archive::version_db_filename(L"cache"));
}

std::wstring base_im::get_favorites_file_name()
{
    return (get_im_data_path() + L"/" + L"favorites" + L"/" + archive::version_db_filename(L"cache"));
}

std::wstring core::base_im::get_im_data_path()
{
	return (utils::get_product_data_path() + L"/" + get_im_path());
}

std::wstring base_im::get_file_name_by_url(const std::string& _url)
{
	return (get_im_data_path() + L"/" + L"content.cache" + L"/" + core::tools::from_utf8(core::tools::md5(_url.c_str(), (int32_t)_url.length())));
}

std::wstring base_im::get_stickers_path()
{
	return (get_im_data_path() + L"/" + L"stickers");
}

std::wstring base_im::get_themes_path()
{
    return (utils::get_product_data_path() + L"/" + L"themes");
}

std::wstring base_im::get_im_downloads_path(const std::string &alt)
{
#ifdef __linux__
    typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter_t;
#else
    typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter_t;
#endif

    auto user_downloads_path = core::tools::system::get_user_downloads_dir();

    converter_t converter;
    const auto walt = converter.from_bytes(alt.c_str());
    if (!walt.empty())
    {
        user_downloads_path = walt;
    }

	if (!core::tools::system::is_dir_writable(user_downloads_path))
	{
        const auto default_path = core::tools::system::get_user_downloads_dir();
		user_downloads_path = default_path;
	}

    //return user_downloads_path;

    if (platform::is_windows())
    {
	    return (user_downloads_path + L"/ICQ");
    }

	std::wstring str = converter.from_bytes("/ICQ");
	return (user_downloads_path + str);
}

// voip
void core::base_im::on_voip_call_set_proxy(const voip_manager::VoipProxySettings& proxySettings) {
#ifndef STRIP_VOIP
	auto call_manager = voip_manager_.get_call_manager();
	assert(!!call_manager);

	if (!!call_manager) {
        call_manager->call_set_proxy(proxySettings);
	}
#endif
}

void core::base_im::on_voip_call_start(std::string contact, bool video, bool attach) {
#ifndef STRIP_VOIP
	assert(!contact.empty());
	if (contact.empty()) {
		return;
	}

	auto call_manager = voip_manager_.get_call_manager();
	assert(!!call_manager);

	auto account = _get_protocol_uid();
	assert(!account.empty());

	if (!!call_manager && !account.empty() && !contact.empty()) {
		call_manager->call_create(voip_manager::Contact(account, contact), video, attach);
	}
#endif
}

void core::base_im::on_voip_call_request_calls() {
#ifndef STRIP_VOIP
	voip_manager_.get_call_manager()->call_request_calls();
#endif
}

void core::base_im::on_voip_window_set_offsets(void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) {
#ifndef STRIP_VOIP
	voip_manager_.get_window_manager()->window_set_offsets(hwnd, l, t, r, b);
#endif
}

bool core::base_im::on_voip_avatar_actual_for_voip(const std::string& contact, unsigned avatar_size) {
#ifndef STRIP_VOIP
	auto account = _get_protocol_uid();
	assert(!account.empty());
	return
		!account.empty() &&
		!contact.empty() &&
		voip_manager_.get_call_manager()->call_have_call(voip_manager::Contact(account, contact));
#endif
}

void core::base_im::on_voip_user_update_avatar_no_video(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
	__on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserNoVideo, data, size, h, w);
}

void core::base_im::on_voip_user_update_avatar_camera_off(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
	__on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_Camera, data, size, h, w);
}

void core::base_im::on_voip_user_update_avatar_no_camera(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
	__on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_CameraCrossed, data, size, h, w);
}

void core::base_im::on_voip_user_update_avatar_text(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
	__on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserText, data, size, h, w);
}

void core::base_im::on_voip_user_update_avatar_text_header(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w) {
	__on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_Header, data, size, h, w);
}

void core::base_im::on_voip_user_update_avatar(const std::string& contact, const unsigned char* data, unsigned size, unsigned avatar_h, unsigned avatar_w) {
	return __on_voip_user_bitmap(voip_manager_, contact, voip2::AvatarType_UserMain, data, size, avatar_h, avatar_w);
}

void core::base_im::on_voip_call_end(std::string contact, bool busy) {
#ifndef STRIP_VOIP
	auto account = _get_protocol_uid();
	assert(!account.empty());
	if (!account.empty() && !contact.empty()) {
		voip_manager_.get_call_manager()->call_decline(voip_manager::Contact(account, contact), false);
	}
#endif
}

void core::base_im::on_voip_device_changed(const std::string& dev_type, const std::string& uid) {
#ifndef STRIP_VOIP
	if ("audio_playback" == dev_type) {
		voip_manager_.get_device_manager()->set_device(voip2::AudioPlayback, uid);
	} else if ("audio_capture" == dev_type) {
		voip_manager_.get_device_manager()->set_device(voip2::AudioRecording, uid);
	} else if ("video_capture" == dev_type) {
		voip_manager_.get_device_manager()->set_device(voip2::VideoCapturing, uid);
	} else {
		assert(false);
	}
#endif
}

void core::base_im::on_voip_window_update_background(void* hwnd, const unsigned char* data, unsigned size, unsigned w, unsigned h) {
#ifndef STRIP_VOIP
	voip_manager::WindowBitmap bmp;
	bmp.hwnd = hwnd;
	bmp.bitmap.data = (void*)data;
	bmp.bitmap.size = size;
	bmp.bitmap.w = w;
	bmp.bitmap.h = h;

	voip_manager_.get_window_manager()->window_set_bitmap(bmp);
#endif
}

void core::base_im::on_voip_call_accept(std::string contact, bool video) {
#ifndef STRIP_VOIP
	auto account = _get_protocol_uid();
	assert(!account.empty());

	if (!account.empty() && !contact.empty()) {
		voip_manager_.get_call_manager()->call_accept(voip_manager::Contact(account, contact), video);
	}
#endif
}

void core::base_im::on_voip_add_window(voip_manager::WindowParams& windowParams) {
#ifndef STRIP_VOIP
	voip_manager_.get_window_manager()->window_add(windowParams);
#endif
}

void core::base_im::on_voip_remove_window(void* hwnd) {
#ifndef STRIP_VOIP
	voip_manager_.get_window_manager()->window_remove(hwnd);
#endif
}

void core::base_im::on_voip_call_stop() {
#ifndef STRIP_VOIP
	voip_manager_.get_call_manager()->call_stop();
#endif
}

void core::base_im::on_voip_switch_media(bool video) {
#ifndef STRIP_VOIP
	if (video) {
		const bool enabled = voip_manager_.get_media_manager()->local_video_enabled();
		voip_manager_.get_media_manager()->media_video_en(!enabled);
	} else {
		const bool enabled = voip_manager_.get_media_manager()->local_audio_enabled();
		voip_manager_.get_media_manager()->media_audio_en(!enabled);
	}
#endif
}

void core::base_im::on_voip_mute_incoming_call_sounds(bool mute) {
#ifdef _WIN32
    voip_manager_.get_call_manager()->mute_incoming_call_sounds(mute);
#endif
}

void core::base_im::on_voip_volume_change(int vol) {
#ifndef STRIP_VOIP
	const float vol_fp = std::max(std::min(vol, 100), 0) / 100.0f;
	voip_manager_.get_device_manager()->set_device_volume(voip2::AudioPlayback, vol_fp);
#endif
}

void core::base_im::on_voip_mute_switch() {
#ifndef STRIP_VOIP
	const bool mute = voip_manager_.get_device_manager()->get_device_mute(voip2::AudioPlayback);
	voip_manager_.get_device_manager()->set_device_mute(voip2::AudioPlayback, !mute);
#endif
}

void core::base_im::on_voip_reset() {
#ifndef STRIP_VOIP
    voip_manager_.get_voip_manager()->reset();
#endif
}

void core::base_im::on_voip_proto_ack(const voip_manager::VoipProtoMsg& msg, bool success) {
#ifndef STRIP_VOIP
	auto account = _get_protocol_uid();
	assert(!account.empty());

	if (!account.empty()) {
		voip_manager_.get_connection_manager()->ProcessVoipAck(account.c_str(), msg, success);
	}
#endif
}

void core::base_im::on_voip_proto_msg(bool allocate, const char* data, unsigned len, std::shared_ptr<auto_callback> _on_complete) {
#ifndef STRIP_VOIP
	auto account = _get_protocol_uid();
	assert(!account.empty());
	if (!account.empty()) {
		const auto msg_type = allocate ? voip2::WIM_Incoming_allocated : voip2::WIM_Incoming_fetch_url;
		voip_manager_.get_connection_manager()->ProcessVoipMsg(account.c_str(), msg_type, data, len);
	}
    _on_complete->callback(0);
#endif
}

void core::base_im::on_voip_update() {
#ifndef STRIP_VOIP
	voip_manager_.get_device_manager()->update();
#endif
}
