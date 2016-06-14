#include "stdafx.h"
#include "VoipProxy.h"
#include "../core_dispatcher.h"

#include "../../core/Voip/VoipManagerDefines.h"
#include "../../core/Voip/VoipSerialization.h"
#include "../utils/utils.h"
#include "../utils/gui_coll_helper.h"
#include "../main_window/sounds/SoundsManager.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../gui_settings.h"

#ifdef __APPLE__
    #include "VideoFrameMacos.h"
#endif

#define INCLUDE_USER_NAME         1
#define INCLUDE_USER_NAME_IN_CONF 0
#define INCLUDE_NO_LOCAL_CAMERA   0
#define INCLUDE_OFF_LOCAL_CAMERA  0
#define INCLUDE_REMOTE_CAMERA_OFF 0
#define INCLUDE_AVATAR            1
#define INCLUDE_WATERMARK         1
#define INCLUDE_CAMERA_OFF_STATUS 1

namespace
{
    inline const size_t __hash(const std::wstring& str)
    {
        std::hash<std::wstring> h;
        return h(str);
    }
    inline const size_t __hash(const std::string& str)
    {
        std::hash<std::string> h;
        return h(str);
    }
}

#define STRING_ID(x) __hash((x))

voip_proxy::VoipController::VoipController(Ui::core_dispatcher& dispatcher)
	: _dispatcher(dispatcher)
	, _call_time_elapsed(0)
	, _call_time_timer(this)
	, _haveEstablishedConnection(false)
    , _iTunesWasPaused(false) {
        _cipherState.state= voip_manager::CipherState::kCipherStateUnknown;

		connect(&_call_time_timer, SIGNAL(timeout()), this, SLOT(_updateCallTime()), Qt::QueuedConnection);
		_call_time_timer.setInterval(1000);

        std::vector<unsigned> codePoints;
        codePoints.reserve(32);

        codePoints.push_back(0xF09F8EA9);
        codePoints.push_back(0xF09F8FA0);
        codePoints.push_back(0xF09F92A1);
        codePoints.push_back(0xF09F9AB2);
        codePoints.push_back(0xF09F8C8D);
        codePoints.push_back(0xF09F8D8C);
        codePoints.push_back(0xF09F8D8F);
        codePoints.push_back(0xF09F909F);
        codePoints.push_back(0xF09F90BC);
        codePoints.push_back(0xF09F928E);
        codePoints.push_back(0xF09F98BA);
        codePoints.push_back(0xF09F8CB2);
        codePoints.push_back(0xF09F8CB8);
        codePoints.push_back(0xF09F8D84);
        codePoints.push_back(0xF09F8D90);
        codePoints.push_back(0xF09F8E88);
        codePoints.push_back(0xF09F90B8);
        codePoints.push_back(0xF09F9180);
        codePoints.push_back(0xF09F9184);
        codePoints.push_back(0xF09F9191);
        codePoints.push_back(0xF09F9193);
        codePoints.push_back(0xF09F9280);
        codePoints.push_back(0xF09F9494);
        codePoints.push_back(0xF09F94A5);
        codePoints.push_back(0xF09F9A97);
        codePoints.push_back(0xE2AD90);
        codePoints.push_back(0xE28FB0);
        codePoints.push_back(0xE29ABD);
        codePoints.push_back(0xE29C8F);
        codePoints.push_back(0xE29882);
        codePoints.push_back(0xE29C82);
        codePoints.push_back(0xE29DA4);

        _voipEmojiManager.addMap(64, 64, ":/emoji/secure_emoji_64.png", codePoints, 64);
        _voipEmojiManager.addMap(80, 80, ":/emoji/secure_emoji_80.png", codePoints, 80);
        _voipEmojiManager.addMap(96, 96, ":/emoji/secure_emoji_96.png", codePoints, 96);
        _voipEmojiManager.addMap(128, 128, ":/emoji/secure_emoji_128.png", codePoints, 128);
}

voip_proxy::VoipController::~VoipController() {
	_call_time_timer.stop();
}

void voip_proxy::VoipController::_updateCallTime() {
	_call_time_elapsed += 1;
	emit onVoipCallTimeChanged(_call_time_elapsed, true);
}


void voip_proxy::VoipController::setAvatars(QPixmap& data, int size, const char* contact) {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	_loadUserBitmaps(collection, &data, contact, size);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

std::string voip_proxy::VoipController::formatCallName(const std::vector<std::string>& names, const char* clip) {
    std::string name;
    for (unsigned ix = 0; ix < names.size(); ix++) {
        const std::string& nt = names[ix];
        assert(!nt.empty());
        if (nt.empty()) {
            continue;
        }

        if (name.empty()) {
            // ------------------------------
        } else if (ix == names.size() - 1 && clip) {
            name += " ";
            name += clip;
            name += " ";
        } else {
            name += ", ";
        }
        name += nt;
    }

    assert(!name.empty() || names.empty());
    return name;
}

void voip_proxy::VoipController::setRequestSettings() {
    Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
    collection.set_value_as_string("type", "update");
    _dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_loadUserBitmaps(Ui::gui_coll_helper& collection, QPixmap* avatar, const std::string& contact, int size) {
	assert(avatar);
	if (!avatar) {
		return;
	}

	assert(!contact.empty() && size);
	if (contact.empty() || !size) {
		return;
	}

	collection.set_value_as_string("type", "converted_avatar");
	collection.set_value_as_string("contact", contact);
	collection.set_value_as_int   ("size", size);

	auto __pack_avatar = [&collection] (const QImage& image, const std::string& prefix) {
		const auto data_size = image.byteCount();
		const auto data_ptr  = image.bits();

		assert(data_size && data_ptr);
		if (data_size && data_ptr) {
			core::ifptr<core::istream> avatar_stream(collection->create_stream());
			avatar_stream->write(data_ptr, data_size);
			collection.set_value_as_stream(prefix + "avatar", avatar_stream.get());
			collection.set_value_as_int(prefix + "avatar_height", image.height());
			collection.set_value_as_int(prefix + "avatar_width", image.width());
		}
	};

	const auto raw_avatar    = avatar->toImage();

    if (INCLUDE_AVATAR) {// simple avatar
        const int avatarSize = Utils::scale_bitmap(voip_manager::kAvatarDefaultSize);

		QImage image(QSize(avatarSize, avatarSize), QImage::Format_ARGB32);
		image.fill(Qt::transparent);

        QPainter painter(&image);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::NoBrush);

		QPainterPath path(QPointF(0, 0));
		path.addEllipse(0, 0, avatarSize, avatarSize);
		painter.setClipPath(path);

        painter.drawImage(QRect(0, 0, avatarSize, avatarSize), raw_avatar);
		__pack_avatar(image, "");
	}

    if (INCLUDE_REMOTE_CAMERA_OFF) {// no remote video avatar
        const int avatar_no_remote_video_sz = Utils::scale_bitmap(160);
        const int x_y_offset = (Utils::scale_bitmap(voip_manager::kAvatarDefaultSize) - avatar_no_remote_video_sz) / 2;

		QImage image(Utils::scale_bitmap(QSize(voip_manager::kAvatarDefaultSize, voip_manager::kAvatarDefaultSize)), QImage::Format_ARGB32);
		image.fill(Qt::transparent);

        QPainter painter(&image);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::NoBrush);

		QPainterPath path(QPointF(0, 0));
		path.addEllipse(x_y_offset, x_y_offset, avatar_no_remote_video_sz, avatar_no_remote_video_sz);
		painter.setClipPath(path);

        painter.drawImage(QRect(x_y_offset, x_y_offset, avatar_no_remote_video_sz, avatar_no_remote_video_sz), raw_avatar);
        painter.fillRect(QRect(0, 0, Utils::scale_bitmap(voip_manager::kAvatarDefaultSize), Utils::scale_bitmap(voip_manager::kAvatarDefaultSize)), QColor(0, 0, 0, 150));
        __pack_avatar(image, "rem_camera_offline_");
	}

	{// signs
		QPainter painter;
		painter.save();
        
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

		QString tmp_contact(Logic::GetContactListModel()->getDisplayName(contact.c_str()));
        if (tmp_contact.isEmpty()) {
            tmp_contact = contact.c_str();
        }

        if (INCLUDE_USER_NAME) {// normal
            QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, Utils::scale_bitmap(16));
            font.setStyleStrategy(QFont::PreferAntialias);

            QFontMetrics fm(font);
            const QSize text_sz = fm.size( Qt::TextSingleLine, tmp_contact);
            const int nickW = Utils::scale_bitmap(voip_manager::kNickTextW);
            const int nickH = Utils::scale_bitmap(voip_manager::kNickTextH);

            QPixmap pm(nickW, nickH);
            pm.fill(QColor(0, 0, 0, 0));

            const QPen pen(QColor(255, 255, 255, 230));

			painter.begin(&pm);
			painter.setPen(pen);
			painter.setFont(font);

			if (text_sz.width() > nickW) {
				painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, tmp_contact);
			} else {
				painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignHCenter | Qt::AlignVCenter, tmp_contact);
			}
			painter.end();

			__pack_avatar(pm.toImage(), "sign_normal_");
		}

		if (INCLUDE_USER_NAME_IN_CONF) {// header
			const QSize border = Utils::scale_bitmap(QSize(2, 2));

            QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, Utils::scale_bitmap(12));
			font.setStyleStrategy(QFont::PreferAntialias);
			const QPen pen(QColor(255, 255, 255, 230));

			QSize text_sz;
			{ // measure text size
				QPixmap pm(1, 1);
				painter.begin(&pm);
				painter.setPen(pen);
				painter.setFont(font);

				QFontMetrics fMetrics = painter.fontMetrics();
				text_sz = Utils::scale_bitmap(fMetrics.size( Qt::TextSingleLine, tmp_contact));
				painter.end();
			}

			{ // draw
				QPixmap pm(text_sz.width() + border.width() * 2, text_sz.height() + border.height() * 2);
				pm.fill(QColor(0, 0, 0, 0));

				painter.begin(&pm);
				painter.setPen(pen);
				painter.setFont(font);

				painter.drawText(QRect(border.width(), border.height(), text_sz.width(), text_sz.height()), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, tmp_contact);
				painter.end();

				__pack_avatar(pm.toImage(), "sign_header_");
			}
		}

		painter.restore();
	}

    if (INCLUDE_OFF_LOCAL_CAMERA) {// local camera offline avatar
		const QImage camera_pic(":/resources/voip.local_camera_offline.png");//voip.call_have_problem.png");
		__pack_avatar(camera_pic, "loc_camera_offline_");
	}

    if (INCLUDE_NO_LOCAL_CAMERA) {// local camera disabled avatar
		const QImage camera_pic(":/resources/voip.no_local_camera.png");//voip.call_have_problem.png");
		__pack_avatar(camera_pic, "loc_camera_disabled_");
	}
}

void voip_proxy::VoipController::handlePacket(core::coll_helper& coll_params) {
	const std::string sig_type = coll_params.get_value_as_string("sig_type");
	if (sig_type == "video_window_show") {
		const bool enable = coll_params.get_value_as_bool("param");

		emit onVoipShowVideoWindow(enable);
	} else if (sig_type == "device_vol_changed") {
		int volume_percent = coll_params.get_value_as_int("volume_percent");
		const std::string device_type = coll_params.get_value_as_string("device_type");

        volume_percent = std::min(100, std::max(volume_percent, 0));
		emit onVoipVolumeChanged(device_type, volume_percent);
    } else if (sig_type == "voip_reset_complete") {
        emit onVoipResetComplete();
    } else if (sig_type == "device_muted") {
		const bool muted = coll_params.get_value_as_bool("muted");
		const std::string device_type = coll_params.get_value_as_string("device_type");

		emit onVoipMuteChanged(device_type, muted);
	} else if (sig_type == "call_invite") {
		const std::string account = coll_params.get_value_as_string("account");
		const std::string contact = coll_params.get_value_as_string("contact");

		emit onVoipCallIncoming(account, contact);
	} else if (sig_type == "call_peer_list_changed") {
        std::vector<voip_manager::Contact> contacts;
        contacts << coll_params;

        _activePeerList.swap(contacts);
		emit onVoipCallNameChanged(_activePeerList);
    } else if (sig_type == "voip_window_add_complete") {
	    intptr_t hwnd;
        hwnd << coll_params;

        emit onVoipWindowAddComplete(hwnd);
    } else if (sig_type == "voip_window_remove_complete") {
	    intptr_t hwnd;
        hwnd << coll_params;

        emit onVoipWindowRemoveComplete(hwnd);
	} else if (sig_type == "call_in_accepted") {
		voip_manager::ContactEx contact_ex;
		contact_ex << coll_params;

		emit onVoipCallIncomingAccepted(contact_ex);
	} else if (sig_type == "device_list_changed") {
		const int device_count = coll_params.get_value_as_int("count");
		std::vector<device_desc> devices;
		devices.reserve(device_count);

		if (device_count > 0) {
			for (int ix = 0; ix < device_count; ix++) {
				std::stringstream ss;
				ss << "device_" << ix;

				auto device = coll_params.get_value_as_collection(ss.str().c_str());
				assert(device);
				if (device) {
					core::coll_helper device_helper(device, false);

					device_desc desc;
					desc.name = device_helper.get_value_as_string("name");
					desc.uid  = device_helper.get_value_as_string("uid");
                    desc.isActive = device_helper.get_value_as_bool("is_active");

					const std::string type = device_helper.get_value_as_string("device_type");
					desc.dev_type = type == "audio_playback" ? kvoip_dev_type_audio_playback :
						type == "video_capture"  ? kvoip_dev_type_video_capture  :
						type == "audio_capture"  ? kvoip_dev_type_audio_capture  :
						kvoip_dev_type_undefined;

					assert(desc.dev_type != kvoip_dev_type_undefined);
					if (desc.dev_type != kvoip_dev_type_undefined) {
						devices.push_back(desc);
					}
				}
			}
		}

		assert((device_count > 0 && !devices.empty()) || (!device_count && devices.empty()));
		emit onVoipDeviceListUpdated(devices);
	} else if (sig_type == "media_loc_a_changed") {
		const bool enabled = coll_params.get_value_as_bool("param");

		emit onVoipMediaLocalAudio(enabled);
	} else if (sig_type == "media_loc_v_changed") {
		const bool enabled = coll_params.get_value_as_bool("param");

		emit onVoipMediaLocalVideo(enabled);
	} else if (sig_type == "mouse_tap") {
		const quintptr hwnd = coll_params.get_value_as_int64("hwnd");
		const std::string tap_type(coll_params.get_value_as_string("mouse_tap_type"));

		emit onVoipMouseTapped(hwnd, tap_type);
	} else if (sig_type == "call_created") {
		voip_manager::ContactEx contact_ex;
		contact_ex << coll_params;

		// We use sound_enabled settings for voip ring.
		// Set sound to 1.0.
		if (!_haveEstablishedConnection)
		{
			onStartCall();
		}

		_haveEstablishedConnection = true;

		emit onVoipCallCreated(contact_ex);
        Ui::GetSoundsManager()->callInProgress(true);
	} else if (sig_type == "call_destroyed") {
		voip_manager::ContactEx contact_ex;
		contact_ex << coll_params;

		if (contact_ex.call_count <= 1) {
			_call_time_timer.stop();
			_call_time_elapsed = 0;
			_haveEstablishedConnection = false;
			onEndCall();

			emit onVoipCallTimeChanged(_call_time_elapsed, false);
            Ui::GetSoundsManager()->callInProgress(false);
		}

		emit onVoipCallDestroyed(contact_ex);
	} else if (sig_type == "call_connected") {
		voip_manager::ContactEx contact_ex;
		contact_ex << coll_params;

		// TODO: When we will have video conference, we need to update this code.
		_call_time_elapsed = 0;
		emit onVoipCallTimeChanged(_call_time_elapsed, true);

		if (!_call_time_timer.isActive()) {
			// Disable mute, if it was set for ring.
			setAPlaybackMute(false);
			_call_time_timer.start();
		}

		emit onVoipCallConnected(contact_ex);
	} else if (sig_type == "frame_size_changed") {
	    voip_manager::FrameSize fs;
        fs << coll_params;

        emit onVoipFrameSizeChanged(fs);
	} else if (sig_type == "media_rem_v_changed") {
        const bool enabled = coll_params.get_value_as_bool("param");
        emit onVoipMediaRemoteVideo(enabled);
	} else if (sig_type == "voip_cipher_state_changed") {
	    _cipherState << coll_params;
        emit onVoipUpdateCipherState(_cipherState);
	} else if (sig_type == "call_out_accepted") {
		voip_manager::ContactEx contact_ex;
		contact_ex << coll_params;

		emit onVoipCallOutAccepted(contact_ex);
	}
}

void voip_proxy::VoipController::updateActivePeerList() {
    emit onVoipCallNameChanged(_activePeerList);
}

void voip_proxy::VoipController::setHangup() {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_stop");
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchAPlaybackMute() {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "audio_playback_mute_switch");
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setVolumeAPlayback(int volume) {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_volume_change");
    collection.set_value_as_int("volume", std::min(100, std::max(0, volume)));
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchACaptureMute() {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_media_switch");
	collection.set_value_as_bool("video", false);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchVCaptureMute() {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_media_switch");
	collection.set_value_as_bool("video", true);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setMuteSounds(bool mute) {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_sounds_mute");
	collection.set_value_as_bool("mute", mute);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setActiveDevice(const voip_proxy::device_desc& description) {
    assert(!description.uid.empty());
    if (description.uid.empty()) { return; }

    std::string devType;
    switch (description.dev_type) {
    case kvoip_dev_type_audio_playback: { devType = "audio_playback"; break; }
    case kvoip_dev_type_audio_capture:  { devType = "audio_capture";  break; }
    case kvoip_dev_type_video_capture:  { devType = "video_capture";  break; }
    case kvoip_dev_type_undefined: 
    default:
        assert(!"unexpected device type");
        return;
    };

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", devType);
    collection.set_value_as_string("uid", description.uid);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptA(const char* contact) {
	assert(contact);
	if (!contact) {
		return;
	}

	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_accept");
	collection.set_value_as_string("mode", "audio");
	collection.set_value_as_string("contact", contact);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptV(const char* contact) {
	assert(contact);
	if (!contact) {
		return;
	}

	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_accept");
	collection.set_value_as_string("mode", "video");
	collection.set_value_as_string("contact", contact);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::voipReset() {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_reset");
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setDecline(const char* contact, bool busy) {
	assert(contact);
	if (!contact) {
		return;
	}

	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_decline");
	collection.set_value_as_string("mode", busy ? "busy" : "not_busy");
	collection.set_value_as_string("contact", contact);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setStartA(const char* contact, bool attach) {
	assert(contact);
	if (!contact) {
		return;
	}

	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_start");
	collection.set_value_as_qstring("contact", contact);
	collection.set_value_as_string("call_type", "audio");
	collection.set_value_as_string("mode", attach ? "attach" : "not attach");

	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setStartV(const char* contact, bool attach) {
	assert(contact);
	if (!contact) {
		return;
	}

	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_call_start");
	collection.set_value_as_qstring("contact", contact);
	collection.set_value_as_string("call_type", "video");
	collection.set_value_as_string("mode", attach ? "attach" : "not attach");

	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowOffsets(quintptr hwnd, int l, int t, int r, int b) {
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "voip_set_window_offsets");
	collection.set_value_as_int64("handle", hwnd);
	collection.set_value_as_int("left", l);
	collection.set_value_as_int("top", t);
	collection.set_value_as_int("right", r);
	collection.set_value_as_int("bottom", b);
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowAdd(quintptr hwnd, bool primary_wnd, bool system_wnd, int panel_height) {
	if (hwnd) {
        Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
        {// window
			collection.set_value_as_string("type", "voip_add_window");
            collection.set_value_as_double("scale", Utils::get_scale_coefficient());
			collection.set_value_as_bool("mode", primary_wnd);
			collection.set_value_as_bool("system_mode", system_wnd);
			collection.set_value_as_int64("handle", hwnd);
		}

        if (INCLUDE_CAMERA_OFF_STATUS && primary_wnd) {
            QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 14);
            font.setStyleStrategy(QFont::PreferAntialias);

            const QString text = QT_TRANSLATE_NOOP("voip_pages", "turned off the camera");

            QFontMetrics fm(font);
            const QSize text_sz = fm.size( Qt::TextSingleLine, text);

            const int nickW = Utils::scale_bitmap(text_sz.width());
            const int nickH = Utils::scale_bitmap(text_sz.height());

            assert(nickW && nickH);
            if (nickW && nickH) {
                QPainter painter;
                painter.save();
    
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::TextAntialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                QPixmap pm(nickW, nickH);
                pm.fill(QColor(0, 0, 0, 0));

                const QPen pen(QColor(0x7e, 0x7e, 0x7e, 0xff));

                painter.begin(&pm);
                painter.setPen(pen);
                painter.setFont(font);

                if (text_sz.width() > nickW) {
                    painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, text);
                } else {
                    painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignHCenter | Qt::AlignVCenter, text);
                }
                painter.end();

                QImage image = pm.toImage();

                auto data_size = image.byteCount();
                auto data_ptr  = image.bits();
                auto sz = image.size();

                if (data_size && data_ptr) {
                    core::ifptr<core::istream> stream(collection->create_stream());
                    stream->write(data_ptr, data_size);

                    collection.set_value_as_stream ("camera_status", stream.get());
                    collection.set_value_as_int    ("camera_status_height", sz.height());
                    collection.set_value_as_int    ("camera_status_width", sz.width());
                }

                painter.restore();
            }
        }

        if (INCLUDE_WATERMARK && primary_wnd) {
            std::stringstream resourceStr;
            resourceStr << ":/resources/video_panel/icq_logo_watermark_" << Utils::scale_value(100) << ".png";
			QImage watermark(resourceStr.str().c_str());

			auto data_size = watermark.byteCount();
			auto data_ptr  = watermark.bits();
			auto sz        = watermark.size();

			assert(data_size && data_ptr);
			if (data_size && data_ptr) {
				core::ifptr<core::istream> stream(collection->create_stream());
				stream->write(data_ptr, data_size);

				collection.set_value_as_stream ("watermark", stream.get());
				collection.set_value_as_int    ("watermark_height", sz.height());
				collection.set_value_as_int    ("watermark_width", sz.width());
			}
        }
        _dispatcher.post_message_to_core("voip_call", collection.get());

		if (!system_wnd) {
			setWindowOffsets(hwnd, Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(panel_height));
		}
	}
}

void voip_proxy::VoipController::setWindowRemove(quintptr hwnd) {
	if (hwnd) {
		Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
		collection.set_value_as_string("type", "voip_remove_window");
		collection.set_value_as_int64("handle", hwnd);
		_dispatcher.post_message_to_core("voip_call", collection.get());
	}
}

void voip_proxy::VoipController::getSecureCode(voip_manager::CipherState& state) const
{
    state.state      = _cipherState.state;
    state.secureCode = _cipherState.secureCode;
}

void voip_proxy::VoipController::setAPlaybackMute(bool mute)
{
	Ui::gui_coll_helper collection(_dispatcher.create_collection(), true);
	collection.set_value_as_string("type", "audio_playback_mute");
	collection.set_value_as_string("mute", mute ? "on" : "off");
	_dispatcher.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::loadPlaybackVolumeFromSettings()
{
	bool sound_enabled = Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true);
	setAPlaybackMute(!sound_enabled);
}

void voip_proxy::VoipController::_checkIgnoreContact(QString contact)
{
	std::string contactName = contact.toStdString();

	if (std::find_if(_activePeerList.begin(), _activePeerList.end(), 
					 [&contactName](const voip_manager::Contact& contact){ return contact.contact == contactName;}) != _activePeerList.end())
	{
		setHangup();
	}	
}

void voip_proxy::VoipController::onStartCall()
{
	setVolumeAPlayback(100);
	loadPlaybackVolumeFromSettings();
#ifdef __APPLE__
        // Pause iTunes if we have call.
        _iTunesWasPaused = _iTunesWasPaused || platform_macos::pauseiTunes();
#endif

	connect(Logic::GetContactListModel(), SIGNAL(ignoreContact(QString)), this, SLOT(_checkIgnoreContact(QString)));
}

void voip_proxy::VoipController::onEndCall()
{
#ifdef __APPLE__
	// Resume iTunes if we paused it.
	if (_iTunesWasPaused)
	{
		platform_macos::playiTunes();
		_iTunesWasPaused = false;
	}
#endif

	disconnect(Logic::GetContactListModel(), SIGNAL(ignoreContact(QString)), this, SLOT(_checkIgnoreContact(QString)));
}



voip_proxy::VoipEmojiManager::VoipEmojiManager()
: activeMapId_(0)
{

}

voip_proxy::VoipEmojiManager::~VoipEmojiManager()
{

}

bool voip_proxy::VoipEmojiManager::addMap(const unsigned sw, const unsigned sh, const std::string& path, const std::vector<unsigned>& codePoints, const size_t size)
{
    assert(!codePoints.empty());

    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    const size_t id = STRING_ID(path);
    bool existsSameSize = false;
    bool exactlySame    = false;

    if (it != codeMaps_.end()) {
        existsSameSize = (it->codePointSize == size);
        exactlySame    = existsSameSize && (it->id == id);
    }

    if (exactlySame) {
        return true;
    }

    if (existsSameSize) {
        codeMaps_.erase(it);
    }

    CodeMap codeMap;
    codeMap.id            = id;
    codeMap.path          = path;
    codeMap.codePointSize = size;
    codeMap.sw            = sw;
    codeMap.sh            = sh;
    codeMap.codePoints    = codePoints;

    codeMaps_.push_back(codeMap);
    std::sort(codeMaps_.begin(), codeMaps_.end(), [] (const CodeMap& l, const CodeMap& r)
    {
       return l.codePointSize < r.codePointSize; 
    });
    return true;
}

bool voip_proxy::VoipEmojiManager::getEmoji(const unsigned codePoint, const size_t size, QImage& image)
{
    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    if (it == codeMaps_.end()) {
        if (codeMaps_.empty()) {
            return false;
        }
        it = codeMaps_.end();
    }

    const CodeMap& codeMap = *it;
    if (codeMap.id != activeMapId_) { // reload code map
        activeMap_.load(codeMap.path.c_str());
        activeMapId_ = codeMap.id;
    }

    const size_t mapW = activeMap_.width();
    const size_t mapH = activeMap_.height();
    assert(mapW != 0 && mapH != 0);

    if (mapW == 0 || mapH == 0) {
        return false;
    }

    const size_t rows = mapH / codeMap.sh;
    const size_t cols = mapW / codeMap.sw;

    for (size_t ix = 0; ix < codeMap.codePoints.size(); ++ix) {
        const unsigned code = codeMap.codePoints[ix];
        if (code == codePoint) {
            const size_t targetRow = ix / cols;
            const size_t targetCol = ix - (targetRow * cols);

            if (targetRow >= rows) {
                return false;
            }

            const QRect r(targetCol * codeMap.sw, targetRow * codeMap.sh, codeMap.sw, codeMap.sh);
            image = activeMap_.copy(r);
            return true;
        }
    }

    assert(false);
    return false;
}
