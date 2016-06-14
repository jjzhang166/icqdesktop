#include "stdafx.h"
#include "VideoPanelHeader.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "VoipTools.h"
#include "../controls/TextEmojiWidget.h"
#include "NameAndStatusWidget.h"
#include "PushButton_t.h"

#define SCALED_VALUE(x) (Utils::scale_value((x)))
#define DEFAULT_BORDER SCALED_VALUE(12)
#define DEFAULT_WINDOW_ROUND_RECT SCALED_VALUE(5)
#define DEFAULT_TIME_BUTTONS_OFFSET SCALED_VALUE(300)

#define COLOR_SECURE_BTN_ACTIVE   QColor(0, 0, 0, 255)
#define COLOR_SECURE_BTN_INACTIVE QColor(0, 0, 0, 0)

#ifdef __APPLE__
    const QString closeButtonStyle =
        "QPushButton { border-image: url(:/resources/video_panel/videoctrl_close_100.png); width: 32dip; height: 32dip; background-color: transparent; padding: 0; margin: 0; border: none; } "
        "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_close_100_hover.png); background-color: #e81123; } "
        "QPushButton:pressed { border-image: url(:/resources/video_panel/videoctrl_close_100_active.png); background-color: #d00516; }";

    const QString minButtonStyle =
        "QPushButton { border-image: url(:/resources/video_panel/videoctrl_minimize_100.png); width: 32dip; height: 32dip; background-color: transparent; padding: 0; margin: 0; border: none; } "
        "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_minimize_100_hover.png); background-color: #d3d3d3; } "
        "QPushButton:pressed { border-image: url(:/resources/video_panel/videoctrl_minimize_100_active.png); background-color: #9b9b9b; }";

    const QString maxButtonStyle =
        "QPushButton { border-image: url(:/resources/video_panel/videoctrl_bigwindow_100.png); width: 32dip; height: 32dip; background-color: transparent; padding: 0; margin: 0; border: none; } "
        "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_bigwindow_100_hover.png); background-color: #d3d3d3; } "
        "QPushButton:pressed { border-image: url(:/resources/video_panel/videoctrl_bigwindow_100_active.png); background-color: #9b9b9b; }";

    const QString maxButtonStyle2 =
        "QPushButton { border-image: url(:/resources/main_window/contr_smallwindow_100.png); width: 32dip; height: 32dip; background-color: transparent; padding: 0; margin: 0; border: none; } "
        "QPushButton:hover { border-image: url(:/resources/main_window/contr_smallwindow_100.png); background-color: #d3d3d3; } "
        "QPushButton:pressed { border-image: url(:/resources/main_window/contr_smallwindow_100_active.png); background-color: #9b9b9b; }";
#else
	const QString closeButtonStyle =
		"QPushButton { background-image: url(:/resources/video_panel/videoctrl_close_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; background-color: transparent; padding-top: 2dip; padding-bottom: 2dip; width: 24dip; height: 24dip; padding-left: 11dip; padding-right: 12dip; border: none; }"
        "QPushButton:hover { background-image: url(:/resources/video_panel/videoctrl_close_100_hover.png); background-color: #e81123; }"
        "QPushButton:pressed { background-image: url(:/resources/video_panel/videoctrl_close_100_active.png); background-color: #d00516; } ";

	const QString minButtonStyle =
		"QPushButton { background-image: url(:/resources/video_panel/videoctrl_minimize_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; background-color: transparent; padding-top: 2dip; padding-bottom: 2dip; width: 24dip; height: 24dip; padding-left: 11dip; padding-right: 12dip; border: none; }"
        "QPushButton:hover { background-image: url(:/resources/video_panel/videoctrl_minimize_100_hover.png); background-color: #d3d3d3; }"
        "QPushButton:pressed { background-image: url(:/resources/video_panel/videoctrl_minimize_100_active.png); background-color: #9b9b9b; } ";

	const QString maxButtonStyle =
		"QPushButton { background-image: url(:/resources/video_panel/videoctrl_bigwindow_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; background-color: transparent; padding-top: 2dip; padding-bottom: 2dip; width: 24dip; height: 24dip; padding-left: 11dip; padding-right: 12dip; border: none; }"
        "QPushButton:hover { background-image: url(:/resources/video_panel/videoctrl_bigwindow_100_hover.png); background-color: #d3d3d3; }"
        "QPushButton:pressed { background-image: url(:/resources/video_panel/videoctrl_bigwindow_100_active.png); background-color: #9b9b9b; } ";

    const QString maxButtonStyle2 =
        "QPushButton { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; padding-top: 2dip; padding-bottom: 2dip; padding-left: 11dip; padding-right: 11dip; border: none; }"
        "QPushButton:hover { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100.png); background-color: #d3d3d3; }"
        "QPushButton:hover:pressed { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100_active.png); background-color: #c8c8c8; }";
#endif

const QString secureCallButton = 
" \
QPushButton \
{ \
    color: #ffffff; \
    font-size: 15dip; \
    text-align: center; \
    border-style: none; \
    background-color: transparent; \
} \
";
    //margin-left:  24dip; \
    //margin-right: 24dip; \

const QString secureCallButtonClicked = 
" \
QPushButton \
{ \
    color: #ffffff; \
    font-size: 15dip; \
    text-align: center; \
    border-style: solid; \
    border-color: #000000; \
    background-color: #000000; \
} \
";
    //border-right-width: 24dip;\
    //border-left-width: 24dip;\

    //margin-left:  24dip; \
    //margin-right: 24dip; \

#define SECURE_BTN_BORDER_W    Utils::scale_value(24)
#define SECURE_BTN_ICON_W      Utils::scale_value(16)
#define SECURE_BTN_ICON_H      SECURE_BTN_ICON_W
#define SECURE_BTN_TEXT_W      Utils::scale_value(50)
#define SECURE_BTN_ICON2TEXT_W Utils::scale_value(12)
#define SECURE_BTN_W           (2 * SECURE_BTN_BORDER_W + SECURE_BTN_ICON_W + SECURE_BTN_TEXT_W + SECURE_BTN_ICON2TEXT_W)


std::string Ui::getFotmatedTime(unsigned ts) {
    int h = ts / (60 * 60);
    int m = (ts / 60) % 60;
    int s = ts % 60;

    std::stringstream ss;
    if (h > 0) {
        ss << std::setfill('0') << std::setw(2) << h << ":";
    }
    ss << std::setfill('0') << std::setw(2) << m << ":";
    ss << std::setfill('0') << std::setw(2) << s;

    return ss.str();
}

Ui::MoveablePanel::MoveablePanel(QWidget* parent)
: QWidget(NULL)
, _parent(parent) {
	_drag_state.is_draging = false;

    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

Ui::MoveablePanel::~MoveablePanel() {
}

void Ui::MoveablePanel::mouseReleaseEvent(QMouseEvent* /*e*/) {
	_drag_state.is_draging = false;
}

void Ui::MoveablePanel::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
#ifdef __APPLE__
    const auto rc = _parent->rect();
    if (_parent && !_parent->isFullScreen()) {
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), DEFAULT_WINDOW_ROUND_RECT, DEFAULT_WINDOW_ROUND_RECT);
        setMask(path.toFillPolygon().toPolygon());
    } else {
        clearMask();
    }
#endif
}

void Ui::MoveablePanel::mousePressEvent(QMouseEvent* e) {
	if (_parent && !_parent->isFullScreen()) {
		_drag_state.is_draging = true;
		_drag_state.pos_drag_begin = e->pos();
	}
}

void Ui::MoveablePanel::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && _drag_state.is_draging) {
        QPoint diff = e->pos() - _drag_state.pos_drag_begin;
        QPoint newpos = _parent->pos() + diff;

        _parent->move(newpos);
    }
}

void Ui::MoveablePanel::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);

    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow() || uiWidgetIsActive()) {
            if (_parent) {
                _parent->raise();
                raise();
            }
        }
    }
}

void Ui::MoveablePanel::keyReleaseEvent(QKeyEvent* e) {
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        emit onkeyEscPressed();
    }
}

bool Ui::VideoPanelHeader::uiWidgetIsActive() const {
    return _lowWidget->isActiveWindow();
}

void Ui::VideoPanelHeader::_onMinimize() {
    emit onMinimize();
}

void Ui::VideoPanelHeader::_onMaximize() {
    emit onMaximize();
}

void Ui::VideoPanelHeader::_onClose() {
    emit onClose();
}

Ui::VideoPanelHeader::VideoPanelHeader(QWidget* parent, int items)
: MoveablePanel(parent)
, _callName(NULL)
, _lowWidget(NULL)
, _callTime(NULL)
, _btnMin(NULL)
, _secureCallEnabled(false)
, _btnMax(NULL)
, _btnClose(NULL)
, _callNameSpacer(NULL)
, _callTimeSpacer(NULL)
, _items_to_show(items) {
    setProperty("VideoPanelHeader", true);
    setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* layout = new QHBoxLayout();
    _lowWidget = new QWidget(this);
    { // low widget. it makes background panel coloured
        _lowWidget->setContentsMargins(DEFAULT_BORDER, 0, 0, 0);
        _lowWidget->setProperty("VideoPanelHeader", true);
        _lowWidget->setLayout(layout);

        QVBoxLayout* vLayoutParent = new QVBoxLayout();
        vLayoutParent->setContentsMargins(0, 0, 0, 0);
        vLayoutParent->setSpacing(0);
        vLayoutParent->setAlignment(Qt::AlignVCenter);
        vLayoutParent->addWidget(_lowWidget);
        setLayout(vLayoutParent);

        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignVCenter);

        //layout->addSpacing(DEFAULT_BORDER);
    }

    auto __addWidget = [] (QWidget* parent)
    {
        QWidget* w = new QWidget(parent);
        w->setContentsMargins(0, 0, 0, 0);
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (parent) {
            parent->layout()->addWidget(w);
        }
        return w;
    };

    auto __addLayout = [] (QWidget* w, Qt::Alignment align)
    {
        assert(w);
        if (w) {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(align);
            w->setLayout(layout);
        }
    };

    QWidget* leftWidg    = __addWidget(_lowWidget);
    QWidget* centerWidg  = __addWidget(_lowWidget);
    QWidget* rightWidg   = __addWidget(_lowWidget);

    __addLayout(leftWidg,   Qt::AlignLeft | Qt::AlignVCenter);
    __addLayout(centerWidg, Qt::AlignCenter);
    __addLayout(rightWidg,  Qt::AlignRight | Qt::AlignVCenter);

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    if (_items_to_show & kVPH_ShowName) {
        _callName = new NameWidget(leftWidg, Utils::scale_value(15));
        _callName->setFont(font);
        _callName->layout()->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _callName->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        _callName->setNameProperty("VideoPanelHeaderText", true);

        leftWidg->layout()->addWidget(_callName);
        //leftWidg->layout()->addSpacing(DEFAULT_BORDER << 1);
    }

    if (_items_to_show & kVPH_ShowTime) {
        _callTime = new voipTools::BoundBox<PushButton_t>(centerWidg);
        _callTime->setPostfixColor(QColor(255, 255, 255, 255));
        _callTime->setFont(font);
        _callTime->setEnabled(false);
        _callTime->setAlignment(Qt::AlignCenter);
        _callTime->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        _callTime->setFixedWidth(SECURE_BTN_TEXT_W);
        _callTime->setIconSize(SECURE_BTN_ICON_W, SECURE_BTN_ICON_H);

        QObject::connect(_callTime, SIGNAL(clicked()), this, SLOT(_onSecureCallClicked()), Qt::QueuedConnection);
        Utils::ApplyStyle(_callTime, secureCallButton);
        centerWidg->layout()->addWidget(_callTime);
        //layout->addSpacing(DEFAULT_BORDER * 3);
    }

    QWidget* parentWidget = rightWidg;
    auto __addButton = [this, parentWidget] (const QString& propertyName, const char* slot)->QPushButton* {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);

        Utils::ApplyStyle(btn, propertyName);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        parentWidget->layout()->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, slot, Qt::QueuedConnection);
        return btn;
    };

    bool spacerBetweenBtnsAndTimeAdded = false;
    if (_items_to_show & kVPH_ShowMin) {
        //if (!spacerBetweenBtnsAndTimeAdded) {
        //    _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
        //    layout->addSpacerItem(_callTimeSpacer);
        //    spacerBetweenBtnsAndTimeAdded = true;
        //}
        _btnMin = __addButton(minButtonStyle, SLOT(_onMinimize()));
    }

    if (_items_to_show & kVPH_ShowMax) {
        //if (!spacerBetweenBtnsAndTimeAdded) {
        //    _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
        //    layout->addSpacerItem(_callTimeSpacer);
        //    spacerBetweenBtnsAndTimeAdded = true;
        //}
        _btnMax = __addButton(maxButtonStyle, SLOT(_onMaximize()));
    }

    if (_items_to_show & kVPH_ShowClose) {
        //if (!spacerBetweenBtnsAndTimeAdded) {
        //    _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
        //    layout->addSpacerItem(_callTimeSpacer);
        //    spacerBetweenBtnsAndTimeAdded = true;
        //}
        _btnClose = __addButton(closeButtonStyle, SLOT(_onClose()));
    }

    //if (!spacerBetweenBtnsAndTimeAdded) {
    //    _callTimeSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
    //    layout->addSpacerItem(_callTimeSpacer);
    //    spacerBetweenBtnsAndTimeAdded = true;
    //}
}

Ui::VideoPanelHeader::~VideoPanelHeader() {
    
}

void Ui::VideoPanelHeader::enterEvent(QEvent* e) {
	QWidget::enterEvent(e);
	emit onMouseEnter();
}

void Ui::VideoPanelHeader::leaveEvent(QEvent* e) {
	QWidget::leaveEvent(e);
	emit onMouseLeave();
}

void Ui::VideoPanelHeader::enableSecureCall(bool enable)
{
    _secureCallEnabled = enable;
    if (_callTime) {
        _callTime->setEnabled(enable);
        _callTime->setOffsets(enable ? Utils::scale_value(12) : 0);
        _callTime->setImageForState(PushButton_t::normal, enable ? ":/resources/video_panel/content_securecall_video_100.png" : "");
        _callTime->setFixedWidth(enable ? SECURE_BTN_W : SECURE_BTN_TEXT_W);

        _callTime->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        _callTime->setColorForState(PushButton_t::hovered, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        _callTime->setColorForState(PushButton_t::pressed, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

        _callTime->setCursor(enable ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
    }
}

void Ui::VideoPanelHeader::_onSecureCallClicked()
{
    emit onSecureCallClicked(geometry());
}

void Ui::VideoPanelHeader::setCallName(const std::string& name) {
    if (!!_callName) {
        _callName->setName(name.c_str());
    }
}

void Ui::VideoPanelHeader::setFullscreenMode(bool en) {
    if (_btnMin) {
        if (en) {
            _btnMin->hide();
        } else {
            _btnMin->show();
        }
    }
}

void Ui::VideoPanelHeader::setSecureWndOpened(const bool opened)
{
    assert(_callTime);
    if (_callTime) {
        if (opened) {
            Utils::ApplyStyle(_callTime, secureCallButtonClicked);

            _callTime->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_ACTIVE);
            _callTime->setColorForState(PushButton_t::hovered, COLOR_SECURE_BTN_ACTIVE);
            _callTime->setColorForState(PushButton_t::pressed, COLOR_SECURE_BTN_ACTIVE);
        } else {
            Utils::ApplyStyle(_callTime, secureCallButton);

            _callTime->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
            _callTime->setColorForState(PushButton_t::hovered, _secureCallEnabled ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
            _callTime->setColorForState(PushButton_t::pressed, _secureCallEnabled ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

            _callTime->setCursor(_secureCallEnabled ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
        }
    }
}

void Ui::VideoPanelHeader::setTime(unsigned ts, bool have_call) {
    if (!have_call) {
        if (!!_callTime) {
            _callTime->setText(QString(), "");
        }
	} else {
        if (!!_callTime) {
            _callTime->setText(QString(), getFotmatedTime(ts).c_str());
        }
	}
}
