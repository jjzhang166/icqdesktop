#ifndef __VIDEO_PANEL_HEADER_H__
#define __VIDEO_PANEL_HEADER_H__

#include "NameAndStatusWidget.h"

namespace voip_manager {
    struct ContactEx;
    struct Contact;
}

namespace Ui {
    std::string getFotmatedTime(unsigned ts);

	enum eVideoPanelHeaderItems {
		kVPH_ShowNone = 0x0000,
		kVPH_ShowName = 0x0001,
		kVPH_ShowTime = 0x0002,

		kVPH_ShowMin = 0x0004,
		kVPH_ShowMax = 0x0008,
		kVPH_ShowClose = 0x0010,

		kVPH_ShowAll  = 0xffff
	};

    class MoveablePanel : public QWidget { Q_OBJECT
    Q_SIGNALS:
        void onkeyEscPressed();

    private Q_SLOTS:

    public:
        MoveablePanel(QWidget *parent);
        virtual ~MoveablePanel();

    private:
        QWidget* _parent;
		struct {
			QPoint pos_drag_begin;
			bool is_draging;
		} _drag_state;

        void changeEvent(QEvent*) override;
		void mousePressEvent(QMouseEvent* e) override;
		void mouseMoveEvent(QMouseEvent* e) override;
		void mouseReleaseEvent(QMouseEvent* e) override;
        void keyReleaseEvent(QKeyEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    protected:
        virtual bool uiWidgetIsActive() const = 0;
    };

    //class videoPanelHeader;
    class VideoPanelHeader : public MoveablePanel {
    Q_OBJECT

    Q_SIGNALS:
        void onMinimize();
        void onMaximize();
        void onClose();
		void onMouseEnter();
		void onMouseLeave();

    private Q_SLOTS:
        void _onMinimize();
        void _onMaximize();
        void _onClose();

    public:
        VideoPanelHeader(QWidget* parent, int items = kVPH_ShowAll);
        virtual ~VideoPanelHeader();

        void setCallName(const std::string&);
		void setTime(unsigned ts, bool have_call);
        void setFullscreenMode(bool en);

    private:
        //std::unique_ptr<videoPanelHeader> _ui;
		int _items_to_show;

        NameWidget*  _callName;
        QSpacerItem* _callNameSpacer;

        QLabel*      _callTime;
        QSpacerItem* _callTimeSpacer;

        QPushButton* _btnMin;
        QPushButton* _btnMax;
        QPushButton* _btnClose;

        QWidget*     _lowWidget;

		void enterEvent(QEvent* e) override;
		void leaveEvent(QEvent* e) override;
//		void resizeEvent(QResizeEvent* e) override;
        bool uiWidgetIsActive() const override;
    };

}

#endif//__VIDEO_PANEL_HEADER_H__