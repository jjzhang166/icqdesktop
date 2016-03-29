#include "stdafx.h"

#include "VoipManager.h"
#include "VoipSerialization.h"
#include "../connections/base_im.h"
#include "../core.h"

#include "call_busy.h"
#include "call_connected.h"
#include "call_dial.h"
#include "call_end.h"
#include "call_hold.h"
#include "call_incoming.h"
#include "call_start.h"

#include "../utils.h"
#include "../../common.shared/version_info.h"
#include <mutex>
#include "VoipProtocol.h"
#include <time.h>
#include "../async_task.h"

#ifdef _WIN32
    #include <windows.h>
    #include <assert.h>
    #include <hash_map>
#endif

#if __APPLE__
    #include <CoreFoundation/CoreFoundation.h>
    #include <ImageIO/ImageIO.h>
#endif

#if defined(_DEBUG) || defined(DEBUG)
    #ifdef _WIN32
        #define VOIP_ASSERT(exp) \
            if (!(exp)) { \
                if (::IsDebuggerPresent()) { \
                    __debugbreak(); \
                } else { \
                    const_cast<VoipLogger*>(&_voipLogger)->logOut(__FILE__, __LINE__, __FUNCSIG__, ""); \
                    const_cast<VoipLogger*>(&_voipLogger)->logFlush(); \
                } \
            }
    #elif __APPLE__
        #define VOIP_ASSERT(exp) \
            if (!(exp)) { \
                const_cast<VoipLogger*>(&_voipLogger)->logOut(__FILE__, __LINE__, __PRETTY_FUNCTION__, ""); \
                const_cast<VoipLogger*>(&_voipLogger)->logFlush(); \
            }
    #else
        #define VOIP_ASSERT(exp) (void)0
    #endif
#else//DEBUG
    #define VOIP_ASSERT(exp) (void)0
#endif//DEBUG

#define VOIP_ASSERT_RETURN(exp) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        return; \
    }

#define VOIP_ASSERT_RETURN_VAL(exp,val) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        return (val); \
    }

#define VOIP_ASSERT_ACTION(exp,action) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        action; \
    }

#define VOIP_ASSERT_BREAK(exp) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        break; \
    }

namespace voip_manager {
    typedef size_t CallKey;
}

namespace {
    std::string from_unicode_to_utf8(const std::wstring& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        return cv.to_bytes(str);
    }

    std::wstring from_utf8_to_unicode(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        return cv.from_bytes(str.c_str());
    }

    inline double calcDecipateWave(double t, double w, double A, double b) {
        return sin(w * t) * A * exp(-1 * b * t);
    };

    void getPriWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

        ws.avatarMain[voip2::WindowTheme_One].height = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].width  = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].textHeight = voip_manager::kNickTextH;
        ws.avatarMain[voip2::WindowTheme_One].textWidth  = voip_manager::kNickTextW;

        ws.previewIsButton = true;
        ws.lentaBetweenChannelOffset = 20 * scale;
        ws.blocksBetweenChannelOffset = 60 * scale;

        ws.animation_curve_len = 0;
        ws.previewDisable      = false;
        ws.previewSelfieMode   = true;
        ws.avatarMain[voip2::WindowTheme_One].offsetLeft = ws.avatarMain[voip2::WindowTheme_One].offsetRight  = 12 * scale;
        ws.avatarMain[voip2::WindowTheme_One].offsetTop  = ws.avatarMain[voip2::WindowTheme_One].offsetBottom = 12 * scale;

        ws.tray_height_pix    = 200 * scale;//m_fullscreenPanel.GetHeight() + int(BORD_CONTROLS - BORD_UPPER);
        ws.previewBorderWidth = 2 * scale;

        // gray
        ws.previewBorderColorBGRA[0] = 0;
        ws.previewBorderColorBGRA[1] = 0;
        ws.previewBorderColorBGRA[2] = 0;
        ws.previewBorderColorBGRA[3] = 100;

        ws.highlight_border_pix = 0;
        // yellow
        ws.highlight_color_bgra[0] = 225;
        ws.highlight_color_bgra[1] = 158;
        ws.highlight_color_bgra[2] = 47;
        ws.highlight_color_bgra[3] = 100;

        ws.normal_color_bgra[0] = 0;
        ws.normal_color_bgra[1] = 0;
        ws.normal_color_bgra[2] = 0;
        ws.normal_color_bgra[3] = 120;

        ws.header_height_pix = 45 * scale;
        ws.gap_width_pix = 30 * scale;

        ws.desired_aspect_ration_in_videoconf = 4.f/3;

        ws.oldverBackroundHeightPer = 50 * scale;
        ws.oldverBackround_bgra[0] = 0;
        ws.oldverBackround_bgra[1] = 0;
        ws.oldverBackround_bgra[2] = 0;
        ws.oldverBackround_bgra[3] = 120;
    }

    void getSecWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

        ws.avatarMain[voip2::WindowTheme_One].height = voip_manager::kDetachedWndAvatarSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].width  = voip_manager::kDetachedWndAvatarSize * scale;
        ws.previewIsButton  = false;
        ws.animation_curve_len = 0;
        ws.previewDisable     = true;
        ws.disable_mouse_events_handler = true;

        ws.desired_aspect_ration_in_videoconf = 4.f/3;
    }

    void getSysWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

        ws.disable_mouse_events_handler = true;
        ws.previewSolo = true;
        ws.desired_aspect_ration_in_videoconf = 4.f/3;
    }
}

namespace {
    template<typename __Param>
    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype, const __Param& vtype) {
        core::coll_helper coll(dispatcher.create_collection(), true); 
        ntype >> coll; 
        vtype >> coll;
        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }
    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype) {
        core::coll_helper coll(dispatcher.create_collection(), true); 
        ntype >> coll;

        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }
}

#define SIGNAL_NOTIFICATOION(type,key) { \
    if (key) { \
        __signal_to_ui(_dispatcher, type, *key); \
    } else {\
        __signal_to_ui(_dispatcher, type); \
    } \
}

#define PSTN_POSTFIX "@pstn"
#define LOCAL_MAX_PATH 300

#define DB_LEFT    0x01
#define DB_CENTER  0x02
#define DB_RIGHT   0x04
#define DB_TOP     0x08
#define DB_VCENTER 0x10
#define DB_BOTTOM  0x20

namespace {
    struct recti {
        int left;
        int top;
        int right;
        int bottom;
    };

    void* bitmapHandleFromRGBData(unsigned width, unsigned height, unsigned bps, const void *data, bool invert = false) {
#ifdef _WIN32
        BITMAPINFO bmi              = {0};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biBitCount    = (WORD)bps; // RGB32, RGB24, 16-RGB555
        bmi.bmiHeader.biWidth       = width;
        bmi.bmiHeader.biHeight      = (invert ? -1 : 1)*height;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC hdc = ::GetDC(NULL);
        char *bits;
        HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
        if(data) {
            unsigned bytesPerRow = (bps>>3)*width;
            unsigned off = (0-bytesPerRow)&0x3;
            if(off == 0) {
                memcpy(bits, data, bytesPerRow*height);
            } else {
                const char* src = (const char*)data;
                for(unsigned i = 0; i < height; i++) {
                    memcpy(bits, src, bytesPerRow);
                    src += bytesPerRow;
                    bits += bytesPerRow + off;
                }
            }
        }
        ReleaseDC(NULL, hdc);
        return (void*)hBitmap;
#else
        CFDataRef imageData     = CFDataCreate(NULL, (const uint8_t*)data, width * height * bps / 8);
        CGImageSourceRef source = CGImageSourceCreateWithData(imageData, NULL);
        CGImageRef cgImage      = CGImageSourceCreateImageAtIndex(source, 0, NULL);
        
        CFRelease(source);
        CFRelease(imageData);
        
        return cgImage;
#endif//WIN32
    }

    void bitmapReleaseHandle(void* hbmp) {
#ifdef _WIN32
        DeleteObject((HBITMAP)hbmp);
#else
        CFRelease(hbmp);
#endif//WIN32
    }
}


namespace {
    const std::string protoNameICQ   = "ICQ";
    const std::string protoNameAgent = "Agent";
    const std::string protoNamePstn  = "pstn";

    inline voip_manager::CallKey getHash(const std::string& account_id, const std::string& user_id) {
        if (account_id.empty()) { return 0; }
        if (user_id.empty()) { return 0; }

        std::string name;
        name.reserve(account_id.size() + sizeof(char) + user_id.size());

        name += account_id;
        name += '#';
        name += user_id;

        assert(!name.empty());
        if (name.empty()) { return 0; }

        std::hash<std::string> __hash;
        return __hash(name);
    }

    const std::string getProtoName(const std::string& account_uid) {
        if (account_uid.empty()) { return ""; }

        if(account_uid.find('@') == std::string::npos || account_uid.find("@uin.icq") != std::string::npos) {
            return protoNameICQ;
        } else if(account_uid.find(PSTN_POSTFIX) != std::string::npos) {
            return protoNamePstn;
        } else {
            return protoNameAgent;
        }
    }

    std::string normalizeUid(const std::string& uid) {
        std::string _uid = uid;
        auto position = _uid.find("@uin.icq");
        if (std::string::npos == position) {
            position = _uid.find(PSTN_POSTFIX);
        }
        if (std::string::npos != position) {
            _uid = _uid.substr(0, position);
        }
        return _uid;
    }

    std::wstring getFilePath() {
#ifdef _WIN32
        wchar_t path[MAX_PATH * 2] = { 0 };
        const auto chars_write = ::GetModuleFileNameW(NULL, path, sizeof(path) / sizeof(path[0]) - 1);
        path[chars_write] = L'\0';

        std::wstring path_str = path;
        const int slash_pos = path_str.rfind(L'\\');
        if (slash_pos >= 0) {
            path_str = path_str.substr(0, slash_pos) + L"\\";
            return path_str;
        }
        return L"";
#else
        char dir[LOCAL_MAX_PATH];
        if (!getcwd(dir, LOCAL_MAX_PATH)) {
            return L"";
        }
        
        std::wstring path = from_utf8_to_unicode(dir);
        if (path.empty()) {
            return L"";
        }
        
        if (path.at(path.size() - 1) != L'/') {
            path += L"/";
        }

        return path;
#endif//WIN32
    }

    std::string getPhoneNumberFromUid(const std::string& uid) {
        const auto pos = uid.find(PSTN_POSTFIX);
        if (std::string::npos != pos) {
            return uid.substr(0, pos);
        }
        
        return "";
    }

    struct uncopyable {
    protected:
        uncopyable() { }
    private:
        uncopyable(const uncopyable&);
        uncopyable& operator=(const uncopyable&);
    };

#ifdef _WIN32
    enum { kMaxPath = MAX_PATH + 1 };
#else 
    enum { kMaxPath = 300 };
#endif
}

namespace voip_manager {

    class VoipLogger : private uncopyable {
        std::ofstream _logFile;
        std::string   _fileName;

    public:
        VoipLogger();
        ~VoipLogger();

        bool logOpen (const std::string& filename);
        void logOut  (const char* file, int line, const char* func, const char* msg);
        void logFlush();
    };

    VoipLogger::VoipLogger() {
    }

    VoipLogger::~VoipLogger() {
        if (_logFile.is_open()) {
            _logFile.flush();
            _logFile.close();
        }
    }

    bool VoipLogger::logOpen(const std::string& filename) {
        if (_logFile.is_open()) {
            _logFile.flush();
            _logFile.close();
        }
        _fileName = filename;
        return true;
    }

    void VoipLogger::logOut(const char* file, int line, const char* func, const char* msg) {
        if (!_logFile.is_open()) {
            if (!_fileName.empty()) {
                _logFile.open(_fileName, std::ios_base::out | std::ios_base::app);
                if (!_logFile.is_open()) {
                    return;
                }
            } else {
                return;
            }   
        }

        time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        _logFile << std::put_time(std::localtime(&now), "%Y-%m-%d %X");;
        _logFile << " <" << file << ", " << line << ">";
        _logFile << " " << func;

#ifdef _WIN32
         _logFile << " [thread:" << ::GetCurrentThreadId() << "] " << msg << std::endl;
#else
         _logFile << " [unknown thread] " << msg << std::endl;
#endif
    }

    void VoipLogger::logFlush() {
        if (_logFile.is_open()) {
            _logFile.flush();
        }
    }


    class VoipManagerImpl 
        : private uncopyable
        , public ICallManager
        , public IDeviceManager
        , public IWindowManager
        , public IVoipManager
        , public IMediaManager
        , public voip2::VoipObserver
        , public voip2::VoipConnection
        , public IConnectionManager {

        enum eCallState {
            kCallState_Unknown = 0,
            kCallState_Created,

            kCallState_Initiated,
            kCallState_Accepted,

            kCallState_Connected,
            kCallState_Disconnected,
        };

        struct VoipSettings {
            bool enableRtpDump;
            bool enableVoipLogs;
            VoipSettings() : enableRtpDump(false), enableVoipLogs(false) { }
        };

        struct CallDesc {
            CallKey call_key;

            std::string account_id;
            std::string user_id;

            bool       remote_cam_en;
            bool       remote_mic_en;

            eCallState call_state;
            bool       outgoing;
            unsigned   started; // GetTickCount()
            unsigned   quality; // 0 - 100% 

            voip2::SessionEvent close_reason;
        };
        typedef std::shared_ptr<CallDesc> CallDescPtr;
        typedef std::vector<CallDescPtr> CallDescArray;

        struct VoipDesc {
            bool  local_cam_en;
            bool  local_aud_en;
            bool  mute_en;
            bool  incomingSoundsMuted;
            float volume;

            std::string aPlaybackDevice;
            std::string aCaptureDevice;
            std::string vCaptureDevice;

            std::string aPlaybackDefDevice;
            std::string aCaptureDefDevice;
            std::string vCaptureDefDevice;

            VoipDesc() : local_aud_en(true), local_cam_en(true), mute_en(false), volume(0), incomingSoundsMuted(false)
            { }
        };

        struct WindowDesc {
            void* handle;
            voip2::LayoutType layout_type;
            float aspectRatio;
            float scaleCoeffitient;
        };

        struct RequestInfo {
            unsigned messageId;
            std::string requestId;
            std::chrono::milliseconds ts;
        };

        std::recursive_mutex    _windowsMx;
        std::vector<WindowDesc> _windows;

        std::recursive_mutex _callsMx;
        CallDescArray        _calls;

        bool                           sendViaIm_;
        std::function<void()>          callback_;
        std::shared_ptr<VoipProtocol> _voipProtocol;

        std::recursive_mutex _voipDescMx;
        VoipDesc   _voip_desc;
        bool _defaultDevicesStored;

        std::map<unsigned, RequestInfo> _requestIds;
        std::shared_ptr<voip2::Voip2>  _engine;
        VoipLogger _voipLogger;

        core::core_dispatcher& _dispatcher;
        bool _voipDestroyed;
    private:
        std::shared_ptr<voip2::Voip2> _get_engine(bool skipCreation = false);

        bool _init_signaling(std::shared_ptr<voip2::Voip2> engine);
        bool _init_media    (std::shared_ptr<voip2::Voip2> engine);
        bool _init_sounds_sc(std::shared_ptr<voip2::Voip2> engine);
        void _storeDefaultDevices(std::shared_ptr<voip2::Voip2> engine);

        void _update_layout();
        void _update_video_window_state();
        void _set_layout   (void* handle, const voip2::LayoutType& lt);

        bool _call_create (const std::string& account_id, const std::string& user_id, CallDescPtr& desc);
        bool _call_destroy(const CallKey& key);
        bool _call_exists (const CallKey& key);
        bool _call_get    (const CallKey& key, CallDescPtr& desc);
        bool _call_have_outgoing_connection();

        inline bool _getVoipSettingsPath(std::wstring& path) const;
        inline bool _getVoipLogPath(std::wstring& path) const;
        inline const VoipSettings _getVoipSettings() const;
        inline voip2::ProxyType _toVoipProxyType(const VoipProxySettings::eProxyType& type);

        __inline CallDescPtr _get_phone_call ();
        __inline bool        _is_phone_call  (const std::string& uid) const;
        __inline bool        _have_phone_call();

        __inline WindowDesc* _find_window(void* handle);

        void _load_avatars       (const std::string& account_uid, const std::string& user_uid, bool request_if_not_exists = true);
        void _update_peer_list   (const std::string& account_uid, const std::string& user_uid);
        void _update_device_list (voip2::DeviceType dev_type);
        void _notify_remote_video_state_changed();

        void _protocolSendAlloc  (const char* data, unsigned size); //executes in VOIP SIGNALING THREAD context
        void _protocolSendMessage(const VoipProtoMsg& data); //executes in VOIP SIGNALING THREAD context

        void _window_set_bg(void* hwnd, void* hbmp);
        void _window_set_av(const std::string& contact, voip2::AvatarType type, void* hbmp);
    public:
        //=========================== ICallManager API ===========================
        void call_set_proxy               (const VoipProxySettings& proxySettings) override;
        void call_create                  (const Contact& contact, bool video, bool add) override;
        void call_stop                    () override;
        void call_accept                  (const Contact& contact, bool video) override;
        void call_decline                 (const Contact& contact, bool busy) override;
        unsigned call_get_count           () override;
        bool call_have_established_connection () override;
        bool call_have_call               (const Contact& contact) override;
        void call_request_calls           () override;
        void call_stop_smart              (const std::function<void()>&) override;

        bool phone_call_start             (const Contact& contact) override;
        bool phone_call_stop              () override;
        bool phone_call_send_dtmf         (int num) override;

        void mute_incoming_call_sounds    (bool mute) override;

        //=========================== IWindowManager API ===========================
        void window_add           (voip_manager::WindowParams& windowParams) override;
        void window_remove        (void* hwnd) override;
        void window_set_bitmap    (const WindowBitmap& bmp) override;
        void window_set_bitmap    (const UserBitmap& bmp) override;
        void window_switch_layout (void* hwnd) override;
        void window_switch_aspect (const std::string& contact, void* hwnd) override;
        void window_set_offsets   (void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) override;
        void window_add_button    (voip2::ButtonType type, voip2::ButtonPosition position) override;

        //=========================== IMediaManager API ===========================
        void media_video_en       (bool enable) override;
        void media_audio_en       (bool enable) override;
        bool local_video_enabled  () override;
        bool local_audio_enabled  () override;
        bool remote_video_enabled () override;
        bool remote_video_enabled (const std::string& account, const std::string& contact) override;
        bool remote_audio_enabled () override;
        bool remote_audio_enabled (const std::string& account, const std::string& contact) override;

        //=========================== IVoipManager API ===========================
        void reset() override;

        //=========================== VoipObserver ===========================
        void DeviceListChanged        (voip2::DeviceType deviceType) override;
        void DeviceStatusChanged      (voip2::DeviceType deviceType, const char *uid, voip2::DeviceStatus deviceStatus) override;
        void AudioDeviceVolumeChanged (voip2::DeviceType deviceType, float volume) override;
        void AudioDeviceMuteChanged   (voip2::DeviceType deviceType, bool mute) override;
        void AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) override;
        //void AudioDeviceInterrupt     (voip2::DeviceType deviceType, bool is_in_interrupt) override;
        void RenderMouseTap           (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) override;
        void ButtonPressed            (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) override;
        void LayoutTypeChanged        (voip::hwnd_t hwnd, voip2::LayoutType layoutType) override;
        void MissedCall               (const char* account_uid, const char* user_id, unsigned timestamp) override;
        void SessionEvent             (const char* account_uid, const char* user_id, voip2::SessionEvent sessionEvent) override;
        void FrameSizeChanged         (voip::hwnd_t hwnd, float aspectRatio) override;
        void InterruptByGsmCall       (bool gsmCallStarted) override;
        void VideoStreamChanged       (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) override;

        //=========================== VoipConnection ===========================
        void SendVoipMsg(const char* from, voip2::VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx) override;

        //=========================== IConnectionManager API ===========================
        void ProcessVoipMsg(const std::string& account_uid, voip2::VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len) override;
        void ProcessVoipAck(const std::string& account_uid, const voip_manager::VoipProtoMsg& msg, bool success) override;

        //=========================== IDeviceManager API ===========================
        unsigned get_devices_number(voip2::DeviceType device_type) override;
        bool     get_device_list   (voip2::DeviceType device_type, std::vector<device_description>& dev_list) override;
        bool     get_device        (voip2::DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) override;
        void     set_device        (voip2::DeviceType device_type, const std::string& device_guid) override;
        void     set_device_mute   (voip2::DeviceType deviceType, bool mute) override;
        bool     get_device_mute   (voip2::DeviceType deviceType) override;
        void     set_device_volume (voip2::DeviceType deviceType, float volume) override;
        float    get_device_volume (voip2::DeviceType deviceType) override;
        void     update            () override;

    public:
        VoipManagerImpl (core::core_dispatcher& dispatcher);
        ~VoipManagerImpl();
    };

    inline bool _initFirstDevice(std::shared_ptr<voip2::Voip2> engine, voip2::DeviceType type, std::string& uid) {
        if (!engine) { return false; }
        if (engine->GetDevicesNumber(type) == 0) { return true; }

        unsigned defaultDeviceIx = 0;
        if (voip2::AudioRecording == type || voip2::AudioPlayback == type) {
            defaultDeviceIx = (unsigned)-1;
        }

        char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
        char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
        if (!!engine->GetDevice(type, defaultDeviceIx, name, guid)) {
            engine->SetDevice(type, guid);
            uid = guid;
        }

        return true;
    }

    VoipManagerImpl::VoipManagerImpl(core::core_dispatcher& dispatcher) 
        : _engine(NULL)
        , _voipDestroyed(false)
        , sendViaIm_(false)
        , _defaultDevicesStored(false)
        , _dispatcher(dispatcher) {

        srand(time(NULL));

        const VoipSettings voipSettings = _getVoipSettings();
        if (voipSettings.enableVoipLogs) {
            std::wstring filePath;
            _getVoipLogPath(filePath);
            assert(!filePath.empty());

            if (!filePath.empty()) {
                const bool openResult = _voipLogger.logOpen(from_unicode_to_utf8(filePath));
                assert(openResult);
            }
        }
    }

    VoipManagerImpl::~VoipManagerImpl() {
        _engine.reset();

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT(_calls.empty());
    }

    bool VoipManagerImpl::_init_signaling(std::shared_ptr<voip2::Voip2> engine) {
        VOIP_ASSERT_RETURN_VAL(engine, false);
        engine->StartSignaling();

        return true;
    }

    bool VoipManagerImpl::_init_media(std::shared_ptr<voip2::Voip2> engine) {
        using namespace voip2;
        VOIP_ASSERT_RETURN_VAL(engine, false);

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        if (!_voip_desc.aPlaybackDevice.empty()) {
            engine->SetDevice(AudioPlayback, _voip_desc.aPlaybackDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, AudioPlayback, _voip_desc.aPlaybackDevice),  false);
        }

        if (!_voip_desc.aCaptureDevice.empty()) {
            engine->SetDevice(AudioRecording, _voip_desc.aCaptureDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, AudioRecording, _voip_desc.aCaptureDevice), false);
        }

        if (!_voip_desc.vCaptureDevice.empty()) {
            engine->SetDevice(VideoCapturing, _voip_desc.vCaptureDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, VideoCapturing, _voip_desc.vCaptureDevice), false);
        }

        return true;
    }

    void VoipManagerImpl::_set_layout(void* handle, const voip2::LayoutType& lt) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(engine);

        engine->WindowSetLayoutType(handle, lt);
    }

    void VoipManagerImpl::_update_peer_list(const std::string& account_uid, const std::string& user_uid) {
        VOIP_ASSERT_RETURN(!account_uid.empty());
        VOIP_ASSERT_RETURN(!user_uid.empty());

        using namespace voip2;
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        ConferenceParticipants cp;
        engine->ShowIncomingConferenceParticipants(user_uid.c_str(), cp);

        std::vector<Contact> contacts;
        contacts.reserve(3);
        contacts.push_back(Contact(normalizeUid(account_uid), normalizeUid(user_uid)));

        if (cp.user_uids) {
            int str_offset = 0;
            while (true) {
                std::string uid = &cp.user_uids[str_offset];
                const unsigned name_len = (unsigned)uid.size();
                if (!name_len) { break; }

                uid = normalizeUid(uid);
                VOIP_ASSERT_BREAK(!uid.empty());

                contacts.push_back(Contact(normalizeUid(account_uid), normalizeUid(uid)));
                str_offset += name_len + 1;
            }
        }
        VOIP_ASSERT_RETURN(!contacts.empty());
        SIGNAL_NOTIFICATOION(kNotificationType_CallPeerListChanged, &contacts);
    }

    bool VoipManagerImpl::_getVoipSettingsPath(std::wstring& path) const {
        path.clear();
        path.reserve(kMaxPath);

        path += core::utils::get_product_data_path();
        VOIP_ASSERT_RETURN_VAL(!path.empty(), false);

        path += L"\\settings\\voip_config.txt";
        return true;
    }

    bool VoipManagerImpl::_getVoipLogPath(std::wstring& path) const {
        path.clear();
        path.reserve(kMaxPath);

        path += core::utils::get_product_data_path();
        VOIP_ASSERT_RETURN_VAL(!path.empty(), false);

        path += L"\\voip_log.txt";
        return true;
    }

    const VoipManagerImpl::VoipSettings VoipManagerImpl::_getVoipSettings() const {
        VoipSettings settings;
        
        std::wstring filePath;
        _getVoipSettingsPath(filePath);
        VOIP_ASSERT_RETURN_VAL(!filePath.empty(), settings);

        std::ifstream fileStream;
        fileStream.open(from_unicode_to_utf8(filePath));
        if (!fileStream.is_open()) { return settings; }

        auto __removeSpaces = [] (std::string& str) {
            str.erase (std::remove(str.begin(), str.end(), ' '), str.end());
        };

        while (fileStream.good()) {
            std::string line;
            std::getline(fileStream, line);

            const std::string::size_type position = line.find('=');
            VOIP_ASSERT_ACTION(position != std::string::npos, continue);

            std::string param = line.substr(0, position);
            VOIP_ASSERT_ACTION(!param.empty(), continue);

            std::string value = line.substr(position + 1);
            VOIP_ASSERT_ACTION(!value.empty(), continue);

            __removeSpaces(param);
            __removeSpaces(value);
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (std::string::npos != param.find("enablertpdump")) {
                settings.enableRtpDump = value[0] != '0';
            } else if (std::string::npos != param.find("enablevoiplogs")) {
                settings.enableVoipLogs = value[0] != '0';
            }
        }

        return settings;
    }

    void VoipManagerImpl::_update_layout() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);

        int counter = 0;
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            CallDescPtr call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (_is_phone_call(call->user_id)) {
                continue;
            } else if (!call->outgoing && call->call_state != kCallState_Accepted && call->call_state != kCallState_Connected && call->call_state != kCallState_Disconnected) {
                continue;
            } else {
                ++counter;
            }
        }

        if (!counter) {
            return;
        }

        const bool video_conf = counter > 1;
        for (unsigned ix = 0; ix < _windows.size(); ++ix) {
            WindowDesc& window_desc = _windows[ix];

            using namespace voip2;
            LayoutType lt;

            switch (window_desc.layout_type) {
            case LayoutType_One:    { lt = video_conf ? LayoutType_Two : LayoutType_One; break; }
            case LayoutType_Two:    { lt = video_conf ? LayoutType_Two : LayoutType_One; break; }
            case LayoutType_Three:  { lt = LayoutType_Four; break; }
            case LayoutType_Four:   { lt = LayoutType_Four; break; }
            default: 
                {
                    VOIP_ASSERT(false);
                    lt = LayoutType_Four;
                    break;
                }
            }
            _set_layout(window_desc.handle, lt);
        }
    }

    bool VoipManagerImpl::_init_sounds_sc(std::shared_ptr<voip2::Voip2> engine) {
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        engine->SetSound(voip2::SoundEvent_HangupRemoteBusy, call_busy_data, sizeof(call_busy_data));
        engine->SetSound(voip2::SoundEvent_Connected, call_connected_data, sizeof(call_connected_data));
        engine->SetSound(voip2::SoundEvent_WaitingForAccept_Confirmed, call_dial_data, sizeof(call_dial_data));
        engine->SetSound(voip2::SoundEvent_HangupLocal, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupRemote, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupHandledByAnotherInstance, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupByError, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_Hold, call_hold_data, sizeof(call_hold_data));
        engine->SetSound(voip2::SoundEvent_IncomingInvite, call_incoming_data, sizeof(call_incoming_data));
        engine->SetSound(voip2::SoundEvent_WaitingForAccept, call_start_data, sizeof(call_start_data));

        return true;
    }

    std::shared_ptr<voip2::Voip2> VoipManagerImpl::_get_engine(bool skipCreation/* = false*/) {
        if (_voipDestroyed) {
            return NULL;
        }

        if (!_engine) {
            if (skipCreation) {
                return NULL;
            }

            using namespace voip2;
            std::string stat_file_path = from_unicode_to_utf8(getFilePath());
            VOIP_ASSERT_RETURN_VAL(!stat_file_path.empty(), NULL);

            std::string app_name;
            app_name += "icq.desktop ";
            app_name += core::tools::version_info().get_version();

            _engine.reset(Voip2::CreateVoip2(*this, *this, app_name.c_str(), stat_file_path.c_str()), [] (voip2::Voip2* obj) {
                if (obj) {
                    voip2::Voip2::DestroyVoip2(obj);
                }
            });
            VOIP_ASSERT_RETURN_VAL(!!_engine, NULL);
            VOIP_ASSERT_RETURN_VAL(!!_engine->Init(), NULL);

            _engine->EnableMsgQueue();
            _init_signaling(_engine);
            _init_media    (_engine);
            _init_sounds_sc(_engine);

            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _engine->MuteAllIncomingSoundNotifications(_voip_desc.incomingSoundsMuted);
            _storeDefaultDevices(_engine);
            _defaultDevicesStored = true;

            const VoipSettings voipSettings = _getVoipSettings();
            _engine->EnableRtpDump(voipSettings.enableRtpDump);

            _update_device_list(AudioPlayback);
            _update_device_list(AudioRecording);
            _update_device_list(VideoCapturing);
        }
        return _engine;
    }

    void VoipManagerImpl::_storeDefaultDevices(std::shared_ptr<voip2::Voip2> engine) {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        const voip2::DeviceType devices [] = { voip2::AudioRecording, voip2::AudioPlayback };

        for (unsigned ix = 0; ix < sizeof(devices) / sizeof(devices[0]); ++ix) {
            const voip2::DeviceType device = devices[ix];
            if (engine->GetDevicesNumber(device) == 0) { continue; }

            unsigned defaultDeviceIx = 0;
            if (voip2::AudioRecording == device || voip2::AudioPlayback == device) {
                defaultDeviceIx = (unsigned)-1;
            }

            std::string& defaultDevice = voip2::AudioPlayback == device ? _voip_desc.aPlaybackDefDevice :
                                         voip2::AudioRecording == device ? _voip_desc.aCaptureDefDevice :
                                         _voip_desc.vCaptureDefDevice;


            char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
            char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
            if (!!engine->GetDevice(device, defaultDeviceIx, name, guid)) {
                defaultDevice = guid;
            }
        }
    }

    bool VoipManagerImpl::_call_exists(const CallKey& key) {
        CallDescPtr desc;
        return _call_get(key, desc);
    }

    bool VoipManagerImpl::_call_create(const std::string& account_id, const std::string& user_id, CallDescPtr& desc) {
        VOIP_ASSERT_RETURN_VAL(!account_id.empty(), false);
        VOIP_ASSERT_RETURN_VAL(!user_id.empty(), false);

        const CallKey key = getHash(account_id, user_id);
        VOIP_ASSERT_RETURN_VAL(!!key, false);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT_RETURN_VAL(!_call_exists(key), false);

        desc.reset(new(std::nothrow) CallDesc());
        VOIP_ASSERT_RETURN_VAL(!!desc, false);;

        desc->account_id    = account_id;
        desc->user_id       = user_id;
        desc->started       = 0;
        desc->remote_cam_en = false;
        desc->remote_mic_en = false;
        desc->outgoing      = false;
        desc->call_state    = kCallState_Created;
        desc->quality       = 0;
        desc->call_key      = key;
        desc->close_reason  = voip2::SE_OPEN_FIRST;

        _calls.push_back(desc);
        return true;
    }

    void VoipManagerImpl::_notify_remote_video_state_changed() {
        const bool remote_video_en = remote_video_enabled();
        SIGNAL_NOTIFICATOION(kNotificationType_MediaRemVideoChanged, &remote_video_en);
    }

    bool VoipManagerImpl::_call_destroy(const CallKey& key) {
        VOIP_ASSERT_RETURN_VAL(!!key, false);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (CallDescArray::iterator it = _calls.begin(); it != _calls.end(); ++it) {
            CallDescPtr call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (call->call_key == key) {
                _calls.erase(it);
                return true;
            }
        }
        return false;
    }

    VoipManagerImpl::CallDescPtr VoipManagerImpl::_get_phone_call() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (CallDescArray::iterator it = _calls.begin(); it != _calls.end(); ++it) {
            CallDescPtr call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (_is_phone_call(call->user_id)) {
                return call;
            }
        }
        return NULL;
    }

    bool VoipManagerImpl::_is_phone_call(const std::string& uid) const {
        return (protoNamePstn == getProtoName(uid));
    }

    bool VoipManagerImpl::_have_phone_call() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        return NULL != _get_phone_call();
    }

    void VoipManagerImpl::media_video_en(bool enable) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->EnableOutgoingVideo(enable);

        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.local_cam_en = enable;
        }

        SIGNAL_NOTIFICATOION(kNotificationType_MediaLocVideoChanged, &enable);
        _update_video_window_state();
    }

    void VoipManagerImpl::media_audio_en(bool enable) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->EnableOutgoingAudio(enable);

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        _voip_desc.local_aud_en = enable;

        SIGNAL_NOTIFICATOION(kNotificationType_MediaLocAudioChanged, &enable);
    }

    bool VoipManagerImpl::local_video_enabled() {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_cam_en;
    }

    bool VoipManagerImpl::local_audio_enabled() {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_aud_en;
    }

    bool VoipManagerImpl::remote_video_enabled() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (unsigned ix = 0; ix < _calls.size(); ++ix) {
            auto call = _calls[ix];
            VOIP_ASSERT_ACTION(!!call, continue);

            if (call->remote_cam_en) {
                return true;
            }
        }
        return false;
    }

    bool VoipManagerImpl::remote_video_enabled(const std::string& account, const std::string& contact) {
        CallDescPtr desc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT_RETURN_VAL(_call_get(getHash(account, contact), desc), false);

        return desc->remote_cam_en;
    }

    void VoipManagerImpl::VideoStreamChanged(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) {
        
    }

    bool VoipManagerImpl::remote_audio_enabled() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (unsigned ix = 0; ix < _calls.size(); ++ix) {
            auto call = _calls[ix];
            VOIP_ASSERT_ACTION(!!call, continue);

            if (call->remote_mic_en) {
                return true;
            }
        }
        return false;
    }

    bool VoipManagerImpl::remote_audio_enabled(const std::string& account, const std::string& contact) {
        CallDescPtr desc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT_RETURN_VAL(_call_get(getHash(account, contact), desc), false);

        return desc->remote_mic_en;
    }

    bool VoipManagerImpl::_call_get(const CallKey& key, CallDescPtr& desc) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (unsigned ix = 0; ix < _calls.size(); ++ix) {
            auto call = _calls[ix];
            VOIP_ASSERT_ACTION(!!call, continue);

            if (call->call_key == key) {
                desc = call;
                return true;
            }
        }
        return false;
    }

    bool VoipManagerImpl::phone_call_start(const Contact& contact) {
        VOIP_ASSERT_RETURN_VAL(!contact.contact.empty(), false);
        VOIP_ASSERT_RETURN_VAL(!_have_phone_call(), false);
        VOIP_ASSERT_RETURN_VAL(!contact.account.empty(), false);

        if (call_get_count() > 0) {
            call_stop();
        }

        std::string num = contact.contact;
        if (std::string::npos == num.find(PSTN_POSTFIX)) {
            num += PSTN_POSTFIX;
        }

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        engine->CallStart(contact.account.c_str(), num.c_str());
        return true;
    }

    bool VoipManagerImpl::phone_call_send_dtmf(int num) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        auto desc = _get_phone_call();
        VOIP_ASSERT_RETURN_VAL(!!desc, false);

        if (desc->user_id.find(PSTN_POSTFIX) == std::string::npos) {
            desc->user_id += PSTN_POSTFIX;
        }

        engine->SendAndPlayOobDTMF(desc->user_id.c_str(), num);
        return true;
    }

    bool VoipManagerImpl::phone_call_stop() {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        auto call = _get_phone_call();
        VOIP_ASSERT_RETURN_VAL(!!call, false);
        VOIP_ASSERT_RETURN_VAL(!call->user_id.empty(), false);

        if (call->user_id.find(PSTN_POSTFIX) == std::string::npos) {
            call->user_id += PSTN_POSTFIX;
        }

        engine->CallDecline(call->user_id.c_str());
        return true;
    }

    voip2::ProxyType VoipManagerImpl::_toVoipProxyType(const VoipProxySettings::eProxyType& type) {
        switch (type) {
        case VoipProxySettings::kProxyType_None:    return voip2::ProxyType_None;
        case VoipProxySettings::kProxyType_Http:    return voip2::ProxyType_HTTP;
        case VoipProxySettings::kProxyType_Socks4:  return voip2::ProxyType_SOCKS4;
        case VoipProxySettings::kProxyType_Socks4a: return voip2::ProxyType_SOCKS4;
        case VoipProxySettings::kProxyType_Socks5:  return voip2::ProxyType_SOCKS5;
        }

        VOIP_ASSERT(false);
        return voip2::ProxyType_None;
    }

    void VoipManagerImpl::call_set_proxy(const VoipProxySettings& proxySettings) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetProxyPrms(
            _toVoipProxyType(proxySettings.type), 
            from_unicode_to_utf8(proxySettings.serverUrl).c_str(), 
            from_unicode_to_utf8(proxySettings.userName).c_str(), 
            from_unicode_to_utf8(proxySettings.userPassword).c_str()
            );
    }

    void VoipManagerImpl::call_create(const Contact& contact, bool video, bool add) {
        VOIP_ASSERT_RETURN(!contact.account.empty());
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        bool havePhoneCall;
        bool haveCall;
        {
            std::lock_guard<std::recursive_mutex> __lock(_callsMx);
            haveCall = !_calls.empty();
            havePhoneCall = _have_phone_call();
        }

        if (havePhoneCall) {
            return;
        }

        if (haveCall && !add) {
            bool localVideoEnabled;
            {
                std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
                localVideoEnabled = _voip_desc.local_cam_en;
            }
            if (video && !localVideoEnabled) {
                media_video_en(true);
            } else if (!video && localVideoEnabled) {
                media_video_en(false);
            }
            return;
        }

        media_video_en(video);
        engine->CallStart(contact.account.c_str(), contact.contact.c_str());
    }

    void VoipManagerImpl::call_stop() {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->CallStop();
    }

    void VoipManagerImpl::call_stop_smart(const std::function<void()>& callback) {
        auto engine = _get_engine(true);
        if(!engine) {
            callback();
            return;
        }

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        const bool haveCalls = !_calls.empty();
        callback_ = callback;

        engine->CallStop();
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);
            if (!call->outgoing && call->call_state != kCallState_Accepted) {
                engine->CallDecline(call->user_id.c_str());
            }
        }

        if (haveCalls) {
            sendViaIm_ = true;
        } else {
            callback();
            callback_ = std::function<void()>();
        }
    }

    void VoipManagerImpl::call_accept(const Contact& contact, bool video) {
        VOIP_ASSERT_RETURN(!contact.account.empty());
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        media_video_en(video);
        engine->CallAccept(contact.account.c_str(), contact.contact.c_str());
    }

    void VoipManagerImpl::call_decline(const Contact& contact, bool busy) {
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->CallDecline(contact.contact.c_str(), busy);
    }

    unsigned VoipManagerImpl::call_get_count() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        unsigned counter = 0;
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!_is_phone_call(call->user_id)) {
                counter++;
            }
        }
        return counter;       
    }

    void VoipManagerImpl::call_request_calls() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        std::vector<Contact> calls;
        calls.reserve(_calls.size());

        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!_is_phone_call(call->user_id)) {
                calls.push_back(Contact(normalizeUid(call->account_id), normalizeUid(call->user_id)));
            }
        }

        SIGNAL_NOTIFICATOION(kNotificationType_CallPeerListChanged, &calls);
    }

    bool VoipManagerImpl::call_have_call(const Contact& contact) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!_is_phone_call(call->user_id)) {
                if (normalizeUid(call->account_id) == contact.account && normalizeUid(call->user_id) == contact.contact) {
                    return true;
                }
            }
        }
        return false;       
    }

    bool VoipManagerImpl::call_have_established_connection() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!_is_phone_call(call->user_id)) {
                if (call->call_state >= kCallState_Accepted) {
                    return true;
                }
            }
        }
        return false;       
    }

    bool VoipManagerImpl::_call_have_outgoing_connection() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (CallDescArray::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!_is_phone_call(call->user_id)) {
                if (call->outgoing) {
                    return true;
                }
            }
        }
        return false;       
    }

    VoipManagerImpl::WindowDesc* VoipManagerImpl::_find_window(void* handle) {
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
        for (unsigned ix = 0; ix < _windows.size(); ++ix) {
            WindowDesc& window_description = _windows[ix];
            if (window_description.handle == handle) {
                return &window_description;
            }
        }
        return NULL;
    }

    void VoipManagerImpl::window_add(voip_manager::WindowParams& windowParams) {
        VOIP_ASSERT_RETURN(!!windowParams.hwnd);

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        
        WindowDesc window_description;
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            if (NULL != _find_window(windowParams.hwnd)) {
                return;
            }

            window_description.handle           = windowParams.hwnd;
            window_description.aspectRatio      = 0.0f;
            window_description.scaleCoeffitient = windowParams.scale >= 0.01 ? windowParams.scale : 1.0f;
            window_description.layout_type      = voip2::LayoutType_One;
            _windows.push_back(window_description);
        }

        voip2::WindowSettings ws;
        if (windowParams.isPrimary) {
            getPriWindowSettings(ws, window_description.scaleCoeffitient);
        } else if (windowParams.isSystem) {
            getSysWindowSettings(ws, window_description.scaleCoeffitient);
        } else {
            getSecWindowSettings(ws, window_description.scaleCoeffitient);
        }

        void* hbmp = NULL;
        if (windowParams.watermark.bitmap.data && windowParams.watermark.bitmap.size && windowParams.watermark.bitmap.w && windowParams.watermark.bitmap.h) {
            hbmp = bitmapHandleFromRGBData(windowParams.watermark.bitmap.w, windowParams.watermark.bitmap.h, sizeof(unsigned) * 8, windowParams.watermark.bitmap.data, true);

            if (hbmp) {
                ws.avatarMain[voip2::LayoutType_One].logoImage    = hbmp;
                ws.avatarMain[voip2::LayoutType_One].logoOffsetL  = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                ws.avatarMain[voip2::LayoutType_One].logoOffsetT  = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                ws.avatarMain[voip2::LayoutType_One].logoOffsetR  = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                ws.avatarMain[voip2::LayoutType_One].logoOffsetB  = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                ws.avatarMain[voip2::LayoutType_One].logoPosition = voip2::Position_Top | voip2::Position_Right;
            }
        }

        engine->WindowAdd(windowParams.hwnd, ws);
        const intptr_t winId = (intptr_t)windowParams.hwnd;
        SIGNAL_NOTIFICATOION(kNotificationType_VoipWindowAddComplete, &winId)

        if (windowParams.isSystem) {
            _set_layout(windowParams.hwnd, voip2::LayoutType_Two);
        } else if (windowParams.isPrimary) {
            engine->WindowSetAvatarPosition(windowParams.hwnd, voip2::Position_Left | voip2::Position_Top);
            _update_layout();
        } else {
            _set_layout(windowParams.hwnd, voip2::LayoutType_Two);
        }

        if (hbmp) {
            bitmapReleaseHandle(hbmp);
        }
    }

    void VoipManagerImpl::window_remove(void* hwnd) {
        auto engine = _get_engine(true);
        if (!engine) {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            _windows.clear();
            return;
        }
        
        engine->WindowRemove(hwnd);
        const intptr_t winId = (intptr_t)hwnd;
        SIGNAL_NOTIFICATOION(kNotificationType_VoipWindowRemoveComplete, &winId)

        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
        WindowDesc* desc = _find_window(hwnd);
        if (NULL == desc) {
	        return;
        }

        for (auto it = _windows.begin(); it != _windows.end(); ++it) {
            if (&*it == desc) {
                _windows.erase(it);
                return;
            }
        }
    }

    void VoipManagerImpl::_window_set_bg(void* hwnd, void* hbmp) {
        VOIP_ASSERT_RETURN(hbmp);
        VOIP_ASSERT_RETURN(hwnd);

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowSetBackground(hwnd, hbmp);
    }

    void VoipManagerImpl::window_set_bitmap(const WindowBitmap& bmp) {
        VOIP_ASSERT_RETURN(bmp.hwnd);
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);

        void* hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, sizeof(unsigned) * 8, bmp.bitmap.data, true);
        VOIP_ASSERT_RETURN(hbmp);

        _window_set_bg(bmp.hwnd, hbmp);
        bitmapReleaseHandle(hbmp);
    }

    void VoipManagerImpl::_window_set_av(const std::string& contact, voip2::AvatarType type, void* hbmp) {
        VOIP_ASSERT_RETURN(hbmp);
        VOIP_ASSERT_RETURN(!contact.empty());

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        if (voip2::AvatarType_Camera == type || voip2::AvatarType_CameraCrossed == type) {
            engine->WindowSetAvatar(PREVIEW_RENDER_NAME, hbmp, type);
        } else {
            engine->WindowSetAvatar(contact.c_str(), hbmp, type);
        }
    }

    void VoipManagerImpl::window_set_bitmap(const UserBitmap& bmp) {
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);

        void* hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, sizeof(unsigned) * 8, bmp.bitmap.data, true);
        VOIP_ASSERT_RETURN(hbmp);

        _window_set_av(bmp.contact, bmp.type, hbmp);
        bitmapReleaseHandle(hbmp);
    }

    void VoipManagerImpl::window_switch_layout(void* hwnd) {
        const unsigned num_calls = call_get_count();
        const bool videoconf     = num_calls > 1;
        VOIP_ASSERT_RETURN(num_calls);

        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
        auto window_description = _find_window(hwnd);
        VOIP_ASSERT_RETURN(NULL != window_description);

        using namespace voip2;
        LayoutType lt;
        LayoutType layout_type = window_description->layout_type;

        switch (layout_type) {
        case LayoutType_One:    { lt = LayoutType_Four; break; }
        case LayoutType_Two:    { lt = LayoutType_Four; break; }
        case LayoutType_Three:  { lt = LayoutType_Four; break; }
        case LayoutType_Four:   { lt = videoconf ? LayoutType_Two : LayoutType_One; break; }
        default: 
            {
                VOIP_ASSERT(false);
                lt = voip2::LayoutType(((int)layout_type + 2) % 4);
            }
            break;
        }
        _set_layout(hwnd, lt);
    }

    void VoipManagerImpl::window_switch_aspect(const std::string& contact, void* hwnd) {
        VOIP_ASSERT_RETURN(!contact.empty());

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowSwitchAspectMode(hwnd, contact.c_str());
    }

    void VoipManagerImpl::window_set_offsets(void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowSetControlsStatus(hwnd, true, l, t, r, b, 500, true);
    }

    void VoipManagerImpl::window_add_button(voip2::ButtonType type, voip2::ButtonPosition position) {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowAddButton(type, position);
    }

    bool VoipManagerImpl::get_device_list(voip2::DeviceType device_type, std::vector<device_description>& dev_list) {
        dev_list.clear();

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        const std::string& activeDevice = voip2::AudioPlayback == device_type ? _voip_desc.aPlaybackDevice :
                                          voip2::AudioRecording == device_type ? _voip_desc.aCaptureDevice :
                                          _voip_desc.vCaptureDevice;

        const auto dev_count = engine->GetDevicesNumber(device_type);
        if (!dev_count) {
            return true;
        }

        dev_list.reserve(dev_count);
        char _device_name[voip2::MAX_DEVICE_NAME_LEN];
        char _device_guid[voip2::MAX_DEVICE_GUID_LEN];

        for (unsigned ix = 0; ix < dev_count; ix++) {
            if (!engine->GetDevice(device_type, ix, _device_name, _device_guid)) {
                continue;
            }
            
            device_description desc;
            desc.name = _device_name;
            desc.uid  = _device_guid;
            desc.type = device_type;
            desc.isActive = activeDevice == _device_guid;

            dev_list.push_back(desc);
        }

        assert((dev_count > 0 && !dev_list.empty()) || (!dev_count && dev_list.empty()));
        return true;
    }

    void VoipManagerImpl::_update_device_list(voip2::DeviceType dev_type) {
        std::vector<device_description> dev_list;
        VOIP_ASSERT_RETURN(get_device_list(dev_type, dev_list));

        SIGNAL_NOTIFICATOION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::DeviceListChanged(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        if (!engine) {
            _update_device_list(deviceType);
            VOIP_ASSERT(false);
            return;
        }

        if (engine->GetDevicesNumber(deviceType) == 0) {
            _update_device_list(deviceType);
            return;
        }

        unsigned defaultDeviceIx = 0;
        if (voip2::AudioRecording == deviceType || voip2::AudioPlayback == deviceType) {
            defaultDeviceIx = (unsigned)-1;
        }

        if (voip2::VideoCapturing != deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            if (!_defaultDevicesStored) {
                return;
            }

            std::string& defaultDevice = voip2::AudioPlayback == deviceType ? _voip_desc.aPlaybackDefDevice :
                voip2::AudioRecording == deviceType ? _voip_desc.aCaptureDefDevice :
                _voip_desc.vCaptureDefDevice;

            char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
            char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
            if (!!engine->GetDevice(deviceType, defaultDeviceIx, name, guid)) {
                if (defaultDevice != guid) {
                    set_device(deviceType, guid);
                    defaultDevice = guid;
                }
            }
        }

        std::vector<device_description> dev_list;
        VOIP_ASSERT_RETURN(get_device_list(deviceType, dev_list));

        for (int ix = dev_list.size() - 1; ix >= 0; --ix) {
            device_description& dd = dev_list[ix];
            if (ix == 0 && !dd.isActive) {
                set_device(deviceType, dd.uid);
                dd.isActive = true;
            } else if (dd.isActive) {
                break;
            }
        }

        SIGNAL_NOTIFICATOION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::DeviceStatusChanged(voip2::DeviceType deviceType, const char *uid, voip2::DeviceStatus deviceStatus) {

    }
    
    void VoipManagerImpl::AudioDeviceVolumeChanged(voip2::DeviceType deviceType, float volume) {
        if (voip2::AudioPlayback == deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.volume  = volume;
        }

        DeviceVol vol;
        vol.type   = deviceType;
        vol.volume = volume;
        SIGNAL_NOTIFICATOION(kNotificationType_DeviceVolChanged, &vol);
    }
    
    void VoipManagerImpl::AudioDeviceMuteChanged(voip2::DeviceType deviceType, bool mute) {
        if (voip2::AudioPlayback == deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.mute_en = mute;
        }

        DeviceMute dm;
        dm.mute = mute;
        dm.type = deviceType;
        SIGNAL_NOTIFICATOION(kNotificationType_DeviceMuted, &dm);
    }
    
    void VoipManagerImpl::FrameSizeChanged(voip::hwnd_t hwnd, float aspectRatio) {
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            auto window_description = _find_window(hwnd);
            VOIP_ASSERT_RETURN(NULL != window_description);
            window_description->aspectRatio = aspectRatio;
        }

        FrameSize fs = { 0 };
        fs.hwnd = (intptr_t)hwnd;
        fs.aspect_ratio = aspectRatio;

        SIGNAL_NOTIFICATOION(kNotificationType_FrameSizeChanged, &fs);
    }

    void VoipManagerImpl::LayoutTypeChanged(voip::hwnd_t hwnd, voip2::LayoutType layoutType) {
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            auto window_description = _find_window(hwnd);
            VOIP_ASSERT_RETURN(NULL != window_description);
            window_description->layout_type = layoutType;
        }

        struct LayoutChanged lc = { 0 };
        lc.tray        = voip2::LayoutType_Three == layoutType || voip2::LayoutType_Four == layoutType;
        lc.layout_type = layoutType; 

        SIGNAL_NOTIFICATOION(kNotificationType_LayoutChanged, &lc);
    }
    
    void VoipManagerImpl::RenderMouseTap(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) {
        MouseTap mt;
        mt.area    = viewArea;
        mt.account = normalizeUid(account_uid);
        mt.contact = normalizeUid(user_id);
        mt.hwnd    = hwnd;
        mt.tap     = mouseTap;

        SIGNAL_NOTIFICATOION(kNotificationType_MouseTap, &mt);
    }

    void VoipManagerImpl::ButtonPressed(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) {
#if defined(__PROCESS_BUTTON_TAPS__)
        VOIP_ASSERT_RETURN(!account_uid.empty());
        VOIP_ASSERT_RETURN(!user_id.empty());

        ButtonTap bt;
        bt.account = normalizeUid(account_uid);
        bt.contact = normalizeUid(user_id);
        bt.hwnd    = (HWND)hwnd;
        bt.type    = type;
        DEBUG_ASSERT_IF(!bt.contact);
        SIGNAL_NOTIFICATOION(kNotificationType_ButtonTap, &bt);
#endif
    }

    void VoipManagerImpl::MissedCall(const char* account_uid, const char* user_id, unsigned timestamp) {
        VOIP_ASSERT_RETURN(!!account_uid);
        VOIP_ASSERT_RETURN(!!user_id);

        struct MissedCall mc;
        mc.account = normalizeUid(account_uid);
        mc.contact = normalizeUid(user_id);
        mc.ts      = timestamp;

        SIGNAL_NOTIFICATOION(kNotificationType_MissedCall, &mc);
    }

    void VoipManagerImpl::SessionEvent(const char* _account_uid, const char* _user_id, voip2::SessionEvent sessionEvent) {
        VOIP_ASSERT_RETURN(!!_account_uid);
        VOIP_ASSERT_RETURN(!!_user_id);

        const std::string account_uid = _account_uid;
        const std::string user_id = _user_id;

        using namespace voip2;
        const voip2::SessionEvent startSessionEvents[] = {
            SE_OUTGOING_STARTED_AUDIO, SE_OUTGOING_STARTED_VIDEO,
            SE_INCOMING_INVITE_AUDIO, SE_INCOMING_INVITE_VIDEO,
            SE_JOINED_AUDIO, SE_JOINED_VIDEO
        };

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        CallDescPtr desc;
        const bool is_phone             = _is_phone_call(user_id);
        eNotificationTypes notification = kNotificationType_Undefined;

        bool start_new_call_event = false;
        for (unsigned ix = 0; ix < sizeof(startSessionEvents) / sizeof(startSessionEvents[0]); ++ix) {
            if (startSessionEvents[ix] == sessionEvent) {
                start_new_call_event = true;
                break;
            }
        }

        if (start_new_call_event) {
            VOIP_ASSERT_RETURN(!!_call_create(account_uid, user_id, desc));
            if (!is_phone) {
                _load_avatars(account_uid, user_id);
            }
        }

        const CallKey key = desc ? desc->call_key : getHash(account_uid, user_id);
        VOIP_ASSERT_RETURN(!!key);

        if (!desc) {
            VOIP_ASSERT_RETURN(!!_call_get(key, desc));
        }

        ContactEx contact_ex;
        contact_ex.contact.account = normalizeUid(account_uid);
        contact_ex.contact.contact = normalizeUid(user_id);
        contact_ex.call_count      = call_get_count();
        contact_ex.incoming        = !desc->outgoing;

        auto notification_sender = [this, &contact_ex, &user_id] (eNotificationTypes nt, bool ip) {
            if (kNotificationType_Undefined != nt) {
                if (ip) {
                    std::string phoneNumber = getPhoneNumberFromUid(user_id);
                    SIGNAL_NOTIFICATOION(nt, &phoneNumber);
                } else {
                    SIGNAL_NOTIFICATOION(nt, &contact_ex);
                }
            }
        };

        switch (sessionEvent) {
            case SE_OUTGOING_STARTED_AUDIO:
            case SE_OUTGOING_STARTED_VIDEO:
                {
                    std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
                    desc->call_state        = kCallState_Initiated;
                    desc->outgoing          = true;
                    contact_ex.incoming     = !desc->outgoing;
                    _voip_desc.local_cam_en = SE_OUTGOING_STARTED_VIDEO == sessionEvent;

					notification_sender(is_phone ? kNotificationType_PhoneCallCreated : kNotificationType_CallCreated, is_phone);
					_update_peer_list(account_uid, user_id);
                }
                break;

            case SE_OUTGOING_ACCEPTED_AUDIO:
            case SE_OUTGOING_ACCEPTED_VIDEO:
                {
                    desc->call_state    = kCallState_Accepted;
                    desc->remote_cam_en = SE_OUTGOING_ACCEPTED_VIDEO == sessionEvent;
                    notification        = is_phone ? kNotificationType_PhoneCallAccepted : kNotificationType_CallOutAccepted;
                    _notify_remote_video_state_changed();
                    _update_video_window_state();
                }
                break;


            case SE_INCOMING_INVITE_AUDIO:
            case SE_INCOMING_INVITE_VIDEO:
                {
                    desc->call_state    = kCallState_Initiated;
                    desc->outgoing      = false;
                    contact_ex.incoming = !desc->outgoing;
                    desc->remote_cam_en = SE_INCOMING_INVITE_VIDEO == sessionEvent;

                    notification_sender(kNotificationType_CallCreated, false);
                    notification_sender(kNotificationType_CallInvite, false);
                    _notify_remote_video_state_changed();
                    _update_peer_list(account_uid, user_id);

                    media_video_en(true);
                }
                break;

            case SE_INCOMING_ACCEPTED_AUDIO:
            case SE_INCOMING_ACCEPTED_VIDEO:
                {
                    std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
                    desc->call_state        = kCallState_Accepted;
                    _voip_desc.local_cam_en = SE_INCOMING_ACCEPTED_VIDEO == sessionEvent;
                    notification            = kNotificationType_CallInAccepted;
                    _update_video_window_state();
                }
                break;

            case SE_JOINED_AUDIO:
            case SE_JOINED_VIDEO:
                {
                    desc->call_state    = kCallState_Connected;
                    desc->outgoing      = false;
                    contact_ex.incoming = !desc->outgoing;
                    desc->remote_cam_en = SE_JOINED_VIDEO == sessionEvent;
                    desc->started       = (unsigned)clock();
                    notification_sender(kNotificationType_CallCreated, false);
                    _notify_remote_video_state_changed();
                    notification        = is_phone ? kNotificationType_PhoneCallConnected : kNotificationType_CallConnected;
                }
                break;

            case SE_QUALITY_BAD:
            case SE_QUALITY_AUDIO_OK:
            case SE_QUALITY_AUDIO_VIDEO_OK:
                {
                    desc->quality = SE_QUALITY_BAD == sessionEvent ? 30 : 
                                    SE_QUALITY_AUDIO_OK == sessionEvent ? 60 : 100;

                    notification_sender(kNotificationType_QualityChanged, is_phone);
                }
                break;

            case SE_CONNECTED:
                {
                    desc->call_state    = kCallState_Connected;
                    if (!desc->started) {
                        desc->started   = (unsigned)clock();
                    }
                    notification        = is_phone ? kNotificationType_PhoneCallConnected : kNotificationType_CallConnected;
                }
                break;

            case SE_DISCONNECTED:
                {
                    desc->call_state    = kCallState_Disconnected;
                    notification        = is_phone ? kNotificationType_PhoneCallDisconnected : kNotificationType_CallDisconnected;
                }
                break;

            case SE_INCOMING_CONF_PEERS_UPDATED:
                {
                    _update_peer_list(account_uid, user_id);
                    _update_video_window_state();
                }
                break;

            case SE_REMOTE_MIC_ON:
            case SE_REMOTE_MIC_OFF:
                {
                    desc->remote_mic_en = SE_REMOTE_MIC_ON == sessionEvent;
                    notification_sender(kNotificationType_MediaRemAudioChanged, is_phone);
                }
                break;

            case SE_REMOTE_CAM_OFF:
            case SE_REMOTE_CAM_ON:
                {
                    desc->remote_cam_en = SE_REMOTE_CAM_ON == sessionEvent;
                    _notify_remote_video_state_changed();
                    _update_video_window_state();
                }
                break;

            case SE_CLOSED_BY_REMOTE_DECLINE:
            case SE_CLOSED_BY_REMOTE_HANDLED_BY_ANOTHER_INSTANCE:
            case SE_CLOSED_BY_REMOTE_BUSY:
            case SE_CLOSED_BY_REMOTE_ERROR:
            case SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_REMOTE:
            case SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_LOCAL:
            case SE_CLOSED_BY_TIMEOUT_INACTIVE:
            case SE_CLOSED_BY_TIMEOUT_CONNECT_INIT:
            case SE_CLOSED_BY_TIMEOUT_CONNECTION:
            case SE_CLOSED_BY_TIMEOUT_RECONNECT:
            case SE_CLOSED_BY_ERROR_CREATE:
            case SE_CLOSED_BY_ERROR_START:
            case SE_CLOSED_BY_ERROR_INTERNAL:
            case SE_CLOSED_BY_LOCAL_BUSY:
            case SE_CLOSED_BY_LOCAL_HANGUP:
                {
                    notification       = is_phone ? kNotificationType_PhoneCallDestroyed : kNotificationType_CallDestroyed;
                    desc->close_reason = sessionEvent;
                }
                break;

            default:
                break;
        }

        if (start_new_call_event) {
            _update_layout();
            _update_video_window_state();

            if (call_get_count() == 1) {
                DeviceVol vol;
                DeviceMute dm;
                {
                    std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
                    auto e = _get_engine();
                    _voip_desc.mute_en = e->GetDeviceMute(voip2::AudioPlayback);
                    _voip_desc.volume  = e->GetDeviceVolume(voip2::AudioPlayback);

                    vol.type   = voip2::AudioPlayback;
                    vol.volume = _voip_desc.volume;

                    dm.mute = _voip_desc.mute_en;
                    dm.type = voip2::AudioPlayback;
                }

                SIGNAL_NOTIFICATOION(kNotificationType_DeviceMuted, &dm);
                SIGNAL_NOTIFICATOION(kNotificationType_DeviceVolChanged, &vol);
            }

            const bool audio_enabled = local_audio_enabled();
            const bool video_enabled = local_video_enabled();
            SIGNAL_NOTIFICATOION(kNotificationType_MediaLocAudioChanged, &audio_enabled);
            SIGNAL_NOTIFICATOION(kNotificationType_MediaLocVideoChanged, &video_enabled);
        }

        notification_sender(notification, is_phone);
        if (SE_CLOSED_BY_REMOTE_DECLINE <= sessionEvent) {
            _call_destroy(key);
            _update_layout();
            desc = NULL;
            _update_video_window_state();
            call_request_calls();
        }

        _dispatcher.excute_core_context([this] () {
            std::lock_guard<std::recursive_mutex> __lockCalls(_callsMx);
            if (_calls.empty() && sendViaIm_) {
                sendViaIm_ = false;
                if (callback_ != NULL) {
                    callback_();
                }
                callback_ = std::function<void()>();
            }
        });
    }

    void VoipManagerImpl::mute_incoming_call_sounds(bool mute) {
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.incomingSoundsMuted = mute;
        }

        auto engine = _get_engine(true);
        if (!!engine) {
            engine->MuteAllIncomingSoundNotifications(mute);
        }
    }

    void VoipManagerImpl::_protocolSendAlloc(const char* data, unsigned size) {
        core::core_dispatcher& dispatcher = _dispatcher;
        std::string dataBuf(data, size);

        _dispatcher.excute_core_context([dataBuf, &dispatcher, this] () {
            if (!_voipProtocol) {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);

            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);
            
            auto packet = im->prepare_voip_msg(dataBuf);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, this] (int32_t _error) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                auto response = packet->getRawData();
                if (!!response && response->available()) {
                    uint32_t responseSize = response->available();
                    ptrIm->on_voip_proto_msg(true, (const char*)response->read(responseSize), responseSize, std::make_shared<core::auto_callback>([](int32_t){}));
                }
            };
        });
    }

    void VoipManagerImpl::_protocolSendMessage(const VoipProtoMsg& data) {
        core::core_dispatcher& dispatcher = _dispatcher;
        _dispatcher.excute_core_context([data, &dispatcher, this] () {
            if (!_voipProtocol) {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);

            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);

            auto packet = im->prepare_voip_pac(data);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, data, this] (int32_t _error) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                auto response = packet->getRawData();
                bool success  = _error == 0 && !!response && response->available();
                ptrIm->on_voip_proto_ack(data, success);
            };
        });
    }

    void VoipManagerImpl::SendVoipMsg(const char* from, voip2::VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx) {
        VOIP_ASSERT_RETURN(data && len);
        VOIP_ASSERT_RETURN(from);

        using namespace voip2;
        switch(voipOutgoingMsg) {
        case WIM_Outgoing_allocate:
            if (sendViaIm_) {
                _dispatcher.post_voip_alloc(0, data, len);
            } else {
                _protocolSendAlloc(data, len);
            }
            break;

        case WIM_Outgoing_invite:
        case WIM_Outgoing_accept:
        case WIM_Outgoing_decline:
        case WIM_Outgoing_json:
        case WIM_Outgoing_keepalive:
            {
                VoipProtoMsg msg;
                msg.msg       = voipOutgoingMsg;
                msg.request.assign(data, len);
                msg.messageId = msg_idx;

                using namespace std::chrono;
                const milliseconds msNow = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                for (auto it = _requestIds.begin(); it != _requestIds.end();) {
                    RequestInfo& requestInfo = it->second;
                    if (msNow.count() - requestInfo.ts.count() >= 3 * 60 * 1000) { // since last sequense number reuse elapsed 3 min... like in voip
                        it = _requestIds.erase(it);
                    } else {
                        ++it;
                    }
                }

                const char delimiter = '_';
                if (msg_idx) {
                    auto it = _requestIds.find(msg_idx);
                    if (it == _requestIds.end()) {
                        std::stringstream reqIdStr;
                        reqIdStr << msNow.count() << delimiter << rand();
                        msg.requestId = reqIdStr.str();

                        RequestInfo requestInfo;
                        requestInfo.requestId = msg.requestId;
                        requestInfo.messageId = msg_idx;
                        requestInfo.ts        = msNow;

                        _requestIds[msg_idx] = requestInfo;
                    } else {
                        RequestInfo& requestInfo = it->second;
                        requestInfo.ts = msNow;
                        msg.requestId  = requestInfo.requestId;
                    }
                } else {
                    std::stringstream reqIdStr;
                    reqIdStr << msNow.count() << delimiter << rand();
                    msg.requestId = reqIdStr.str();
                }

                VOIP_ASSERT(!msg.requestId.empty());
                if (kUseVoipProtocolAsDefault) {
                    if (sendViaIm_) {
                        _dispatcher.post_voip_message(0, msg);
                    } else {
                        _protocolSendMessage(msg);
                    }
                } else {
                    _dispatcher.post_voip_message(0, msg);
                }
            }
            break;

        default:
            VOIP_ASSERT(false);
            break;
        }
    }

    void VoipManagerImpl::ProcessVoipAck(const std::string& account_uid, const voip_manager::VoipProtoMsg& msg, bool success) {
        VOIP_ASSERT_RETURN(!account_uid.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->ReadVoipAck(account_uid.c_str(), msg.messageId, success);
    }

    void VoipManagerImpl::ProcessVoipMsg(const std::string& account_uid, voip2::VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len) {
        VOIP_ASSERT_RETURN(!account_uid.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->ReadVoipMsg(account_uid.c_str(), voipIncomingMsg, data, len);
    }

    void VoipManagerImpl::_load_avatars(const std::string& account_uid, const std::string& user_uid, bool request_if_not_exists/* = true*/) {   
        _dispatcher.excute_core_context([account_uid, user_uid, this] () {
            auto im = _dispatcher.find_im_by_id(0);
            assert(!!im);

            im->get_contact_avatar(0, user_uid, kAvatarRequestSize);
        });
    }

    unsigned VoipManagerImpl::get_devices_number(voip2::DeviceType device_type) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, 0);

        return engine->GetDevicesNumber(device_type);
    }

    bool VoipManagerImpl::get_device(voip2::DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        char _device_name[voip2::MAX_DEVICE_NAME_LEN];
        char _device_guid[voip2::MAX_DEVICE_GUID_LEN];
        if (engine->GetDevice(device_type, index, _device_name, _device_guid)) {
            device_name = _device_name;
            device_guid = _device_guid;
            return true;
        }
        return false;
    }

    void VoipManagerImpl::AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) {
        
    }

    void VoipManagerImpl::InterruptByGsmCall(bool gsmCallStarted) {
        
    }

    void VoipManagerImpl::_update_video_window_state() {
        const auto call_count                  = call_get_count();
        const bool have_established_connection = call_have_established_connection();
        const bool have_outgoing               = _call_have_outgoing_connection();
        const bool have_video                  = local_video_enabled() || remote_video_enabled();

        const bool enable_video = call_count > 1 || (have_video && call_count && (have_established_connection || have_outgoing));
        SIGNAL_NOTIFICATOION(kNotificationType_ShowVideoWindow, &enable_video);
    }

    void VoipManagerImpl::set_device(voip2::DeviceType device_type, const std::string& device_guid) {
        VOIP_ASSERT_RETURN(voip2::MAX_DEVICE_GUID_LEN >= device_guid.length());
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            switch(device_type) {
            case voip2::AudioRecording: { _voip_desc.aCaptureDevice  = device_guid; } break;
            case voip2::AudioPlayback:  { _voip_desc.aPlaybackDevice = device_guid; } break;
            case voip2::VideoCapturing: { _voip_desc.vCaptureDevice  = device_guid; } break;
            default: { VOIP_ASSERT(false); return; }
            }
        }

        auto engine = _engine;
        if (!!engine) {
            engine->SetDevice(device_type, device_guid.c_str());
        }

    }

    void VoipManagerImpl::set_device_mute(voip2::DeviceType deviceType, bool mute) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetDeviceMute(deviceType, mute);
    }

    bool VoipManagerImpl::get_device_mute(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        return engine->GetDeviceMute(deviceType);
    }

    void VoipManagerImpl::set_device_volume(voip2::DeviceType deviceType, float volume) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetDeviceVolume(deviceType, volume);
    }

    void VoipManagerImpl::update() {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
    }

    float VoipManagerImpl::get_device_volume(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, 0.0f);

        return engine->GetDeviceVolume(deviceType);
    }

    void VoipManagerImpl::reset() {
        _voipDestroyed = true;
        _engine.reset();
        SIGNAL_NOTIFICATOION(kNotificationType_VoipResetComplete, &_voipDestroyed);
    }

    VoipManager::VoipManager(core::core_dispatcher& dispatcher) {
        _impl.reset(new(std::nothrow) VoipManagerImpl(dispatcher));
    }

    std::shared_ptr<ICallManager> VoipManager::get_call_manager() {
        return _impl;
    }

    std::shared_ptr<IWindowManager> VoipManager::get_window_manager() {
        return _impl;
    }

    std::shared_ptr<IMediaManager> VoipManager::get_media_manager() {
        return _impl;
    }

    std::shared_ptr<IDeviceManager> VoipManager::get_device_manager() {
        return _impl;
    }

    std::shared_ptr<IConnectionManager> VoipManager::get_connection_manager() {
        return _impl;
    }

    std::shared_ptr<IVoipManager> VoipManager::get_voip_manager() {
        return _impl;
    }
}