#ifndef __VOIP_PROXY_H__
#define __VOIP_PROXY_H__
#include "../../core/Voip/VoipManagerDefines.h"

namespace core{
	class coll_helper;
}

namespace Ui{
	class core_dispatcher;
	class gui_coll_helper;
}

namespace voip_manager {
    struct Contact;
    struct ContactEx;
    struct FrameSize;
}

namespace voip_proxy {

	enum evoip_dev_types {
        kvoip_dev_type_undefined      = 0,
        kvoip_dev_type_audio_playback = 1,
        kvoip_dev_type_audio_capture  = 2,
        kvoip_dev_type_video_capture  = 3
    };

    struct device_desc {
        std::string name;
        std::string uid;
        evoip_dev_types dev_type;
        bool isActive;
    };

    class VoipEmojiManager
    {
        struct CodeMap   { size_t id; size_t codePointSize ;std::string path; unsigned sw; unsigned sh; std::vector<unsigned> codePoints; };
        
        std::vector<CodeMap>   codeMaps_;
        size_t                 activeMapId_;
        QImage                 activeMap_;

    public:
        VoipEmojiManager();
        ~VoipEmojiManager();

    public:
        bool addMap(const unsigned sw, const unsigned sh, const std::string& path, const std::vector<unsigned>& codePoints, const size_t size);
        bool getEmoji(const unsigned codePoint, const size_t size, QImage& image);
    };

    class VoipController : public QObject {
        
	Q_OBJECT

	Q_SIGNALS:
        void onVoipShowVideoWindow(bool show);
        void onVoipVolumeChanged(const std::string& device_type, int vol);
        void onVoipMuteChanged(const std::string& device_type, bool muted);
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts);
        void onVoipCallIncoming(const std::string& account, const std::string& contact);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& contact_ex);
        void onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>& devices);
        void onVoipMediaLocalAudio(bool enabled);
        void onVoipMediaLocalVideo(bool enabled);
        void onVoipMediaRemoteVideo(bool enabled);
        void onVoipMouseTapped(quintptr win_id, const std::string& tap_type);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& contact_ex);
        void onVoipCallCreated(const voip_manager::ContactEx& contact_ex);
        void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);
        void onVoipCallConnected(const voip_manager::ContactEx& contact_ex);
		void onVoipCallTimeChanged(unsigned sec_elapsed, bool have_call);
        void onVoipFrameSizeChanged(const voip_manager::FrameSize& fs);
        void onVoipResetComplete(); //hack for correct close
        void onVoipWindowRemoveComplete(quintptr win_id);
        void onVoipWindowAddComplete(quintptr win_id);
        void onVoipUpdateCipherState(const voip_manager::CipherState& state);

	private Q_SLOTS:
		void _updateCallTime();
		void _checkIgnoreContact(QString contact);

	private:
        voip_manager::CipherState _cipherState;
		Ui::core_dispatcher& _dispatcher;
		QTimer               _call_time_timer;
		unsigned             _call_time_elapsed;
        VoipEmojiManager     _voipEmojiManager;
        std::vector<voip_manager::Contact> _activePeerList;
		bool				 _haveEstablishedConnection;
        bool                 _iTunesWasPaused;

		void _loadUserBitmaps(Ui::gui_coll_helper& collection, QPixmap* avatar, const std::string& contact, int size);

		// Load and apply volume from settings.
		void loadPlaybackVolumeFromSettings();

		// This method is called, when you start or recive call.
		void onStartCall();
		// This method is called on end of call.
		void onEndCall();

	public:
		VoipController(Ui::core_dispatcher& dispatcher);
		virtual ~VoipController();

	public:
        void voipReset();
        void updateActivePeerList();
        void getSecureCode(voip_manager::CipherState& state) const;
        VoipEmojiManager& getEmojiManager() { return _voipEmojiManager; }

		void setWindowAdd(quintptr hwnd, bool primary_wnd, bool system_wnd, int panel_height);
		void setWindowRemove(quintptr hwnd);
		void setWindowOffsets(quintptr hwnd, int lpx, int tpx, int rpx, int bpx);
		void setAvatars(QPixmap& data, int size, const char* contact);

		void setStartA(const char* contact, bool attach);
		void setStartV(const char* contact, bool attach);
		void setHangup();
		void setAcceptA(const char* contact);
		void setAcceptV(const char* contact);
		void setDecline(const char* contact, bool busy);

		void setSwitchAPlaybackMute();
		void setSwitchACaptureMute();
		void setSwitchVCaptureMute();
		void setVolumeAPlayback(int volume);
        void setRequestSettings();
        void setActiveDevice(const device_desc& description);
        void setMuteSounds(bool mute);

		void setAPlaybackMute(bool mute);

		void handlePacket(core::coll_helper& coll_params);
        static std::string formatCallName(const std::vector<std::string>& names, const char* clip);
	};
}

#endif//__VOIP_PROXY_H__