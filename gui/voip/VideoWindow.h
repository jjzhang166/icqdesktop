#ifndef __VIDEO_WINDOW_H__
#define __VIDEO_WINDOW_H__

#include "VideoFrame.h"
#include "DetachedVideoWnd.h"

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
    class VoipSysPanelHeader;
    class VideoPanel;
    class MaskPanel;
    class SecureCallWnd;

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
#ifndef __APPLE__
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
#endif
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent * event) override;
        void mousePressEvent(QMouseEvent * event) override;
        void wheelEvent(QWheelEvent * event) override;
        void moveEvent(QMoveEvent * event) override;
        
        // @return true if we resend message to any transporent panel.
        template <typename E> bool resendMouseEventToPanel(E* event_);
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

		void showPanel(QWidget* widget);
		void hidePanel(QWidget* widget);

		void fadeInPanels(int kAnimationDefDuration);
		void fadeOutPanels(int kAnimationDefDuration);
		void hidePanels();

        // call this method in all cases, when you need to hide panels.
        void tryRunPanelsHideTimer();
        void hideSecurityDialog();

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

		void onSetPreviewPrimary();
		void onSetContactPrimary();

        void onShowMaskList();
        void onHideMaskList();

        void getCallStatus(bool& isAccepted);

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();
        bool isActiveWindow();

    private:
        FrameControl_t* rootWidget_;

        // List of all panels of video window.
		std::vector<BaseVideoPanel*> panels_;
        
#ifndef _WIN32
        // Transporent widgets, we resend mouse events to this widgets under mac,
        // because we did not find better way to catch mouse events for tranporent panels.
		std::vector<BaseVideoPanel*> transporentPanels_;
#endif
        
		std::unique_ptr<VideoPanelHeader>   topPanelSimple_;
		std::unique_ptr<VoipSysPanelHeader> topPanelOutgoing_;
        std::unique_ptr<VideoPanel>          videoPanel_;
		std::unique_ptr<MaskPanel>           maskPanel_;

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
        
        // We have own double click processing,
        // Because Qt has bug with double click and fullscreen.
        QTime doubleClickTime_;
#endif
    };
}

#endif//__VIDEO_WINDOW_H__
