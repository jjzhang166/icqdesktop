#ifndef __VIDEO_WINDOW_H__
#define __VIDEO_WINDOW_H__

#include "VideoPanel.h"
#include "VideoPanelHeader.h"
#include "DetachedVideoWnd.h"
#include "VoipSysPanelHeader.h"
#include "VideoFrame.h"
#include "secureCallWnd.h"

namespace voip_manager{
    struct CipherState;
    struct FrameSize;
}

namespace Ui {
    
    class DetachedVideoWindow;
    namespace video_window {
        class ResizeEventFilter : public QObject {
            Q_OBJECT

        public:
            ResizeEventFilter(std::vector<QWidget*>& top_panels, std::vector<QWidget*>& bottom_panels, QObject* parent);

        protected:
            bool eventFilter(QObject* obj, QEvent* event);

        private:
            std::vector<QWidget*> _top_panels;
            std::vector<QWidget*> _bottom_panels;
        };
    }

    class VideoWindow : public AspectRatioResizebleWnd {
        Q_OBJECT
        
    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void resizeEvent(QResizeEvent* e) override;
        void closeEvent(QCloseEvent *) override;
        void paintEvent(QPaintEvent *e) override;
        void changeEvent(QEvent *) override;
        void escPressed() override;
        
#ifndef _WIN32
        void mouseDoubleClickEvent(QMouseEvent* e) override;
        void enterEvent(QEvent* e) override;
        void leaveEvent(QEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;
#endif

	private:
        quintptr getContentWinId() override;
        void updatePanels_() const;
        void _switchFullscreen();
        void _checkPanelsVisibility(bool forceHide = false);
        inline void updateWindowTitle_();

		// @return true, if we was able to load size from settings.
		bool getWindowSizeFromSettings(int& nWidth, int& nHeight);

    private Q_SLOTS:
        void _check_overlap();
		void _check_panels_vis();
        void onVoipMouseTapped(quintptr, const std::string& tap_type);
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);
		void onVoipCallTimeChanged(unsigned sec_elapsed, bool have_call);
        void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);
        void onVoipMediaRemoteVideo(bool enabled);
        void onVoipWindowRemoveComplete(quintptr win_id);
        void onVoipWindowAddComplete(quintptr win_id);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& contact_ex);
        void onVoipCallCreated(const voip_manager::ContactEx& contact_ex);
        void _escPressed();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

		void onPanelMouseEnter();
		void onPanelMouseLeave();
		void onPanelFullscreenClicked();

        void onVoipUpdateCipherState(const voip_manager::CipherState& state);
        void onSecureCallClicked(const QRect&);
        void onSecureCallWndOpened();
        void onSecureCallWndClosed();

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();

    private:
        FrameControl_t* _rootWidget;

        template<typename __Type> class VideoPanelUnit
        {
            std::unique_ptr<__Type> panel_;
            UIEffects*              effect_;

        public:
            VideoPanelUnit();
            ~VideoPanelUnit();

            bool init(const std::function<__Type*()>& creator);

            void fadeIn(unsigned int ms);
            void fadeOut(unsigned int ms);

            void show();
            void hide();

            WId getId();
            operator __Type*();
            operator __Type*() const;
            bool operator!() const;
            __Type* operator->();
            const __Type* operator->() const;
        };

        VideoPanelUnit<VideoPanelHeader>   topPanelSimple_;
        VideoPanelUnit<VoipSysPanelHeader> topPanelOutgoing_;

        std::unique_ptr<VideoPanel>          video_panel_;
        //std::unique_ptr<VideoPanelHeader>    video_panel_header_;
        //std::unique_ptr<VoipSysPanelHeader>  video_panel_header_with_avatars_;

        std::unique_ptr<DetachedVideoWindow> detached_wnd_;
        video_window::ResizeEventFilter*     event_filter_;

        QTimer check_overlapped_timer_;
		QTimer show_panel_timer_;

        bool have_remote_video_;
        bool _outgoingNotAccepted;

		UIEffects* video_panel_effect_;
		//UIEffects* video_panel_header_effect_;
        //UIEffects* video_panel_header_effect_with_avatars_;

        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QWidget *widget_;

        SecureCallWnd* secureCallWnd_;

        struct {
            std::string name;
            unsigned time;
        } callDescription;
    };
}

#endif//__VIDEO_WINDOW_H__