#ifndef __DETACHED_VIDEO_WND_H__
#define __DETACHED_VIDEO_WND_H__
#include "VideoFrame.h"

namespace voip_manager{
    struct ContactEx;
    struct FrameSize;
}

namespace Ui {
	struct UIEffects {
		UIEffects(QWidget& obj);
		virtual ~UIEffects();

		void fadeOut(unsigned interval);
		void fadeIn(unsigned interval);

        void geometryTo(const QRect& rc, unsigned interval);

	private:
		bool _fadedIn;

		QGraphicsOpacityEffect* _fade_effect;
		QPropertyAnimation* _animation;
        QPropertyAnimation* _resize_animation;
        
        QWidget& _obj;
	};

    class AspectRatioResizebleWnd : public QWidget { Q_OBJECT
    public:
        AspectRatioResizebleWnd();
        virtual ~AspectRatioResizebleWnd();

        bool isInFullscreen() const;
        void switchFullscreen();

        void useAspect();
        void unuseAspect();

    protected:
        bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
        virtual quintptr getContentWinId() = 0;
        virtual void escPressed() { }

    private Q_SLOTS:
        void onVoipFrameSizeChanged(const voip_manager::FrameSize& fs);
        void onVoipCallCreated(const voip_manager::ContactEx& contact_ex);

    private:
        float _aspect_ratio;
        bool _use_aspect;

        UIEffects* _self_resize_effect;
        bool _first_time_use_aspect_ratio;

        void _applyFrameAspectRatio(float was_ar);
        void keyReleaseEvent(QKeyEvent*) override;
#ifdef _WIN32
        bool _onWMSizing(RECT& rc, unsigned wParam);
#endif
    };

	namespace video_window {
		class ResizeEventFilter;
	}
	
	class VideoPanelHeader;
    class DetachedVideoWindow : public AspectRatioResizebleWnd {
        Q_OBJECT

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
		void changeEvent(QEvent* e) override;
		void enterEvent(QEvent* e) override;
		void leaveEvent(QEvent* e) override;

    private Q_SLOTS:
		void _check_panels_vis();
		void onPanelMouseEnter();
		void onPanelMouseLeave();

		void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);
        void onVoipWindowRemoveComplete(quintptr win_id);
        void onVoipWindowAddComplete(quintptr win_id);

    public:
        DetachedVideoWindow(QWidget* parent);
        ~DetachedVideoWindow();

        quintptr get_video_frame_id() const;
		bool closedManualy();

        void showFrame();
        void hideFrame();

    private:
		std::unique_ptr<VideoPanelHeader> video_panel_header_;
		UIEffects* video_panel_header_effect_;
		QTimer show_panel_timer_;
		video_window::ResizeEventFilter* event_filter_;
        QWidget* parent_;
		QPoint pos_drag_begin_;
		bool closed_manualy_;
        QHBoxLayout *horizontal_layout_;

        platform_specific::GraphicsPanel* _rootWidget;
        
		void mousePressEvent(QMouseEvent* e) override;
		void mouseMoveEvent(QMouseEvent* e) override;
		void mouseDoubleClickEvent(QMouseEvent* e) override;
        quintptr getContentWinId() override;
    };
}

#endif//__DETACHED_VIDEO_WND_H__