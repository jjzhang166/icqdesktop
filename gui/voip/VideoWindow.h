#ifndef __VIDEO_WINDOW_H__
#define __VIDEO_WINDOW_H__

#include "VideoPanel.h"
#include "VideoPanelHeader.h"
#include "DetachedVideoWnd.h"
#include "VoipSysPanelHeader.h"
#include "VideoFrame.h"
#include "secureCallWnd.h"
#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

namespace voip_manager
{
    struct CipherState;
    struct FrameSize;
}

namespace Ui
{
	class DetachedVideoWindow;
	class ShadowWindow;

    class VideoWindow : public AspectRatioResizebleWnd
    {
        Q_OBJECT
        
    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void resizeEvent(QResizeEvent* _e) override;
        void closeEvent(QCloseEvent *) override;
        void paintEvent(QPaintEvent *_e) override;
        void changeEvent(QEvent *) override;
        void escPressed() override;
        void executeCommandList();

        
#ifndef _WIN32
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
#endif

    private:
        quintptr getContentWinId() override;
        void updatePanels() const;
        void _switchFullscreen();
        void checkPanelsVisibility(bool _forceHide = false);
        inline void updateWindowTitle();
        bool isOverlapped() const;

        // @return true, if we was able to load size from settings.
        bool getWindowSizeFromSettings(int& _nWidth, int& _nHeight);
        
        void offsetWindow(int _bottom);
        void updateUserName();

		// We use this proxy to catch methods call during fullscreen animation.
		// we will save commands and call it later, after fullscreen animation.
		void callMethodProxy(const QString& _method);

    private Q_SLOTS:
        void checkOverlap();
        void checkPanelsVis();
        void onVoipMouseTapped(quintptr, const std::string& _tapType);
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipMediaRemoteVideo(bool _enabled);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onEscPressed();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onPanelMouseEnter();
        void onPanelMouseLeave();
        void onPanelFullscreenClicked();

        void onVoipUpdateCipherState(const voip_manager::CipherState& _state);
        void onSecureCallClicked(const QRect&);
        void onSecureCallWndOpened();
        void onSecureCallWndClosed();
        
        // This methods used under mac to correct close video window, when it is in full screen.
        void fullscreenAnimationStart(); // Calls on mac, when we start fullscreen animation.
        void fullscreenAnimationFinish(); // Calls on mac, when we finish fullscreen animation.
        void activeSpaceDidChange(); // Calls on mac, when active work sapce changed.
        
        void showVideoPanel();
        void autoHideToolTip(bool& autoHide);
        void companionName(QString& name);
        void showToolTip(bool &show);

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();
        bool isActiveWindow();

    private:
        FrameControl_t* rootWidget_;

        template<typename __Type> class VideoPanelUnit
        {
            std::unique_ptr<__Type> panel_;
            UIEffects*              effect_;

        public:
            VideoPanelUnit();
            ~VideoPanelUnit();

            bool init(const std::function<__Type*()>& creator);

            void fadeIn(unsigned int _ms);
            void fadeOut(unsigned int _ms);

            void show();
            void hide();

            WId getId() const;
            operator __Type*();
            operator __Type*() const;
            bool operator!() const;
            __Type* operator->();
            const __Type* operator->() const;
        };

        VideoPanelUnit<VideoPanelHeader>   topPanelSimple_;
        VideoPanelUnit<VoipSysPanelHeader> topPanelOutgoing_;

        std::unique_ptr<VideoPanel>          videoPanel_;
        //std::unique_ptr<VideoPanelHeader>    video_panel_header_;
        //std::unique_ptr<VoipSysPanelHeader>  video_panel_header_with_avatars_;

        std::unique_ptr<DetachedVideoWindow> detachedWnd_;
        ResizeEventFilter*     eventFilter_;

        QTimer checkOverlappedTimer_;
        QTimer showPanelTimer_;

        bool hasRemoteVideo_;
        bool outgoingNotAccepted_;

        // Current call contacts
        std::vector<voip_manager::Contact> currentContacts_;

        //UIEffects* video_panel_header_effect_;
        //UIEffects* video_panel_header_effect_with_avatars_;

        QWidget *widget_;

        SecureCallWnd* secureCallWnd_;

        struct
        {
            std::string name;
            unsigned time;
        } callDescription;
        
		// Shadow of this widnow.
		ShadowWindowParent shadow_;
        
#ifdef __APPLE__
        // We use notification about fullscreen to fix problem on mac.
        platform_macos::FullScreenNotificaton _fullscreenNotification;
        // This is list of commands, which we call after fullscreen.
        QList<QString> commandList_;
        bool isFullscreenAnimation_;
        bool changeSpaceAnimation_;
#endif
    };
}

#endif//__VIDEO_WINDOW_H__