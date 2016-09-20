#ifndef __VIDEO_PANEL_HEADER_H__
#define __VIDEO_PANEL_HEADER_H__

#include "NameAndStatusWidget.h"
#include "WindowHeaderFormat.h"

namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Ui
{
    std::string getFotmatedTime(unsigned _ts);

    class PushButton_t;

    class MoveablePanel : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void onkeyEscPressed();

    private Q_SLOTS:

    public:
        MoveablePanel(QWidget* _parent);
        virtual ~MoveablePanel();

    private:
        QWidget* parent_;
        struct
        {
            QPoint posDragBegin;
            bool isDraging;
        } dragState_;

        void changeEvent(QEvent*) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void keyReleaseEvent(QKeyEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    protected:
        virtual bool uiWidgetIsActive() const = 0;
    };

    //class videoPanelHeader;
    class VideoPanelHeader : public MoveablePanel
    {
    Q_OBJECT

    Q_SIGNALS:
        void onMinimize();
        void onMaximize();
        void onClose();
        void onMouseEnter();
        void onMouseLeave();
        void onSecureCallClicked(const QRect& _rc);

    private Q_SLOTS:
        void _onMinimize();
        void _onMaximize();
        void _onClose();

        void _onSecureCallClicked();

    public:
        VideoPanelHeader(QWidget* _parent, int _items = kVPH_ShowAll);
        virtual ~VideoPanelHeader();

        void setCallName(const std::string&);
        void setTime(unsigned _ts, bool _hasCall);
        void setFullscreenMode(bool en);

        void setSecureWndOpened(const bool _opened);
        void enableSecureCall(bool _enable);

    private:
        //std::unique_ptr<videoPanelHeader> _ui;
        int itemsToShow_;

        NameWidget*  callName_;

        PushButton_t* callTime_;

        QPushButton* btnMin_;
        QPushButton* btnMax_;
        QPushButton* btnClose_;

        QWidget*     lowWidget_;
        bool         secureCallEnabled_;

        QPushButton* _backToVideoCode;

        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        bool uiWidgetIsActive() const override;
    };

}

#endif//__VIDEO_PANEL_HEADER_H__