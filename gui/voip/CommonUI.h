
#pragma once

#include "../main_window/MainWindow.h"

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
	class ResizeEventFilter : public QObject
	{
		Q_OBJECT

	public:
		ResizeEventFilter(std::vector<QWidget*>& _topPanels, std::vector<QWidget*>& _bottomPanels, ShadowWindow* shadow, QObject* _parent);

	protected:
		bool eventFilter(QObject* _obj, QEvent* _event);

	private:
		std::vector<QWidget*> topPanels_;
		std::vector<QWidget*> bottomPanels_;
		ShadowWindow* shadow_;
	};

	class ShadowWindowParent
	{
	public:
		ShadowWindowParent(QWidget* parent);

		void showShadow();
		void hideShadow();
		ShadowWindow* getShadowWidget();
		void setActive(bool _value);

	protected:
		// shadow
		ShadowWindow* shadow_;
	};

	struct UIEffects
	{
		UIEffects(QWidget& _obj);
		virtual ~UIEffects();

		void fadeOut(unsigned _interval);
		void fadeIn(unsigned _interval);

		void geometryTo(const QRect& _rc, unsigned _interval);

	private:
		bool fadedIn_;

		QGraphicsOpacityEffect* fadeEffect_;
		QPropertyAnimation* animation_;
		QPropertyAnimation* resizeAnimation_;

		QWidget& obj_;
	};

	class AspectRatioResizebleWnd : public QWidget
	{
		Q_OBJECT
	public:
		AspectRatioResizebleWnd();
		virtual ~AspectRatioResizebleWnd();

		bool isInFullscreen() const;
		void switchFullscreen();

		void useAspect();
		void unuseAspect();

	protected:
		bool nativeEvent(const QByteArray& _eventType, void* _message, long* _result) override;
		virtual quintptr getContentWinId() = 0;
		virtual void escPressed() { }

		private Q_SLOTS:
		void onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs);
		void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);

	private:
		float aspectRatio_;
		bool useAspect_;

		UIEffects* selfResizeEffect_;
		bool firstTimeUseAspectRatio_;

		void applyFrameAspectRatio(float _wasAr);
		void keyReleaseEvent(QKeyEvent*) override;
#ifdef _WIN32
		bool onWMSizing(RECT& _rc, unsigned _wParam);
#endif
	};


}
