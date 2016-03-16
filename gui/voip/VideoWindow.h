#ifndef __VIDEO_WINDOW_H__
#define __VIDEO_WINDOW_H__

#include "VideoPanel.h"
#include "VideoPanelHeader.h"
#include "DetachedVideoWnd.h"
#include "VoipSysPanelHeader.h"
#include "VideoFrame.h"

namespace voip_manager{
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
        void updatePanels_();
        void _switchFullscreen();
        void _checkPanelsVisibility(bool forceHide = false);
        inline void updateWindowTitle_();

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
        void _escPressed();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

		void onPanelMouseEnter();
		void onPanelMouseLeave();
		void onPanelFullscreenClicked();

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();

    private:
        FrameControl_t* _rootWidget;

        std::unique_ptr<VideoPanel>          video_panel_;
        std::unique_ptr<VideoPanelHeader>    video_panel_header_;
        std::unique_ptr<VoipSysPanelHeader>  video_panel_header_with_avatars_;

        std::unique_ptr<DetachedVideoWindow> detached_wnd_;
        video_window::ResizeEventFilter*     event_filter_;

        QTimer check_overlapped_timer_;
		QTimer show_panel_timer_;
        bool have_remote_video_;

		UIEffects* video_panel_effect_;
		UIEffects* video_panel_header_effect_;
        UIEffects* video_panel_header_effect_with_avatars_;

        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QWidget *widget_;

        struct {
            std::string name;
            unsigned time;
        } callDescription;
    };
}

#endif//__VIDEO_WINDOW_H__