#include "stdafx.h"
#include "IncomingCallWindow.h"
#include "../core_dispatcher.h"
#include "DetachedVideoWnd.h"
#include "VideoWindow.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "CallPanelMain.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "VoipTools.h"

namespace {
    enum {
        kIncomingCallWndDefH = 285,
        kIncomingCallWndDefW = 390,
    };

#ifdef _DEBUG
    const int ht[kIncomingCallWndDefH > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefH must be not null
    const int wt[kIncomingCallWndDefW > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefW must be not null
#endif
}

Ui::IncomingCallWindow::IncomingCallWindow(const std::string& account, const std::string& contact)
: QWidget(NULL) 
, contact_(contact)
, account_(account)
, header_(new(std::nothrow) voipTools::BoundBox<VoipSysPanelHeader>(this))
, controls_(new voipTools::BoundBox<VoipSysPanelControl>(this)) {
    QIcon icon(":/resources/main_window/appicon.ico");
    setWindowIcon(icon);
    setProperty("IncomingCallWindow", true);
    
#ifdef _WIN32
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::SubWindow);
#else
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window | Qt::WindowDoesNotAcceptFocus/*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif
    
    setAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignVCenter);
    setLayout(root_layout);
    
    header_->setWindowFlags(header_->windowFlags() | Qt::WindowStaysOnTopHint);
    controls_->setWindowFlags(controls_->windowFlags() | Qt::WindowStaysOnTopHint);

    std::vector<QWidget*> panels;
    panels.push_back(header_.get());
    panels.push_back(controls_.get());
#ifndef STRIP_VOIP
    _rootWidget = platform_specific::GraphicsPanel::create(this, panels);
    _rootWidget->setContentsMargins(0, 0, 0, 0);
    _rootWidget->setAttribute(Qt::WA_UpdatesDisabled);
    _rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(_rootWidget);
#endif //__linux__

    std::vector<QWidget*> topPanels;
    topPanels.push_back(header_.get());

    std::vector<QWidget*> bottomPanels;
    bottomPanels.push_back(controls_.get());

	event_filter_ = new video_window::ResizeEventFilter(topPanels, bottomPanels, this);
    installEventFilter(event_filter_);

    connect(controls_.get(), SIGNAL(onDecline()), this, SLOT(onDeclineButtonClicked()), Qt::QueuedConnection);
    connect(controls_.get(), SIGNAL(onVideo()), this, SLOT(onAcceptVideoClicked()),   Qt::QueuedConnection);
    connect(controls_.get(), SIGNAL(onAudio()), this, SLOT(onAcceptAudioClicked()),   Qt::QueuedConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);

    const QSize defaultSize(Utils::scale_value(kIncomingCallWndDefW), Utils::scale_value(kIncomingCallWndDefH));
    setMinimumSize(defaultSize);
    setMaximumSize(defaultSize);
    resize(defaultSize);

    QDesktopWidget dw;
    const auto screen_rect = dw.screenGeometry(dw.primaryScreen());
    const auto wndSize = defaultSize;
    const auto center  = screen_rect.center();

    const QRect rc(center.x() - wndSize.width()*0.5f, center.y() - wndSize.height()*0.5f, wndSize.width(), wndSize.height());
    setGeometry(rc);
}

Ui::IncomingCallWindow::~IncomingCallWindow() {
#ifndef STRIP_VOIP
    delete _rootWidget;
#endif
    removeEventFilter(event_filter_);
	delete event_filter_;
}
void Ui::IncomingCallWindow::onVoipWindowRemoveComplete(quintptr win_id) {
#ifndef STRIP_VOIP
    if (win_id == _rootWidget->frameId()) {
        hide();
    }
#endif
}

void Ui::IncomingCallWindow::onVoipWindowAddComplete(quintptr win_id) {
#ifndef STRIP_VOIP
    if (win_id == _rootWidget->frameId()) {
        showNormal();
    }
#endif
}

void Ui::IncomingCallWindow::showFrame() {
#ifndef STRIP_VOIP
    assert(_rootWidget->frameId());
    if (_rootWidget->frameId()) {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)_rootWidget->frameId(), false, true, 0);
    }
#endif
}

void Ui::IncomingCallWindow::hideFrame() {
#ifndef STRIP_VOIP
    assert(_rootWidget->frameId());
    if (_rootWidget->frameId()) {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)_rootWidget->frameId());
    }
#endif
}

void Ui::IncomingCallWindow::showEvent(QShowEvent* e) {
    header_->show();
    controls_->show();
    QWidget::showEvent(e);
}

void Ui::IncomingCallWindow::hideEvent(QHideEvent* e) {
    header_->hide();
    controls_->hide();
    QWidget::hideEvent(e);
}

void Ui::IncomingCallWindow::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
}

void Ui::IncomingCallWindow::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts) {
    if(contacts.empty()) {
        return;
    }

    if (contacts[0].account == account_ && contacts[0].contact == contact_) {
        std::vector<std::string> users;
        std::vector<std::string> friendly_names;
        for(unsigned ix = 0; ix < contacts.size(); ix++) {
            users.push_back(contacts[ix].contact);

            std::string n = Logic::GetContactListModel()->getDisplayName(contacts[ix].contact.c_str()).toUtf8().data();
            friendly_names.push_back(n);
        }

        header_->setAvatars(users);

        auto name = voip_proxy::VoipController::formatCallName(friendly_names, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8());
        assert(!name.empty());

        header_->setTitle(name.c_str());
        header_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Incoming call").toUtf8());
    }
}

void Ui::IncomingCallWindow::onAcceptVideoClicked() {
    assert(!contact_.empty());
    if (!contact_.empty()) {
		Ui::GetDispatcher()->getVoipController().setAcceptV(contact_.c_str());
    }
}

void Ui::IncomingCallWindow::onAcceptAudioClicked() {
    assert(!contact_.empty());
    if (!contact_.empty()) {
		Ui::GetDispatcher()->getVoipController().setAcceptA(contact_.c_str());
    }
}


void Ui::IncomingCallWindow::onDeclineButtonClicked() {
    assert(!contact_.empty());
    if (!contact_.empty()) {
		Ui::GetDispatcher()->getVoipController().setDecline(contact_.c_str(), false);
    }
}

