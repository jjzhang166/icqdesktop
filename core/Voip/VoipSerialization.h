#ifndef __VOIP_SERIALIZATION_H__
#define __VOIP_SERIALIZATION_H__

#include "VoipManagerDefines.h"
#include <assert.h>
#include <sstream>
#include "../../corelib/collection_helper.h"

inline void operator>>(const voip2::ButtonType& type, core::coll_helper& coll) {
    switch (type) {
    case voip2::ButtonType_Close: coll.set_value_as_string("button_type", "close"); return;
    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::ButtonTap& button_tap, core::coll_helper& coll) {
    coll.set_value_as_string("account", button_tap.account.c_str());
    coll.set_value_as_string("contact", button_tap.contact.c_str());
    button_tap.type >> coll;
}

inline void operator>>(const std::vector<std::string>& contacts, core::coll_helper& coll) {
    for (unsigned ix = 0; ix < contacts.size(); ix++) {
        const std::string& contact = contacts[ix];
        if (contact.empty()) {
            assert(false);
            continue;
        }

        std::stringstream ss;
        ss << "contact_" << ix;

        coll.set_value_as_string(ss.str().c_str(), contact.c_str());
    }
}

inline void operator>>(const voip2::DeviceType& type, core::coll_helper& coll) {
    const char* name = "device_type";
    switch (type) {
    case voip2::VideoCapturing: coll.set_value_as_string(name, "video_capture"); return;
    case voip2::AudioRecording: coll.set_value_as_string(name, "audio_capture"); return;
    case voip2::AudioPlayback: coll.set_value_as_string(name, "audio_playback"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::LayoutType& type, core::coll_helper& coll) {
    const char* name = "layout_type";
    switch (type) {
    case voip2::LayoutType_One: coll.set_value_as_string(name, "square_with_detach_preview"); return;
    case voip2::LayoutType_Two: coll.set_value_as_string(name, "square_with_attach_preview"); return;
    case voip2::LayoutType_Three: coll.set_value_as_string(name, "primary_with_detach_preview"); return;
    case voip2::LayoutType_Four: coll.set_value_as_string(name, "primary_with_attach_preview"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::MouseTap& type, core::coll_helper& coll) {
    const char* name = "mouse_tap_type";
    switch (type) {
    case voip2::MouseTap_Single: coll.set_value_as_string(name, "single"); return;
    case voip2::MouseTap_Double: coll.set_value_as_string(name, "double"); return;
    case voip2::MouseTap_Long: coll.set_value_as_string(name, "long"); return;
    case voip2::MouseTap_Over: coll.set_value_as_string(name, "over"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::ViewArea& type, core::coll_helper& coll) {
    const char* name = "view_area_type";
    switch (type) {
    case voip2::ViewArea_Primary: coll.set_value_as_string(name, "primary"); return;
    case voip2::ViewArea_Detached: coll.set_value_as_string(name, "detached"); return;
    case voip2::ViewArea_Default: coll.set_value_as_string(name, "default"); return;
    case voip2::ViewArea_Background: coll.set_value_as_string(name, "background"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::DeviceState& state, core::coll_helper& coll) {
    state.type >> coll;
    coll.set_value_as_bool("success", state.success);
    if (!state.uid.empty()) {
        coll.set_value_as_string("uid", state.uid.c_str());
    } else {
        assert(false);
    }
}

inline void operator>>(const intptr_t& ptr, core::coll_helper& coll) {
    coll.set_value_as_int64("pointer", (int64_t)ptr);
}

inline void operator<<(intptr_t& ptr, core::coll_helper& coll) {
    ptr = coll.get_value_as_int64("pointer");
}

inline void operator>>(const voip_manager::DeviceVol& vol, core::coll_helper& coll) {
    vol.type >> coll;
    coll.set_value_as_int("volume_percent", int(vol.volume * 100.0f));
}

inline void operator>>(const voip_manager::DeviceMute& mute, core::coll_helper& coll) {
    mute.type >> coll;
    coll.set_value_as_bool("muted", mute.mute);
}

inline void operator>>(const bool& param, core::coll_helper& coll) {
    coll.set_value_as_bool("param", param);
}

inline void operator>>(const voip_manager::DeviceInterrupt& inter, core::coll_helper& coll) {
    inter.type >> coll;
    coll.set_value_as_bool("interrupt", inter.interrupt);
}

inline void operator>>(const voip_manager::LayoutChanged& type, core::coll_helper& coll) {
    type.layout_type >> coll;
    coll.set_value_as_bool("tray", type.tray);
}

inline void operator>>(const voip_manager::device_description& device, core::coll_helper& coll) {
    coll.set_value_as_string("name", device.name);
    coll.set_value_as_string("uid", device.uid);
    coll.set_value_as_bool("is_active", device.isActive);
    device.type >> coll;
}

inline void operator>>(const std::vector<voip_manager::device_description>& devices, core::coll_helper& coll) {
    coll.set_value_as_int("count", (int)devices.size());

    for (unsigned ix = 0; ix < devices.size(); ix++) {
        const voip_manager::device_description& desc = devices[ix];
        core::coll_helper device_coll(coll->create_collection(), false);
        desc >> device_coll;

        std::stringstream sstream;
        sstream << "device_" << ix;

        coll.set_value_as_collection(sstream.str(), device_coll.get());
    }
}

inline void operator>>(const voip_manager::FrameSize& fs, core::coll_helper& coll) {
    coll.set_value_as_int64("wnd", fs.hwnd);
    coll.set_value_as_double("aspect_ratio", fs.aspect_ratio);
}

inline void operator>>(const voip_manager::MouseTap& tap, core::coll_helper& coll) {
    coll.set_value_as_string("account", tap.account.c_str());
    coll.set_value_as_string("contact", tap.contact.c_str());

    int64_t hwnd = (int64_t)tap.hwnd;
    coll.set_value_as_int64("hwnd", hwnd);

    tap.tap >> coll;
    tap.area >> coll;
}

inline void operator>>(const voip_manager::MissedCall& missed_call, core::coll_helper& coll) {
    coll.set_value_as_string("account", missed_call.account.c_str());
    coll.set_value_as_string("contact", missed_call.contact.c_str());
    coll.set_value_as_int("ts", missed_call.ts);
}

inline void operator>>(const std::string& param, core::coll_helper& coll) {
    coll.set_value_as_string("param", param.c_str());
}

inline void operator>>(const voip_manager::Contact& contact, core::coll_helper& coll) {
    coll.set_value_as_string("account", contact.account.c_str());
    coll.set_value_as_string("contact", contact.contact.c_str());
}

inline void operator<<(voip_manager::Contact& contact, core::coll_helper& coll) {
    contact.account = coll.get_value_as_string("account");
    contact.contact = coll.get_value_as_string("contact");

    assert(!contact.account.empty());
    assert(!contact.contact.empty());
}

inline void operator>>(const voip_manager::ContactEx& contact_ex, core::coll_helper& coll) {
    contact_ex.contact >> coll;
    coll.set_value_as_int("call_count", contact_ex.call_count);
    coll.set_value_as_bool("incoming", contact_ex.incoming);
}

inline void operator<<(voip_manager::ContactEx& contact_ex, core::coll_helper& coll) {
    contact_ex.contact << coll;
    contact_ex.call_count = coll.get_value_as_int("call_count");
    contact_ex.incoming = coll.get_value_as_bool("incoming");
}

inline void operator<<(voip_manager::FrameSize& fs, core::coll_helper& coll) {
    fs.aspect_ratio = coll.get_value_as_double("aspect_ratio");
    fs.hwnd = coll.get_value_as_int64("wnd");
}

inline void operator>>(const std::vector<voip_manager::Contact>& contacts, core::coll_helper& coll) {
    coll.set_value_as_int("count", (int)contacts.size());

    for (unsigned ix = 0; ix < contacts.size(); ix++) {
        const voip_manager::Contact& cont = contacts[ix];

        core::coll_helper contact_coll(coll->create_collection(), false);
        cont >> contact_coll;

        std::stringstream sstream;
        sstream << "contact_" << ix;

        coll.set_value_as_collection(sstream.str(), contact_coll.get());
    }
}

inline void operator<<(std::vector<voip_manager::Contact>& contacts, core::coll_helper& coll) {
    contacts.clear();
    const auto count = coll.get_value_as_int("count");
    if (!count) {
        return;
    }

    contacts.reserve(count);
    for (int ix = 0; ix < count; ix++) {
        std::stringstream ss;
        ss << "contact_" << ix;

        auto contact = coll.get_value_as_collection(ss.str().c_str());
        assert(contact);
        if (!contact) { continue; }

        core::coll_helper contact_helper(contact, false);
        voip_manager::Contact cont_desc;
        cont_desc << contact_helper;

        contacts.push_back(cont_desc);
    }
}

inline void operator>>(const voip_manager::eNotificationTypes& type, core::coll_helper& coll) {
    const char* name = "sig_type";

    using namespace voip_manager;
    switch (type) {
    case kNotificationType_Undefined:   coll.set_value_as_string(name, "undefined");    return;
    case kNotificationType_CallCreated: coll.set_value_as_string(name, "call_created"); return;
    case kNotificationType_CallInvite:  coll.set_value_as_string(name, "call_invite");  return;
    case kNotificationType_CallOutAccepted: coll.set_value_as_string(name, "call_out_accepted"); return;
    case kNotificationType_CallInAccepted: coll.set_value_as_string(name, "call_in_accepted"); return;
    case kNotificationType_CallConnected: coll.set_value_as_string(name, "call_connected"); return;
    case kNotificationType_CallDisconnected: coll.set_value_as_string(name, "call_disconnected"); return;
    case kNotificationType_CallDestroyed: coll.set_value_as_string(name, "call_destroyed"); return;
    case kNotificationType_CallPeerListChanged: coll.set_value_as_string(name, "call_peer_list_changed"); return;

    case kNotificationType_PhoneCallCreated: coll.set_value_as_string(name, "phone_call_created"); return;
    case kNotificationType_PhoneCallAccepted: coll.set_value_as_string(name, "phone_call_accepted"); return;
    case kNotificationType_PhoneCallConnected: coll.set_value_as_string(name, "phone_call_connected"); return;
    case kNotificationType_PhoneCallDisconnected: coll.set_value_as_string(name, "phone_call_disconnected"); return;
    case kNotificationType_PhoneCallDestroyed: coll.set_value_as_string(name, "phone_call_destroyed"); return;

    case kNotificationType_QualityChanged: coll.set_value_as_string(name, "quality_changed"); return;

    case kNotificationType_MediaLocAudioChanged: coll.set_value_as_string(name, "media_loc_a_changed"); return;
    case kNotificationType_MediaLocVideoChanged: coll.set_value_as_string(name, "media_loc_v_changed"); return;
    case kNotificationType_MediaRemVideoChanged: coll.set_value_as_string(name, "media_rem_v_changed"); return;
    case kNotificationType_MediaRemAudioChanged: coll.set_value_as_string(name, "media_rem_a_changed"); return;

    case kNotificationType_DeviceListChanged: coll.set_value_as_string(name, "device_list_changed"); return;
    case kNotificationType_DeviceStarted: coll.set_value_as_string(name, "device_started"); return;
    case kNotificationType_DeviceMuted: coll.set_value_as_string(name, "device_muted"); return;
    case kNotificationType_DeviceVolChanged: coll.set_value_as_string(name, "device_vol_changed"); return;
    case kNotificationType_DeviceInterrupt: coll.set_value_as_string(name, "device_interrupt"); return;

    case kNotificationType_MouseTap: coll.set_value_as_string(name, "mouse_tap"); return;
    case kNotificationType_ButtonTap: coll.set_value_as_string(name, "button_tap"); return;
    case kNotificationType_LayoutChanged: coll.set_value_as_string(name, "layout_changed"); return;

    case kNotificationType_MissedCall: coll.set_value_as_string(name, "missed_call"); return;
    case kNotificationType_ShowVideoWindow: coll.set_value_as_string(name, "video_window_show"); return;
    case kNotificationType_FrameSizeChanged: coll.set_value_as_string(name, "frame_size_changed"); return;
    case kNotificationType_VoipResetComplete: coll.set_value_as_string(name, "voip_reset_complete"); return;
    case kNotificationType_VoipWindowRemoveComplete: coll.set_value_as_string(name, "voip_window_remove_complete"); return;
    case kNotificationType_VoipWindowAddComplete: coll.set_value_as_string(name, "voip_window_add_complete"); return;

    default: assert(false); return;
    }
}

#endif//__VOIP_SERIALIZATION_H__