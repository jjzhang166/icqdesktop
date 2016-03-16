#include "stdafx.h"
#include "VoipSysPanelHeader.h"
#include "../utils/utils.h"
#ifdef __APPLE__
#   include <QtWidgets/qtoolbutton.h>
#else
#   include <QToolButton>
#   include <QStyleOptionToolButton>
#endif // __APPLE__

#define VOIP_VIDEO_PANEL_BTN_OFFSET (Utils::scale_value(40))
#include "../../installer/utils/styles.h"
#include "VoipTools.h"

Ui::VoipSysPanelControl::VoipSysPanelControl(QWidget* parent)
: QWidget(NULL)
, _rootWidget(NULL)
, _parent(parent) {
    setProperty("VoipSysPanelControls", true);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);  

    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignVCenter);
    setLayout(root_layout);

    _rootWidget = new QWidget(this);
    _rootWidget->setContentsMargins(0, 0, 0, 0);
    _rootWidget->setProperty("VoipSysPanelControls", true);
    layout()->addWidget(_rootWidget);
    
    QHBoxLayout* layoutTarget = new QHBoxLayout();
    layoutTarget->setContentsMargins(0, 0, 0, 0);
    layoutTarget->setSpacing(0);
    layoutTarget->setAlignment(Qt::AlignVCenter);
    _rootWidget->setLayout(layoutTarget);

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);

    QWidget* rootWidget = _rootWidget;
    const int btnMinWidth = Utils::scale_value(40);

    auto __getWidget = [rootWidget] (const bool vertical) {
        QBoxLayout* layoutInWidget;
        if (vertical) {
            layoutInWidget = new QVBoxLayout();
        } else {
            layoutInWidget = new QHBoxLayout();
        }

        layoutInWidget->setContentsMargins(0, 0, 0, 0);
        layoutInWidget->setSpacing(0);
        layoutInWidget->setAlignment(Qt::AlignCenter);

        QWidget* widget = new voipTools::BoundBox<QWidget>(rootWidget);
        widget->setContentsMargins(0, 0, 0, 0);
        widget->setLayout(layoutInWidget);

        return widget;
    };

    auto __getButton = [this, &font, btnMinWidth] (QWidget* parent, const char* propertyName, const char* /*text*/, const char* slot, QPushButton**btnOut)->int {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parent);
        btn->setProperty(propertyName, true);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        connect(btn, SIGNAL(clicked()), this, slot, Qt::QueuedConnection);
        *btnOut = btn;
        
        btn->resize(btn->sizeHint().width(), btn->sizeHint().height());
        return (btn->width() - btnMinWidth) / 2;
    };

    auto __getLabel = [this, &font, btnMinWidth] (QWidget* parent, const char* /*propertyName*/, const char* text, QLabel**labelOut)->int {
        QLabel* label = new voipTools::BoundBox<QLabel>(parent);
        label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        label->setText(text);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);
        *labelOut = label;

        Utils::ApplyStyle(label, "QLabel { color: #ffffff; font-size: 12dip; }");
        label->resize(label->sizeHint().width(), label->sizeHint().height());
        return (label->width() - btnMinWidth) * 0.5f;
    };

    QWidget* widget1 = __getWidget(true);
    QWidget* widget2 = __getWidget(true);
    QWidget* widget3 = __getWidget(true);

    int tail1 = 0;
    int tail2 = 0;
    int tail3 = 0;

    const struct ButtonDesc {
        QWidget*    parent;
        std::string propertyName;
        std::string text;
        std::string slot;
        int&        tail;
    } buttonsToCreate [] = { 
        { widget1, "VoipSysPanelControls_hangup", QT_TRANSLATE_NOOP("voip_pages","End call").toUtf8().data(), SLOT(_onDecline()), tail1 },
        { widget2, "VoipSysPanelControls_answer", QT_TRANSLATE_NOOP("voip_pages", "Answer").toUtf8().data(),  SLOT(_onAudio()),   tail2 },
        { widget3, "VoipSysPanelControls_video",  QT_TRANSLATE_NOOP("voip_pages", "Video").toUtf8().data(),   SLOT(_onVideo()),   tail3 }
    };

    for (unsigned ix = 0; ix < sizeof buttonsToCreate / sizeof buttonsToCreate[0]; ++ix) {
        const ButtonDesc& buttonDesc = buttonsToCreate[ix];

        QPushButton* btn = NULL;
        const int btnTail = __getButton(buttonDesc.parent, buttonDesc.propertyName.c_str(), buttonDesc.text.c_str(), buttonDesc.slot.c_str(), &btn);

        QLabel* label = NULL;
        const int labelTail = __getLabel(buttonDesc.parent, buttonDesc.propertyName.c_str(), buttonDesc.text.c_str(), &label);

        assert(btn && label);
        if (!btn || !label) {
            continue;
        }
        buttonDesc.tail = std::max(btnTail, labelTail);

        QWidget* tmpWidget = __getWidget(false);
        tmpWidget->layout()->addWidget(btn);
        buttonDesc.parent->layout()->addWidget(tmpWidget);
        buttonDesc.parent->layout()->addWidget(label);
    }

    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    layoutTarget->addWidget(widget1);
    layoutTarget->addSpacing(VOIP_VIDEO_PANEL_BTN_OFFSET - tail1 - tail2);
    layoutTarget->addWidget(widget2);
    layoutTarget->addSpacing(VOIP_VIDEO_PANEL_BTN_OFFSET - tail2 - tail3);
    layoutTarget->addWidget(widget3);
    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
}

Ui::VoipSysPanelControl::~VoipSysPanelControl() {
}

void Ui::VoipSysPanelControl::_onDecline() {
    emit onDecline();
}

void Ui::VoipSysPanelControl::_onAudio() {
    emit onAudio();
}

void Ui::VoipSysPanelControl::_onVideo() {
    emit onVideo();
}

void Ui::VoipSysPanelControl::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow() || _rootWidget->isActiveWindow()) {
            if (_parent) {
                //_parent->blockSignals(true);
                _parent->raise();
                raise();
                //_parent->blockSignals(false);
            }
        }
    }
}

Ui::VoipSysPanelHeader::VoipSysPanelHeader(QWidget* parent)
: MoveablePanel(parent)
, _nameAndStatusContainer(NULL)
, _rootWidget(NULL)
, _avatarContainer(NULL) {
    setProperty("VoipSysPanelHeader", true);
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    //setAttribute(Qt::WA_NoSystemBackground, true);
    //setAttribute(Qt::WA_TranslucentBackground, true);  

    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignVCenter);
    setLayout(root_layout);

    _rootWidget = new QWidget(this);
    _rootWidget->setContentsMargins(0, 0, 0, 0);
    _rootWidget->setProperty("VoipSysPanelHeader", true);
    layout()->addWidget(_rootWidget);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignVCenter);
    _rootWidget->setLayout(layout);

    _avatarContainer = new voipTools::BoundBox<AvatarContainerWidget>(_rootWidget, Utils::scale_value(66), Utils::scale_value(12), Utils::scale_value(12), Utils::scale_value(5));
    _avatarContainer->setOverlap(0.2f);
    _rootWidget->layout()->addWidget(_avatarContainer);

    _nameAndStatusContainer = new NameAndStatusWidget(_rootWidget, Utils::scale_value(23), Utils::scale_value(15));
    _nameAndStatusContainer->setNameProperty("VoipPanelHeaderText_Name", true);
    _nameAndStatusContainer->setStatusProperty("VoipPanelHeaderText_Status", true);
    _nameAndStatusContainer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    _rootWidget->layout()->addWidget(_nameAndStatusContainer);

    layout->addSpacing(Utils::scale_value(12));
}

Ui::VoipSysPanelHeader::~VoipSysPanelHeader() {
    
}

void Ui::VoipSysPanelHeader::enterEvent(QEvent* e) {
	QWidget::enterEvent(e);
	emit onMouseEnter();
}

void Ui::VoipSysPanelHeader::leaveEvent(QEvent* e) {
	QWidget::leaveEvent(e);
	emit onMouseLeave();
}

void Ui::VoipSysPanelHeader::setAvatars(const std::vector<std::string> avatarList) {
    _avatarContainer->dropExcess(avatarList);
}

void Ui::VoipSysPanelHeader::setTitle(const char* s) {
    if (_nameAndStatusContainer) {
        _nameAndStatusContainer->setName(s);
    }
}

void Ui::VoipSysPanelHeader::setStatus(const char* s) {
    if (_nameAndStatusContainer) {
        _nameAndStatusContainer->setStatus(s);
    }
}

bool Ui::VoipSysPanelHeader::uiWidgetIsActive() const {
    if (_rootWidget) {
        return _rootWidget->isActiveWindow();
    }
    return false;
}