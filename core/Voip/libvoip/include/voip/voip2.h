#ifndef __VOIP2_H__
#define __VOIP2_H__

#include "voip_types.h"

namespace voip2 {

using namespace voip;

#define PREVIEW_RENDER_NAME     "@preview"
enum LayoutType {       // -= remote =-         -= camera =-
    LayoutType_One,     // equally spaced       detached
    LayoutType_Two,     // equally spaced       as remote
    LayoutType_Three,   // primary+tray         detached
    LayoutType_Four     // primary+tray         as remote
};
enum PreviewMode {
    PreviewMode_Detached,
    PreviewMode_AsRemote,
};
enum MouseTap {
    MouseTap_Single,
    MouseTap_Double,
    MouseTap_Long,
    MouseTap_Over, // reported once per second
};

enum ViewArea {
    ViewArea_Primary,
    ViewArea_Detached,
    ViewArea_Default,
    ViewArea_Background,
};

enum AvatarType {
    AvatarType_UserMain,
    AvatarType_UserNoVideo,
    AvatarType_UserText,
    AvatarType_Header,
    AvatarType_Camera,
    AvatarType_CameraCrossed,
    AvatarType_Background,
    AvatarType_Foreground,
};
enum DeviceType {
    AudioRecording = 0,
    AudioPlayback,
    VideoCapturing,
};

enum Position {
    Position_Left    = 0x01,
    Position_HCenter = 0x02,
    Position_Right   = 0x04,

    Position_Top     = 0x08,
    Position_VCenter = 0x10,
    Position_Bottom  = 0x20
};

enum SoundEvent {
    SoundEvent_OutgoingStarted,
    SoundEvent_WaitingForAccept,            // loop
    SoundEvent_WaitingForAccept_Confirmed,  // loop (we have ringing received)
    SoundEvent_IncomingInvite,              // loop
    SoundEvent_Connected,
    SoundEvent_Connecting,                  // loop
    SoundEvent_Reconnecting,                // loop
    SoundEvent_Hold,                        // loop
    SoundEvent_HangupLocal,
    SoundEvent_HangupRemote,
    SoundEvent_HangupRemoteBusy,
    SoundEvent_HangupHandledByAnotherInstance, // silent
    SoundEvent_HangupByError,

    SoundEvent_Max
};

enum VoipIncomingMsg {
    WIM_Incoming_fetch_url           = 0,
    WIM_Incoming_allocated,

    MRIM_Incoming_SessionAllocated   = 10,
    MRIM_Incoming_UdpMedia,
    MRIM_Incoming_UdpMediaAck,

    OSCAR_Incoming_Allocated         = 20,
    OSCAR_Incoming_WebRtc
};

enum VoipOutgoingMsg {
    WIM_Outgoing_allocate            = 0,
    WIM_Outgoing_invite,
    WIM_Outgoing_accept,
    WIM_Outgoing_decline,
    WIM_Outgoing_json,
    WIM_Outgoing_keepalive,

    MRIM_Outgoing_SessionAllocate    = 10,
    MRIM_Outgoing_UdpMedia,
    MRIM_Outgoing_UdpMediaAck,

    OSCAR_Outgoing_SessionAllocate   = 20,
    OSCAR_Outgoing_SessionAllocatePstn,
    OSCAR_Outgoing_WebRtc
};
/*
    | Event | Code   |
    +-------+--------+  <- DTMF codes
    | 0--9  | 0--9   |
    | *     | 10     |
    | #     | 11     |
    | A--D  | 12--15 |
*/
#define ANIMATION_CURVE_SAMPLERATE_HZ 50
enum { kMaxAnimationCurveLen = 500 };
enum AnimationTypes {
    kAnimationType_Connecting = 0,
    kAnimationType_Reconnecting,
    //----------------------------
    kAnimationType_Total
};

struct AnimationContext {
    unsigned animationPeriodMs; // can be less then animation duration

    unsigned width;
    unsigned height;

    unsigned frameWidth;
    unsigned frameHeight;

    int xOffset; // from viewport center
    int yOffset; // from viewport center

    unsigned animation_curve_len;
    unsigned color_bgra_animation_curve[kMaxAnimationCurveLen];// sampled at 50Hz (20ms step), 5 sec max
    float geometry_animation_curve[kMaxAnimationCurveLen];// sampled at 50Hz (20ms step), 5 sec max
};

//#define WindowSettingsThemesCount 2
typedef enum {
    WindowTheme_One = 0,
    WindowTheme_Two = 1,
    WindowTheme_Total = 2
} WindowThemeType;

struct WindowSettings {

    WindowThemeType theme;
    
    struct {
        // picture size
        unsigned width;
        unsigned height;
        // text size
        unsigned textWidth;
        unsigned textHeight;

        // Avatar position adjustment:
        // for (Position_HCenter | Position_VCenter)
        // signed (up/down) offset in percents (-100..100) of the (HEIGHT/2) of the draw rect
        signed offsetVertical;

        // Avatar offset form the viewport frame
        unsigned offsetTop;
        unsigned offsetBottom;
        unsigned offsetLeft;
        unsigned offsetRight;

        ChannelStatusContext status;
        // Avatar color rings animations
        AnimationContext animation[kAnimationType_Total];

        hbmp_t   logoImage;
        unsigned logoPosition;// combination of values from Position enum, if (hwnd == NULL) sets position for all windows
        int      logoOffsetL;
        int      logoOffsetT;
        int      logoOffsetR;
        int      logoOffsetB;
    } avatarMain[WindowTheme_Total];

    // Preview:
    bool previewSelfieMode;         // on Calling or Invite mode show preview fullscreen
    bool previewIsButton;           // false - show actual camera state, true - operate as a player button
    bool previewDisable;            // disable preview for this window
    bool previewSolo;               // show only preview in this window

    unsigned previewMaxArea;        // maximum area = (width * heigth) for preview
    unsigned previewMinArea;        // minimum area

    unsigned      previewBorderWidth;        // width in pixels
    unsigned char previewBorderColorBGRA[4]; // IOS: Quartz supports premultiplied alpha only for images


    bool disable_mouse_events_handler;

    unsigned      highlight_border_pix;
    unsigned char highlight_color_bgra[4];
    unsigned char normal_color_bgra[4];


    // Conferencing settings
    // Avatar secondary peer names text size (conference)
    unsigned channelTextDisplayW;
    unsigned channelTextDisplayH;
    // Avatar main peer status text size
    unsigned channelStatusDisplayW; // if 0 then appropriate size will be extracted
    unsigned channelStatusDisplayH; // from ChannelStatusContext

    unsigned tray_height_pix;
    unsigned lentaBetweenChannelOffset;
    unsigned blocksBetweenChannelOffset;
    bool     forcePrimaryVideoCropIfConference;
    bool     alignPrimaryVideoTopIfConference;
    float    desired_aspect_ration_in_videoconf;
    
    ButtonContext        buttonContext;

    unsigned headerOffset;
    unsigned header_height_pix;
    unsigned gap_width_pix;

    // Avatar bounce animation
    unsigned  animation_curve_len;   // in samples
    float     animation_curve[kMaxAnimationCurveLen];     // sampled at 50Hz (20ms step), 5 sec max

    // Glow animation
    unsigned      statusGlowRadius;           // 0 - disable glow effect, otherwise set radius
    double        glowAttenuation;
    unsigned char connectedGlowColor[4];
    unsigned char disconnectedGlowColor[4];

    //
    hbmp_t oldverTextLarge;     // joint peer
    hbmp_t oldverTextLarge2;    // direct
    hbmp_t oldverTextSmall;
    unsigned char oldverBackround_bgra[4];   
    unsigned oldverBackroundHeightPer; // 0 - 100 %
    unsigned oldverTextLargeDelayMs; // if 0 - TextLarge will be shown on hover only
};
/*
    Integration design:

    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Application <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<        >>>>>>>>>>> Voip <<<<<<<<<<<   

     ------------------------------------                        --------------------------------------                   ----------------
    |                                    |                      |                                      |                 |                |
    |      -= MainWnd =- -- MAIN_THREAD ------ user action ---->| -= Controller =-  *--- MAIN_THREAD ---- user action -->|   -= Voip =-   |
    |                                    |                      |                                      |                 |                |
    |                                    |<--- read info -------|       implements VoipObserver  <------- signaling -----|                |
    |                                    |                      |                       |              |                 |                |
    |   implements ControllerObserver <------- state changed -----*----- MAIN_THREAD ---*              |                 |                |
    |                                    |                      | |                                    |                 |                |
     ------------------------------------                       | |                                    |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | *--|         -= CallMgr =-         | |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |            \/     /\               |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | *--|     -= ContactListMgr =-      | |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |            \/     /\               |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |  |     -= IMConnectionMgr =-     | |                 |                |
                                                                | *--|   implements VoipConnection <----- signaling -----|                |
                                                                |     -------------------------------  |                  ---------------
                                                                 --------------------------------------
    Outgoing call:

                ButtonPressed          ---------------------------------------------------------------------------------> CallStart(user_id)
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_OUTGOING_STARTED
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_OUTGOING_ACCEPTED_VIDEO
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_CONNECTED
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                ...
                ...

                ButtonPressed          ---------------------------------------------------------------------------------> CallDecline(user_id)
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_CLOSED_BY_LOCAL_HANGUP
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---

*/

class VoipConnection {
public:
    // msg_idx == 0 means this message is not required to store for resending
    virtual void SendVoipMsg   (const char* from, VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx = 0) = 0;
};

enum SessionEvent { // alive session events
    SE_OPEN_FIRST = 0,
    SE_OUTGOING_STARTED_AUDIO = SE_OPEN_FIRST,  // Feedback on local CallStart(): there is no session with given user_id and we're going to create one
    SE_OUTGOING_STARTED_VIDEO,                  //      [ audio/video flag duplicates local camera state off/on ]
    SE_INCOMING_INVITE_AUDIO,                   // Invite from remote peer. Application may ask for attached list of conference participants using ... .
    SE_INCOMING_INVITE_VIDEO,
    SE_OUTGOING_ACCEPTED_AUDIO,                 // Remote peer has accepted our invite
    SE_OUTGOING_ACCEPTED_VIDEO,
    SE_INCOMING_ACCEPTED_AUDIO,                 // Feedback on local CallAccept()
    SE_INCOMING_ACCEPTED_VIDEO,                 //      [ audio/video flag duplicates local camera state off/on ]
    SE_JOINED_AUDIO,                            // Notification about verified peer joined existing conference
    SE_JOINED_VIDEO,
    SE_OPEN_LAST = SE_JOINED_VIDEO,

    SE_CONNECTION_FIRST = 20,
    SE_DISCONNECTED = SE_CONNECTION_FIRST,
    SE_CONNECTED,
    SE_CONNECTED_EXT_AUDIO_NONE,    // Detailed connection info, application must not rely on those states to manage call flow
    SE_CONNECTED_EXT_AUDIO_UDP,
    SE_CONNECTED_EXT_AUDIO_TCP,
    SE_CONNECTED_EXT_AUDIO_RELAY,
    SE_CONNECTED_EXT_VIDEO_NONE,
    SE_CONNECTED_EXT_VIDEO_UDP,
    SE_CONNECTED_EXT_VIDEO_TCP,
    SE_CONNECTED_EXT_VIDEO_RELAY,
    SE_CONNECTION_LAST = SE_CONNECTED_EXT_VIDEO_RELAY,

    SE_REMOTE_FIRST = 40,
    SE_REMOTE_MIC_ON = SE_REMOTE_FIRST,
    SE_REMOTE_MIC_OFF,
    SE_REMOTE_CAM_ON,
    SE_REMOTE_CAM_OFF,
    SE_QUALITY_BAD,
    SE_QUALITY_AUDIO_OK,
    SE_QUALITY_AUDIO_VIDEO_OK,
    SE_INCOMING_CONF_PEERS_UPDATED,             // new conference participant for existing invite, application must ask voip for details and update invite dialog
    SE_NO_CONF_SUPPORTED,                       // remote peer does not support conference mode 
    SE_OUTGOING_VIDEO_DISABLED_LOW_BANDWIDTH,
    SE_REMOTE_LAST = SE_OUTGOING_VIDEO_DISABLED_LOW_BANDWIDTH,

    // closed session events:   (Note: if you change this part (e.g. by adding new events), don't forget to update 
    //                           stat mapping in call_stat/call_record.cc:SessionEvent2TerminateReason() )
    SE_CLOSED_BY_REMOTE_DECLINE = 128,
    SE_CLOSED_BY_REMOTE_HANDLED_BY_ANOTHER_INSTANCE,
    SE_CLOSED_BY_REMOTE_BUSY,
    SE_CLOSED_BY_REMOTE_ERROR,
    SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_REMOTE,
    SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_LOCAL,
    SE_CLOSED_BY_TIMEOUT_INACTIVE,      // this peer invited by conference participant and we get no additional info on timeout exhausted
    SE_CLOSED_BY_TIMEOUT_CONNECT_INIT,  // connection has not been started
    SE_CLOSED_BY_TIMEOUT_CONNECTION,
    SE_CLOSED_BY_TIMEOUT_RECONNECT,
    SE_CLOSED_BY_ERROR_CREATE,
    SE_CLOSED_BY_ERROR_START,
    SE_CLOSED_BY_ERROR_INTERNAL,
    SE_CLOSED_BY_LOCAL_BUSY,
    SE_CLOSED_BY_LOCAL_HANGUP
};

struct CallInfo {
    enum { kMaxUsers = 16 };

    bool     local_audio_enabled;
    bool     local_video_enabled;
    unsigned numUsers;

    struct UserInfo {
        enum { kMaxUserIdSize = 256 };

        char user_id[kMaxUserIdSize];    

        ConnectionState connstate_audio;
        ConnectionState connstate_video;  

        bool remote_audio_enabled;
        bool remote_video_enabled;

    } users[kMaxUsers];
};

enum DeviceStatus {
    DeviceStatus_Started,
    DeviceStatus_Resumed,
    DeviceStatus_Paused,
    DeviceStatus_Stopped,
    DeviceStatus_Stopped_ByVoip,
    DeviceStatus_Stopped_StartFail,
};

class VoipObserver {
public:
    virtual void DeviceListChanged          (DeviceType deviceType) = 0;
    virtual void DeviceStatusChanged        (DeviceType deviceType, const char *uid, DeviceStatus deviceStatus) = 0;

    virtual void AudioDeviceVolumeChanged   (DeviceType deviceType, float volume) = 0;
    virtual void AudioDeviceMuteChanged     (DeviceType deviceType, bool mute) = 0;
    virtual void AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) = 0;

    virtual void RenderMouseTap     (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) = 0;
    virtual void ButtonPressed      (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) = 0;
    virtual void MissedCall         (const char* account_uid, const char* user_id, unsigned timestamp) = 0;
    virtual void SessionEvent       (const char* account_uid, const char* user_id, voip2::SessionEvent sessionEvent) = 0;
    virtual void LayoutTypeChanged  (voip::hwnd_t hwnd, voip2::LayoutType layoutType) = 0;
    virtual void FrameSizeChanged   (voip::hwnd_t hwnd, float aspectRatio) = 0;
    virtual void VideoStreamChanged (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) = 0;

    virtual void InterruptByGsmCall(bool gsmCallStarted) = 0;
};

enum {
    MAX_DEVICE_NAME_LEN = 512,
    MAX_DEVICE_GUID_LEN = 512
};

struct CreatePrms; // hidden from application

struct VOIP_EXPORT ConferenceParticipants {
    ConferenceParticipants();
    ~ConferenceParticipants();
    const char *user_uids; // separated with '\0', ends with '\0\0'
};

class VOIP_EXPORT Voip2 { // singletone
public:
    static Voip2* CreateVoip2(
        VoipConnection& voipConnection, 
        VoipObserver& voipObserver, 
        const char* client_name_and_version,        // Short client identification string, e.g. "ICQ 1.2.1234", "MyPhone 1.2.111", etc.
        const char* app_data_folder_utf8,           // Path to folder where call_stat will store backup file
        const char* stat_servers_override = NULL,   // Format: "host1:port1;host2:port2;host3" (80 assumed for host3)
        bool useOscarAsIcqProtocol = false,
        voip::SystemObjects *platformSystemObject = NULL, 
        const char *paramDB_json = NULL, 
        CreatePrms* createPrms = NULL               // internal use only
    );
    static void  DestroyVoip2(Voip2*);

    virtual bool Init(CreatePrms* createPrms = NULL) = 0;
    virtual void EnableMsgQueue() = 0;
    virtual void StartSignaling() = 0;

    virtual void EnableRtpDump(bool enable) = 0;

    virtual unsigned GetDevicesNumber(DeviceType deviceType) = 0;
    virtual bool     GetDevice       (DeviceType deviceType, unsigned index, char deviceName[MAX_DEVICE_NAME_LEN], char deviceUid[MAX_DEVICE_GUID_LEN]) = 0;
    virtual void     SetDevice       (DeviceType deviceType, const char *deviceUid) = 0;
    virtual void     SetDeviceMute   (DeviceType deviceType, bool mute) = 0;
    virtual bool     GetDeviceMute   (DeviceType deviceType) = 0;
    virtual void     SetDeviceVolume (DeviceType deviceType, float volume) = 0;
    virtual float    GetDeviceVolume (DeviceType deviceType) = 0;

    virtual void SetProxyPrms        (voip::ProxyType proxyType, const char *serverUrl, const char *userName, const char *password) = 0;
    virtual void SetSound            (SoundEvent soundEvent, const void* data, unsigned size, const unsigned* vibroPatternMs = NULL, unsigned vibroPatternLen = 0, unsigned samplingRateHz = 0) = 0;
    virtual void SetSound            (SoundEvent soundEvent, const char* fileName, const unsigned* vibroPatternMs = NULL, unsigned vibroPatternLen = 0) = 0;
    virtual void MuteIncomingSoundNotifications   (const char* user_id, bool mute) = 0;
    virtual void MuteAllIncomingSoundNotifications(bool mute) = 0;

    virtual void SetLoudspeakerMode  (bool enable)  = 0;

    virtual void ReadVoipMsg (const char *account_uid, VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len, const char *phonenum = NULL) = 0;
    virtual void ReadVoipPush(const char *account_uid, const char *data, unsigned len, const char *phonenum = NULL) = 0;
    virtual void ReadVoipAck (const char *account_uid, unsigned msg_idx, bool success) = 0;

    virtual void EnableOutgoingAudio(bool enable) = 0;
    virtual void EnableOutgoingVideo(bool enable) = 0;

    virtual void CallStart  (const char* account_uid, const char* user_id) = 0;
    virtual void CallAccept (const char* account_uid, const char* user_id) = 0;
    virtual void CallDecline(const char* user_id, bool busy = false) = 0;
    virtual void CallStop   () = 0; // Does not affect incoming calls

    virtual void ShowIncomingConferenceParticipants(const char* user_id, ConferenceParticipants& conferenceParticipants) = 0;
    virtual void SendAndPlayOobDTMF(const char* user_id, int code, int play_ms=200, int play_Db=10) = 0;

    // Layout controls
    virtual void WindowAdd                  (hwnd_t wnd, const WindowSettings& windowSettings) = 0;
    virtual void WindowRemove               (hwnd_t wnd) = 0;
    virtual void WindowSetBackground        (hwnd_t wnd, hbmp_t hbmp) = 0;
    virtual void WindowSetAvatar            (const char* user_id, hbmp_t hbmp,  AvatarType avatarType, WindowThemeType theme = WindowTheme_One,
                                                int radiusPix = 0, // negative value set radius to min(width/2, height/2)
                                                unsigned smoothingBorderPx = 0, unsigned attenuation_from_0_to_100 = 0) = 0;
    virtual void WindowChangeOrientation    (OrientationMode mode) = 0;
    virtual void WindowSetLayoutType        (hwnd_t wnd, LayoutType layoutType) = 0;
    virtual void WindowSwitchAspectMode     (hwnd_t wnd, const char* user_id) = 0;
    virtual void WindowSetPrimary           (hwnd_t wnd, const char* user_id) = 0;
    virtual void WindowSetControlsStatus    (hwnd_t wnd, bool visible, unsigned off_left, unsigned off_top, unsigned off_right, unsigned off_bottom, unsigned period_ms, bool enableOverlap) = 0;
    virtual void WindowAddButton            (ButtonType type, ButtonPosition position) = 0;
    virtual void WindowSetAvatarPosition    (hwnd_t wnd, unsigned position) = 0; // position - combination of values from Position enum, if (hwnd == NULL) sets position for all windows
    virtual void WindowSetTheme             (hwnd_t wnd, voip2::WindowThemeType theme) = 0;

#if (__PLATFORM_WINPHONE || WINDOWS_PHONE) && defined(__cplusplus_winrt)
    virtual void GetVoipInfo(voip2::CallInfo& callInfo) = 0;
#endif

    virtual void UserRateLastCall(const char* account_uid, const char* user_id, int score) = 0; // use after call complete (SE_CLOSED_BY_* received)
    static void DisableStatistics();// internal use only
};
}

#if (__PLATFORM_WINPHONE || WINDOWS_PHONE) && defined(__cplusplus_winrt)
namespace webrtc {
    class MouseEventHandler;
}

namespace VoipSvc
{
    class IMouseEventsSource
    {
    public:
        // invokes to store sink in UI every time mouse and windows event listener created.
        virtual void OnMouseEventsSinkCreated(voip::hwnd_t wnd, webrtc::MouseEventHandler& mouseEventHandler) = 0;

        // invokes to reset sink in UI.
        virtual void OnMouseEventsSinkDestroyed(voip::hwnd_t wnd, webrtc::MouseEventHandler& mouseEventHandler) = 0;
    };
}

#endif

#endif
