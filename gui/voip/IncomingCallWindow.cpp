#include "stdafx.h"
#include "IncomingCallWindow.h"

#include "CallPanelMain.h"
#include "DetachedVideoWnd.h"
#include "VideoWindow.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"
#include "../../core/Voip/VoipManagerDefines.h"

namespace
{
    enum
    {
        kIncomingCallWndDefH = 300,
        kIncomingCallWndDefW = 400,
    };

    // default offset for next incoming window.
    const float defaultWindowOffset = 0.25f;

#ifdef _DEBUG
    const int ht[kIncomingCallWndDefH > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefH must be not null
    const int wt[kIncomingCallWndDefW > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefW must be not null
#endif
}

QList<Ui::IncomingCallWindow*> Ui::IncomingCallWindow::instances_;

Ui::IncomingCallWindow::IncomingCallWindow(const std::string& _account, const std::string& _contact)
    : QWidget(NULL) 
    , contact_(_contact)
    , account_(_account)
    , header_(new(std::nothrow) voipTools::BoundBox<VoipSysPanelHeader>(this))
    , controls_(new voipTools::BoundBox<IncomingCallControls>(this))
	, shadow_(this)
{
    setStyleSheet(Utils::LoadStyle(":/voip/incoming_call.qss"));
    QIcon icon(":/resources/main_window/appicon.ico");
    setWindowIcon(icon);
    
#ifdef _WIN32
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_ShowWithoutActivating);
#else
    // We have problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
    // If video window is showing with this flags, we cannot activate main ICQ window.
    // We use Qt::Dialog here, because it works correctly, if main window is fullscreen.
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    //setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif
    
    setAttribute(Qt::WA_UpdatesDisabled);

    QVBoxLayout* rootLayout = new QVBoxLayout();
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);
    
    header_->setWindowFlags(header_->windowFlags() | Qt::WindowStaysOnTopHint);
    controls_->setWindowFlags(controls_->windowFlags() | Qt::WindowStaysOnTopHint);

    std::vector<QWidget*> panels;
    panels.push_back(header_.get());
    panels.push_back(controls_.get());
#ifndef STRIP_VOIP
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(rootWidget_);
#endif //__linux__

    std::vector<QWidget*> topPanels;
    topPanels.push_back(header_.get());

    std::vector<QWidget*> bottomPanels;
    bottomPanels.push_back(controls_.get());

    eventFilter_ = new ResizeEventFilter(topPanels, bottomPanels, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    connect(controls_.get(), SIGNAL(onDecline()), this, SLOT(onDeclineButtonClicked()), Qt::QueuedConnection);
    connect(controls_.get(), SIGNAL(onVideo()), this, SLOT(onAcceptVideoClicked()),   Qt::QueuedConnection);
    connect(controls_.get(), SIGNAL(onAudio()), this, SLOT(onAcceptAudioClicked()),   Qt::QueuedConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
	QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), header_.get(), SLOT(setVideoStatus(bool)), Qt::DirectConnection);
	QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), controls_.get(), SLOT(setVideoStatus(bool)), Qt::DirectConnection);

    const QSize defaultSize(Utils::scale_value(kIncomingCallWndDefW), Utils::scale_value(kIncomingCallWndDefH));
    setMinimumSize(defaultSize);
    setMaximumSize(defaultSize);
    resize(defaultSize);

    QDesktopWidget dw;
    const auto screenRect = dw.screenGeometry(dw.primaryScreen());
    const auto wndSize = defaultSize;
    const auto center  = screenRect.center();

    QPoint windowPosition = QPoint(center.x() - wndSize.width() * 0.5f, center.y() - wndSize.height() * 0.5f);
    windowPosition = findBestPosition(windowPosition, QPoint(wndSize.width() * defaultWindowOffset, wndSize.height() * defaultWindowOffset));

    const QRect rc(windowPosition, wndSize);
    setGeometry(rc);

    // I hope constructor and destructor are called from the same thread.
    instances_.push_back(this);
}

Ui::IncomingCallWindow::~IncomingCallWindow()
{
#ifndef STRIP_VOIP
    delete rootWidget_;
#endif
    removeEventFilter(eventFilter_);
    delete eventFilter_;

    // I hope constructor and destructor are called from the same thread.
    instances_.removeAll(this);
}

void Ui::IncomingCallWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
#ifndef STRIP_VOIP
    if (_winId == rootWidget_->frameId())
    {
        hide();
    }
#endif
}

void Ui::IncomingCallWindow::onVoipWindowAddComplete(quintptr _winId)
{
#ifndef STRIP_VOIP
    if (_winId == rootWidget_->frameId())
    {
        showNormal();
    }
#endif
}

void Ui::IncomingCallWindow::showFrame()
{
#ifndef STRIP_VOIP
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), false, true, 0);
    }
#endif
}

void Ui::IncomingCallWindow::hideFrame()
{
#ifndef STRIP_VOIP
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
    }
#endif
    hide();
}

void Ui::IncomingCallWindow::showEvent(QShowEvent* _e)
{
    updateTitle();

    header_->show();
    controls_->show();

	shadow_.showShadow();

    QWidget::showEvent(_e);
}

void Ui::IncomingCallWindow::hideEvent(QHideEvent* _e)
{
	header_->hide();
	controls_->hide();

	shadow_.hideShadow();

    QWidget::hideEvent(_e);
}

void Ui::IncomingCallWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
}

void Ui::IncomingCallWindow::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& _contacts)
{
    if(_contacts.empty()) {
        return;
    }

    if (_contacts[0].account == account_ && _contacts[0].contact == contact_)
    {
        contacts_ = _contacts;
        std::vector<std::string> users;
        for(unsigned ix = 0; ix < _contacts.size(); ix++)
        {
            users.push_back(_contacts[ix].contact);
        }

        header_->setAvatars(users);

        updateTitle();

        header_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Incoming call").toUtf8());
    }
}

void Ui::IncomingCallWindow::onAcceptVideoClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptV(contact_.c_str());
    }
    hide();
}

void Ui::IncomingCallWindow::onAcceptAudioClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptA(contact_.c_str());
    }
    hide();
}


void Ui::IncomingCallWindow::onDeclineButtonClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setDecline(contact_.c_str(), false);
    }
    hide();
}


void Ui::IncomingCallWindow::updateTitle()
{
    if(contacts_.empty())
    {
        return;
    }

    if (contacts_[0].account == account_ && contacts_[0].contact == contact_)
    {
        std::vector<std::string> friendlyNames;
        for(unsigned ix = 0; ix < contacts_.size(); ix++)
        {
            std::string n = Logic::getContactListModel()->getDisplayName(contacts_[ix].contact.c_str()).toUtf8().data();
            friendlyNames.push_back(n);
        }

        auto name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8());
        assert(!name.empty());

        header_->setTitle(name.c_str());
    }
}

QPoint Ui::IncomingCallWindow::findBestPosition(const QPoint& _windowPosition, const QPoint& _offset)
{
    QPoint res = _windowPosition;

    QList<QPoint> avaliblePositions;

    // Create list with all avalible positions.
    for (int i = 0; i < instances_.size() + 1; i ++)
    {
        avaliblePositions.push_back(_windowPosition + _offset * i);
    }

    // Search position for next window.
    // For several incommign window we place them into cascade.
    for (IncomingCallWindow* window : instances_)
    {
        if (window)
        {
            avaliblePositions.removeAll(window->pos());
        }
    }

    if (!avaliblePositions.empty())
    {
        res = avaliblePositions.first();
    }

    return res;
}