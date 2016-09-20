#ifndef __DETACHED_VIDEO_WND_H__
#define __DETACHED_VIDEO_WND_H__

#include "VideoFrame.h"
#include "CommonUI.h"

namespace voip_manager
{
    struct ContactEx;
    struct FrameSize;
}

namespace Ui
{
    class ResizeEventFilter;
    class ShadowWindowParent;
    class VideoPanelHeader;

    class DetachedVideoWindow : public AspectRatioResizebleWnd
    {
        Q_OBJECT

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;

    private Q_SLOTS:
        void checkPanelsVis();
        void onPanelMouseEnter();
        void onPanelMouseLeave();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);

    public:
        DetachedVideoWindow(QWidget* _parent);
        ~DetachedVideoWindow();

        quintptr getVideoFrameId() const;
        bool closedManualy();

        void showFrame();
        void hideFrame();

    private:
        std::unique_ptr<VideoPanelHeader> videoPanelHeader_;
        UIEffects* videoPanelHeaderEffect_;
        QTimer showPanelTimer_;
        ResizeEventFilter* eventFilter_;
        QWidget* parent_;
        QPoint posDragBegin_;
        bool closedManualy_;
        QHBoxLayout *horizontalLayout_;

		std::unique_ptr<ShadowWindowParent> shadow_;
        platform_specific::GraphicsPanel* rootWidget_;
        
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
        quintptr getContentWinId() override;
    };
}

#endif//__DETACHED_VIDEO_WND_H__