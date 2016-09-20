#include "stdafx.h"
#include "VideoWindow.h"
#include "DetachedVideoWnd.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../gui_settings.h"

#include "VideoFrame.h"
#include "VoipTools.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

extern std::string getFotmatedTime(unsigned _ts);
namespace
{
    enum { kPreviewBorderOffset = 12 };
    enum { kMinimumW = 338, kMinimumH = 164 };
    enum { kDefaultW = 640, kDefaultH = 480 };
    enum { kAnimationDefDuration = 500 };

    const std::string videoWndWName = "video_window_w";
    const std::string videoWndHName = "video_window_h";

    bool windowIsOverlapped(platform_specific::GraphicsPanel* _window, quintptr* _exclude, int _size)
    {
        if (!_window)
        {
            return false;
        }
        
#ifdef _WIN32
        HWND target = (HWND)_window->frameId();
        if (!::IsWindowVisible(target))
        {
            return false;
        }

        RECT r;
        ::GetWindowRect(target, &r);

        const int overlapDepthPts = 90;
        typedef std::vector<POINT> ptList;

        const int ptsNumY = (r.bottom - r.top)/overlapDepthPts;
        const int ptsNumX = (r.right - r.left)/overlapDepthPts;

        ptList pts;
        pts.reserve(ptsNumY * ptsNumX);

        for( int j=0; j<ptsNumY; ++j ) 
        {
            int ptY = r.top + overlapDepthPts*j;
            for( int i=0; i<ptsNumX; ++i ) 
            {
                int ptX = r.left + overlapDepthPts*i;

                const POINT pt = { ptX, ptY };
                pts.push_back(pt);
            }
        }

        int ptsCounter = 0;
        for (ptList::const_iterator it = pts.begin(); it != pts.end(); ++it)
        {
            const HWND top = ::WindowFromPoint( *it );

            bool isMyWnd = top == target;
            for (int i = 0; i < _size; i++) {
                isMyWnd |= top == (HWND)_exclude[i];
            }

            if (!isMyWnd) {
                ++ptsCounter;
            }
        }

        return (ptsCounter * 10) >= int(pts.size() * 4); // 40 % overlapping
#elif defined (__APPLE__)
        return platform_macos::windowIsOverlapped(_window);
#endif
    }
}



Ui::VideoWindow::VideoWindow()
    : AspectRatioResizebleWnd()
    , videoPanel_(new(std::nothrow) voipTools::BoundBox<VideoPanel>(this, this))
#ifdef _WIN32
    , detachedWnd_(new DetachedVideoWindow(this))
#endif
    , checkOverlappedTimer_(this)
    , showPanelTimer_(this)
    , hasRemoteVideo_(false)
    , outgoingNotAccepted_(false)
    , secureCallWnd_(NULL)
	, shadow_(this)
#ifdef __APPLE__
    , _fullscreenNotification(*this)
    , isFullscreenAnimation_(false)
    , changeSpaceAnimation_(false)
#endif
{
    callDescription.time = 0;

    topPanelSimple_.init([this] ( )
    {
        return new(std::nothrow) voipTools::BoundBox<VideoPanelHeader>(this, kVPH_ShowName | kVPH_ShowTime | kVPH_ShowMin | kVPH_ShowClose);
    });

    topPanelOutgoing_.init([this] ( )
    {
        return new(std::nothrow) voipTools::BoundBox<VoipSysPanelHeader>(this);
    });
    
#ifdef _WIN32
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
#else
    // We have problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
    // If video window is showing with this flags,, we cannot activate main ICQ window.
    setWindowFlags(Qt::Window /*| Qt::WindowDoesNotAcceptFocus*//*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
    //setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif
    
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    
    QVBoxLayout* rootLayout = new QVBoxLayout();
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);
    
    std::vector<QWidget*> panels;
    panels.push_back(videoPanel_.get());
    if (!!topPanelSimple_)
    {
        panels.push_back((QWidget*)(VideoPanelHeader*)topPanelSimple_);
    }

    if (!!topPanelOutgoing_)
    {
        panels.push_back((QWidget*)(VoipSysPanelHeader*)topPanelOutgoing_);
    }

#ifndef __linux__
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(rootWidget_);
#endif //__linux__

    QIcon icon(":/resources/main_window/appicon.ico");
    setWindowIcon(icon);

    std::vector<QWidget*> topPanels;
    topPanels.push_back((QWidget*)(VideoPanelHeader*)topPanelSimple_);
    if (!!topPanelOutgoing_)
    {
        topPanels.push_back((QWidget*)(VoipSysPanelHeader*)topPanelOutgoing_);
    }

    std::vector<QWidget*> bottomPanels;
    bottomPanels.push_back(videoPanel_.get());

    eventFilter_ = new ResizeEventFilter(topPanels, bottomPanels, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    if (videoPanel_)
    {
        videoPanel_->setFullscreenMode(isInFullscreen());

        connect(videoPanel_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onFullscreenClicked()), this, SLOT(onPanelFullscreenClicked()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);

        connect(videoPanel_.get(), SIGNAL(showPanel()), this, SLOT(showVideoPanel()));
        connect(videoPanel_.get(), SIGNAL(autoHideToolTip(bool&)),  this, SLOT(autoHideToolTip(bool&)));
        connect(videoPanel_.get(), SIGNAL(companionName(QString&)), this, SLOT(companionName(QString&)));
        connect(videoPanel_.get(), SIGNAL(showToolTip(bool &)), this, SLOT(showToolTip(bool &)));
    }

    if (!!topPanelSimple_)
    {
        topPanelSimple_->setFullscreenMode(isInFullscreen());

        connect(topPanelSimple_, SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onClose()), this, SLOT(onPanelClickedClose()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onMinimize()), this, SLOT(onPanelClickedMinimize()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onMaximize()), this, SLOT(onPanelClickedMaximize()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);
        connect(topPanelSimple_, SIGNAL(onSecureCallClicked(const QRect&)), this, SLOT(onSecureCallClicked(const QRect&)), Qt::QueuedConnection);
    }

    if (!!topPanelOutgoing_)
    {
        topPanelOutgoing_->setTitle("");
        topPanelOutgoing_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Ringing...").toUtf8());

        connect(topPanelOutgoing_, SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(topPanelOutgoing_, SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(topPanelOutgoing_, SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipUpdateCipherState(const voip_manager::CipherState&)), this, SLOT(onVoipUpdateCipherState(const voip_manager::CipherState&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMouseTapped(quintptr,const std::string&)), this, SLOT(onVoipMouseTapped(quintptr,const std::string&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaRemoteVideo(bool)), this, SLOT(onVoipMediaRemoteVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallOutAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallOutAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);
	QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), topPanelOutgoing_, SLOT(setVideoStatus(bool)), Qt::DirectConnection);
    
    connect(&checkOverlappedTimer_, SIGNAL(timeout()), this, SLOT(checkOverlap()), Qt::QueuedConnection);
    checkOverlappedTimer_.setInterval(500);
    checkOverlappedTimer_.start();

    connect(&showPanelTimer_, SIGNAL(timeout()), this, SLOT(checkPanelsVis()), Qt::QueuedConnection);
#ifdef __APPLE__
    connect(&_fullscreenNotification, SIGNAL(fullscreenAnimationStart()), this, SLOT(fullscreenAnimationStart()));
    connect(&_fullscreenNotification, SIGNAL(fullscreenAnimationFinish()), this, SLOT(fullscreenAnimationFinish()));
    connect(&_fullscreenNotification, SIGNAL(activeSpaceDidChange()), this, SLOT(activeSpaceDidChange()));
#endif

    showPanelTimer_.setInterval(1500);
    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

    setMinimumSize(Utils::scale_value(kMinimumW), Utils::scale_value(kMinimumH));

    int savedW = Utils::scale_value(kDefaultW);
    int savedH = Utils::scale_value(kDefaultH);

    getWindowSizeFromSettings(savedW, savedH);
    resize(savedW, savedH);
}

Ui::VideoWindow::~VideoWindow()
{
    checkOverlappedTimer_.stop();

    removeEventFilter(eventFilter_);
    delete eventFilter_;
}

quintptr Ui::VideoWindow::getContentWinId()
{
    return rootWidget_->frameId();
}

void Ui::VideoWindow::onPanelMouseEnter()
{
    showPanelTimer_.stop();
    videoPanel_->fadeIn(kAnimationDefDuration);
    topPanelSimple_.fadeIn(kAnimationDefDuration);
    topPanelOutgoing_.fadeIn(kAnimationDefDuration);

    offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
}

void Ui::VideoWindow::onVoipMediaRemoteVideo(bool _enabled)
{
    hasRemoteVideo_ = _enabled;

    checkPanelsVisibility();
    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
        useAspect();
    }
    else
    {
        showPanelTimer_.stop();
        videoPanel_->startToolTipHideTimer();
        
        unuseAspect();
        videoPanel_->fadeIn(kAnimationDefDuration);
        topPanelSimple_.fadeIn(kAnimationDefDuration);
        topPanelOutgoing_.fadeIn(kAnimationDefDuration);

        offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
    }
}

void Ui::VideoWindow::onPanelMouseLeave()
{
    const bool haveSecurityWnd = secureCallWnd_ && secureCallWnd_->isVisible();
    if (hasRemoteVideo_ && !haveSecurityWnd)
    {
        showPanelTimer_.start();
    }
}

#ifndef _WIN32 // on win32 we have our own mouse event handler
void Ui::VideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    showPanelTimer_.stop();
    
    videoPanel_->fadeIn(kAnimationDefDuration);
    topPanelSimple_.fadeIn(kAnimationDefDuration);
    topPanelOutgoing_.fadeIn(kAnimationDefDuration);
    
    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
    }
}

void Ui::VideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    showPanelTimer_.start();
}

void Ui::VideoWindow::mouseDoubleClickEvent(QMouseEvent* _e)
{
    QWidget::mouseDoubleClickEvent(_e);
    _switchFullscreen();
}

void Ui::VideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    showPanelTimer_.stop();
    
    videoPanel_->fadeIn(kAnimationDefDuration);
    topPanelSimple_.fadeIn(kAnimationDefDuration);
    topPanelOutgoing_.fadeIn(kAnimationDefDuration);
    
    offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
    
    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
    }
}

#endif

bool Ui::VideoWindow::isOverlapped() const
{
    quintptr friendlyWnds[] =
    {
        !!videoPanel_ ? videoPanel_->winId() : NULL,
        !!topPanelSimple_ ? topPanelSimple_.getId() : NULL,
        !!topPanelOutgoing_ ? topPanelOutgoing_.getId() : NULL,
        !!secureCallWnd_ ? secureCallWnd_->winId() : NULL
    };

    return windowIsOverlapped(rootWidget_, friendlyWnds, sizeof(friendlyWnds) / sizeof(friendlyWnds[0]));
}

void Ui::VideoWindow::checkOverlap()
{
    if (!isVisible())
    {
        return;
    }
    
    assert(!!rootWidget_);
    if (!rootWidget_)
    {
        return;
    }

    // We do not change normal video widnow to small video vindow, if
    // there is active modal window, becuase it can lost focus and
    // close automatically.
    if (!!detachedWnd_ && QApplication::activeModalWidget() == nullptr)
    {
        // Additional to overlapping we show small video window, if VideoWindow was minimized.
        if (hasRemoteVideo_ && !detachedWnd_->closedManualy() &&
            (isOverlapped() || isMinimized()))
        {
            QApplication::alert(this);
            detachedWnd_->showFrame();
        }
        else
        {
            detachedWnd_->hideFrame();
        }
    }
}

void Ui::VideoWindow::checkPanelsVis()
{
    showPanelTimer_.stop();
    videoPanel_->fadeOut(kAnimationDefDuration);
    topPanelSimple_.fadeOut(kAnimationDefDuration);
    topPanelOutgoing_.fadeOut(kAnimationDefDuration);

    offsetWindow(Utils::scale_value(kPreviewBorderOffset));
}

void Ui::VideoWindow::onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall)
{
    if (!!topPanelSimple_)
    {
        topPanelSimple_->setTime(_secElapsed, _hasCall);
        callDescription.time = _hasCall ? _secElapsed : 0;
        updateWindowTitle();
    }

    if (!!topPanelOutgoing_)
    {
        if (_hasCall)
        {
            topPanelOutgoing_->setStatus(getFotmatedTime(_secElapsed).c_str());
        }
        else
        {
            topPanelOutgoing_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Ringing...").toUtf8());
        }
    }
}

void Ui::VideoWindow::onPanelClickedMinimize()
{
    showMinimized();
}

void Ui::VideoWindow::onPanelClickedMaximize()
{
    if (isMaximized())
    {
        showNormal();
    }
    else
    {
        showMaximized();
    }
    videoPanel_->setFullscreenMode(isInFullscreen());

    if (!!topPanelSimple_)
    {
        topPanelSimple_->setFullscreenMode(isInFullscreen());
    }
}

void Ui::VideoWindow::onPanelClickedClose()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::hideFrame()
{
    Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
}

void Ui::VideoWindow::showFrame()
{
    Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), true, false, (videoPanel_->geometry().height() + Utils::scale_value(5)) * Utils::scale_bitmap(1));
    
    offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
}

void Ui::VideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        callMethodProxy("hide");
        //hide();
    }
}

void Ui::VideoWindow::updatePanels() const
{
    if (!rootWidget_)
    {
        assert(false);
        return;
    }
    rootWidget_->clearPanels();
    
    std::vector<QWidget*> panels;
    if (videoPanel_->isVisible())
    {
        panels.push_back(videoPanel_.get());
    }
    
    if (!!topPanelSimple_)
    {
        if (topPanelSimple_->isVisible())
        {
            panels.push_back(const_cast<QWidget*>((const QWidget*)(const VideoPanelHeader*)topPanelSimple_));
        }
    }
    
    if (!!topPanelOutgoing_)
    {
        if (topPanelOutgoing_->isVisible())
        {
            panels.push_back(const_cast<QWidget*>((const QWidget*)(const VoipSysPanelHeader*)topPanelOutgoing_));
        }
    }
    
    //qDebug() << "+ UPDATE PANELS CALLED";
    rootWidget_->addPanels(panels);
    
#ifdef __APPLE__
    if (videoPanel_->isVisible())
    {
        platform_macos::setWindowPosition(*videoPanel_.get(), *this, false);
    }

    if (!!topPanelSimple_ && topPanelSimple_->isVisible())
    {
        platform_macos::setWindowPosition(*(VideoPanelHeader*)topPanelSimple_, *this, true);
    }

    if (!!topPanelOutgoing_ && topPanelOutgoing_->isVisible())
    {
        platform_macos::setWindowPosition(*(VoipSysPanelHeader*)topPanelOutgoing_, *this, true);
    }
#endif
}

void Ui::VideoWindow::onVoipWindowAddComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        updatePanels();
        show();
    }
}

void Ui::VideoWindow::hideEvent(QHideEvent* _ev)
{
    AspectRatioResizebleWnd::hideEvent(_ev);
    checkPanelsVisibility(true);
    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

	shadow_.hideShadow();
}

void Ui::VideoWindow::showEvent(QShowEvent* _ev)
{
    AspectRatioResizebleWnd::showEvent(_ev);
    checkPanelsVisibility();

    showPanelTimer_.stop();
    videoPanel_->fadeIn(kAnimationDefDuration);
    topPanelSimple_.fadeIn(kAnimationDefDuration);
    topPanelOutgoing_.fadeIn(kAnimationDefDuration);

    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
    }

    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

    // If ICQ is in normal mode, we reload window size from settings.
    if (!isInFullscreen())
    {
        int savedW = 0;
        int savedH = 0;
        if (getWindowSizeFromSettings(savedW, savedH))
        {
            resize(savedW, savedH);
        }
    }

    showNormal();
    activateWindow();
    videoPanel_->setFullscreenMode(isInFullscreen());

    if (!!topPanelSimple_)
    {
        topPanelSimple_->setFullscreenMode(isInFullscreen());
    }

	shadow_.showShadow();

    updateUserName();
}

void Ui::VideoWindow::onVoipMouseTapped(quintptr _hwnd, const std::string& _tapType)
{
    const bool dblTap = _tapType == "double";
    const bool over = _tapType == "over";

    if (!!detachedWnd_ && detachedWnd_->getVideoFrameId() == _hwnd)
    {
        if (dblTap)
        {
#ifdef _WIN32
            raise();
#endif
        }
    } else if ((quintptr)rootWidget_->frameId() == _hwnd)
    {
        if (dblTap)
        {
            _switchFullscreen();
        }
        else if (over)
        {
            showPanelTimer_.stop();

            videoPanel_->fadeIn(kAnimationDefDuration);
            topPanelSimple_.fadeIn(kAnimationDefDuration);
            topPanelOutgoing_.fadeIn(kAnimationDefDuration);

            offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
            
            if (hasRemoteVideo_)
            {
                showPanelTimer_.start();
            }
        }
    }
}

void Ui::VideoWindow::onPanelFullscreenClicked()
{
    _switchFullscreen();
}

void Ui::VideoWindow::onVoipCallOutAccepted(const voip_manager::ContactEx& /*contact_ex*/)
{
    outgoingNotAccepted_ = false;
}

void Ui::VideoWindow::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
{
    if (!_contactEx.incoming)
    {
        outgoingNotAccepted_ = true;
    }
}

void Ui::VideoWindow::checkPanelsVisibility(bool _forceHide /*= false*/)
{
    videoPanel_->setFullscreenMode(isInFullscreen());
    if (!!topPanelSimple_)
    {
        topPanelSimple_->setFullscreenMode(isInFullscreen());
    }

    if (isHidden() || isMinimized() || _forceHide)
    {
        videoPanel_->hide();
        topPanelSimple_.hide();
        topPanelOutgoing_.hide();
        return;
    }

    videoPanel_->show();
#ifdef __APPLE__
    if (isInFullscreen())
    {
#else
    if (false)
    {
#endif
        topPanelSimple_.hide();
        topPanelOutgoing_.hide();
    }
    else
    {
        if (outgoingNotAccepted_)
        {
            topPanelSimple_.hide();
            topPanelOutgoing_.show();
            videoPanel_->talkFinished();
        }
        else
        {
            topPanelSimple_.show();
            topPanelOutgoing_.hide();
            videoPanel_->talkStarted();
        }
    }
        
    if (!!rootWidget_)
    {
        rootWidget_->clearPanels();
        updatePanels();
    }
}

void Ui::VideoWindow::_switchFullscreen()
{
    switchFullscreen();
    checkPanelsVisibility();
    
    if (!!rootWidget_)
    {
        rootWidget_->fullscreenModeChanged(isInFullscreen());
    }
    
    videoPanel_->fadeIn(kAnimationDefDuration);
    topPanelSimple_.fadeIn(kAnimationDefDuration);
    topPanelOutgoing_.fadeIn(kAnimationDefDuration);
    
    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
    }
}

void Ui::VideoWindow::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& _contacts)
{
    if(_contacts.empty())
    {
        return;
    }

    currentContacts_ = _contacts;

    updateUserName();
}

void Ui::VideoWindow::updateWindowTitle()
{
    std::stringstream fullName;
    fullName << "[ " << getFotmatedTime(callDescription.time) << " ]  " << callDescription.name;

    setWindowTitle(fullName.str().c_str());
}

void Ui::VideoWindow::paintEvent(QPaintEvent *_e)
{
    return AspectRatioResizebleWnd::paintEvent(_e);
}

void Ui::VideoWindow::changeEvent(QEvent* _e)
{
    AspectRatioResizebleWnd::changeEvent(_e);
    // Becuase we attach panels to video window, we can remove this code.
    // It fixes some problem with window ordering.
    /*if (e->type() == QEvent::ActivationChange)
    {
#ifndef __APPLE__
        if (isActiveWindow())
        {
            videoPanel_->blockSignals(true);
            videoPanel_->raise();
            videoPanel_->blockSignals(false);
            
            if (!!topPanelOutgoing_)
            {
                topPanelOutgoing_->blockSignals(true);
                topPanelOutgoing_->raise();
                topPanelOutgoing_->blockSignals(false);
            }

            if (!!topPanelSimple_)
            {
                topPanelSimple_->blockSignals(true);
                topPanelSimple_->raise();
                topPanelSimple_->blockSignals(false);
            }
        }
#endif
    }
    else*/ if (_e->type() == QEvent::WindowStateChange)
    {
#ifdef __APPLE__
        // On Mac we can go to normal size using OSX.
        // We need to update buttons and panel states.
        if (_e->spontaneous())
        {
            checkPanelsVisibility();
            if (!!rootWidget_)
            {
                rootWidget_->fullscreenModeChanged(isInFullscreen());
            }
        }
#endif
    }
}

void Ui::VideoWindow::closeEvent(QCloseEvent* _e)
{
    AspectRatioResizebleWnd::closeEvent(_e);
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::resizeEvent(QResizeEvent* _e)
{
    AspectRatioResizebleWnd::resizeEvent(_e);
    
    bool isSystemEvent = _e->spontaneous();
    qt_gui_settings* settings = Ui::get_gui_settings();
    // Save size in settings only if it is system or user event.
    // Ignore any inside window size changing.
    if (isSystemEvent && settings) 
    {
        settings->set_value<int>(videoWndWName.c_str(), width());
        settings->set_value<int>(videoWndHName.c_str(), height());
    }

#ifdef _WIN32
    int borderWidth = Utils::scale_value(2);
    const auto fg = frameGeometry();
    const auto ge = geometry();

    /* It is better to use setMask, but for Windows it breaks fadein/out animation for ownered panesl.
    We use Win API here to make small border.
    borderWidth = std::min(ge.top() - fg.top(), 
                   std::min(fg.right() - ge.right(),
                   std::min(fg.bottom() - ge.bottom(),
                   std::min(ge.left() - fg.left(), 
                   borderWidth))));

    QRegion reg(-borderWidth, -borderWidth, ge.width() + 2*borderWidth, ge.height() + 2*borderWidth);
    setMask(reg);
    */

    borderWidth = std::max(
        std::min(ge.top() - fg.top(),
            std::min(fg.right() - ge.right(),
                std::min(fg.bottom() - ge.bottom(),
                    ge.left() - fg.left()))), borderWidth) - borderWidth;

    HRGN rectRegion = CreateRectRgn(borderWidth, borderWidth, fg.width() - borderWidth, fg.height() - borderWidth);
    if (rectRegion)
    {
        SetWindowRgn((HWND)winId(), rectRegion, FALSE);
        DeleteObject (rectRegion);
    }
#endif
}

void Ui::VideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count <= 1)
    {
        unuseAspect();
        hasRemoteVideo_ = false;
        outgoingNotAccepted_ = false;

#ifdef __APPLE__
        // On mac if video window is fullscreen and on active, we make it active.
        // It needs to correct close video window.
        if (isFullScreen() && !isActiveWindow() && !topPanelSimple_->isActiveWindow() &&
            !topPanelOutgoing_->isActiveWindow() && !videoPanel_->isActiveWindow())
        {
            // Make video window active and wait finish of switch animation.
            callMethodProxy("activateWindow");
            callMethodProxy("raise");
            changeSpaceAnimation_ = true;
        }
#endif
        
        callMethodProxy("onEscPressed");

        //onEscPressed();

        if (secureCallWnd_)
        {
            secureCallWnd_->hide();
        }

        if (!!topPanelSimple_)
        {
            topPanelSimple_->enableSecureCall(false);
        }

        if (!!topPanelOutgoing_)
        {
            topPanelOutgoing_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Ringing...").toUtf8());
        }
    }
}

void Ui::VideoWindow::escPressed()
{
    onEscPressed();
}

void Ui::VideoWindow::onEscPressed()
{
    if (isInFullscreen())
    {
        _switchFullscreen();
    }
}

void Ui::VideoWindow::onSecureCallWndOpened()
{
    onPanelMouseEnter();
    if (!!topPanelSimple_)
    {
        topPanelSimple_->setSecureWndOpened(true);
    }
}

void Ui::VideoWindow::onSecureCallWndClosed()
{
    onPanelMouseLeave();
    if (!!topPanelSimple_)
    {
        topPanelSimple_->setSecureWndOpened(false);
    }
}

void Ui::VideoWindow::onVoipUpdateCipherState(const voip_manager::CipherState& _state)
{
    if (!!topPanelSimple_)
    {
        topPanelSimple_->enableSecureCall(voip_manager::CipherState::kCipherStateEnabled == _state.state);
    }

    if (secureCallWnd_ && voip_manager::CipherState::kCipherStateEnabled == _state.state)
    {
        secureCallWnd_->setSecureCode(_state.secureCode);
    }
}

void Ui::VideoWindow::onSecureCallClicked(const QRect& _rc)
{
    if (!secureCallWnd_)
    {
        secureCallWnd_ = new SecureCallWnd();
        connect(secureCallWnd_, SIGNAL(onSecureCallWndOpened()), this, SLOT(onSecureCallWndOpened()), Qt::QueuedConnection);
        connect(secureCallWnd_, SIGNAL(onSecureCallWndClosed()), this, SLOT(onSecureCallWndClosed()), Qt::QueuedConnection);
    }

    if (!secureCallWnd_)
    {
        return;
    }

    const QPoint windowTCPt((_rc.left() + _rc.right()) * 0.5f + Utils::scale_value(12), _rc.bottom() - Utils::scale_value(4));
    const QPoint secureCallWndTLPt(windowTCPt.x() - secureCallWnd_->width()*0.5f, windowTCPt.y());

    voip_manager::CipherState cipherState;
    Ui::GetDispatcher()->getVoipController().getSecureCode(cipherState);

    if (voip_manager::CipherState::kCipherStateEnabled == cipherState.state)
    {
        secureCallWnd_->setSecureCode(cipherState.secureCode);
        secureCallWnd_->move(secureCallWndTLPt);
        secureCallWnd_->show();
        secureCallWnd_->raise();
        secureCallWnd_->setFocus(Qt::NoFocusReason);
    }
}

template<typename __Type>
Ui::VideoWindow::VideoPanelUnit<__Type>::VideoPanelUnit()
: effect_(nullptr)
{

}

template<typename __Type>
Ui::VideoWindow::VideoPanelUnit<__Type>::~VideoPanelUnit()
{
    delete effect_;
    panel_.reset();
}

template<typename __Type>
bool Ui::VideoWindow::VideoPanelUnit<__Type>::init(const std::function<__Type*()>& creator)
{
    if (!!panel_)
    {
        return true;
    }

    panel_.reset(creator());
    assert(!!panel_);
    if (!panel_)
    {
        return false;
    }

    delete effect_;
    effect_ = new(std::nothrow) UIEffects(*panel_);
    assert(!!effect_);
    if (!effect_)
    {
        return false;
    }

    return true;
}

template<typename __Type>
void Ui::VideoWindow::VideoPanelUnit<__Type>::fadeIn(unsigned int _ms)
{
    if (effect_)
    {
        effect_->fadeIn(_ms);
    }
}

template<typename __Type>
void Ui::VideoWindow::VideoPanelUnit<__Type>::fadeOut(unsigned int _ms)
{
    if (effect_)
    {
        effect_->fadeOut(_ms);
    }
}

template<typename __Type>
void Ui::VideoWindow::VideoPanelUnit<__Type>::show()
{
    if (!!panel_)
    {
        panel_->show();
    }
}

template<typename __Type>
void Ui::VideoWindow::VideoPanelUnit<__Type>::hide()
{
    if (!!panel_)
    {
        panel_->hide();
    }
}

template<typename __Type>
Ui::VideoWindow::VideoPanelUnit<__Type>::operator __Type*() 
{
    return !!panel_ ? panel_.get() : nullptr;
}

template<typename __Type>
Ui::VideoWindow::VideoPanelUnit<__Type>::operator __Type*() const
{
    return !!panel_ ? panel_.get() : nullptr;
}

template<typename __Type>
WId Ui::VideoWindow::VideoPanelUnit<__Type>::getId() const
{
    return !!panel_ ? panel_->winId() : NULL;
}

template<typename __Type>
bool Ui::VideoWindow::VideoPanelUnit<__Type>::operator!() const 
{
    return !panel_;
}

template<typename __Type>
__Type* Ui::VideoWindow::VideoPanelUnit<__Type>::operator->() 
{
    return !!panel_ ? panel_.get() : NULL;
}

template<typename __Type>
const __Type* Ui::VideoWindow::VideoPanelUnit<__Type>::operator->() const 
{
    return !!panel_ ? panel_.get() : NULL;
}

bool Ui::VideoWindow::getWindowSizeFromSettings(int& _nWidth, int& _nHeight)
{
    bool res = false;
    if (qt_gui_settings* settings = Ui::get_gui_settings())
    {
        const int tmpW = settings->get_value<int>(videoWndWName.c_str(), -1);
        const int tmpH = settings->get_value<int>(videoWndHName.c_str(), -1);

        if (tmpW > 0 && tmpH > 0)
        {
            _nWidth  = tmpW;
            _nHeight = tmpH;
            res = true;
        }
    }
    return res;
}


void Ui::VideoWindow::fullscreenAnimationStart()
{
#ifdef __APPLE__
    isFullscreenAnimation_ = true;
    rootWidget_->fullscreenAnimationStart();
#endif
}

void Ui::VideoWindow::fullscreenAnimationFinish()
{
#ifdef __APPLE__
    if (isFullscreenAnimation_)
    {
        isFullscreenAnimation_ = false;
        rootWidget_->fullscreenAnimationFinish();
        
        executeCommandList();
    }
#endif
}
    
void Ui::VideoWindow::activeSpaceDidChange()
{
#ifdef __APPLE__
    if (changeSpaceAnimation_)
    {
        changeSpaceAnimation_ = false;
        
        executeCommandList();
    }
#endif
}
    
void Ui::VideoWindow::executeCommandList()
{
#ifdef __APPLE__
    QMutableListIterator<QString> iterator(commandList_);
    
    while (iterator.hasNext())
    {
        if (!isFullscreenAnimation_ && !changeSpaceAnimation_)
        {
            callMethodProxy(iterator.next());
            iterator.remove();
        }
        else
        {
            // Animation is in progress.
            break;
        }
    }
#endif
}

    
void Ui::VideoWindow::callMethodProxy(const QString& _method)
{
#ifdef __APPLE__
    if (isFullscreenAnimation_ || changeSpaceAnimation_)
    {
        commandList_.push_back(_method);
    }
    else
#endif
    {
        QMetaObject::invokeMethod(this, _method.toLatin1().data(), /*bQueued ? Qt::QueuedConnection :*/ Qt::DirectConnection);
    }
}
    
    
void Ui::VideoWindow::offsetWindow(int _bottom)
{
    Ui::GetDispatcher()->getVoipController().setWindowOffsets(
        (quintptr)rootWidget_->frameId(),
        Utils::scale_value(kPreviewBorderOffset) * Utils::scale_bitmap(1),
        Utils::scale_value(kPreviewBorderOffset) * Utils::scale_bitmap(1),
        Utils::scale_value(kPreviewBorderOffset) * Utils::scale_bitmap(1),
        _bottom * Utils::scale_bitmap(1)
    );
}

void Ui::VideoWindow::showVideoPanel()
{
    videoPanel_->fadeIn(kAnimationDefDuration);
    
    if (hasRemoteVideo_)
    {
        showPanelTimer_.start();
    }
}

void Ui::VideoWindow::autoHideToolTip(bool& autoHide)
{
    autoHide = !hasRemoteVideo_;
}

void Ui::VideoWindow::companionName(QString& name)
{
    name = QString(callDescription.name.c_str());
}

void Ui::VideoWindow::showToolTip(bool &show)
{
    show = !isMinimized() && !isOverlapped();
}

void Ui::VideoWindow::updateUserName()
{
    if (currentContacts_.empty())
    {
        return;
    }

    std::vector<std::string> users;
    std::vector<std::string> friendlyNames;
    for (unsigned ix = 0; ix < currentContacts_.size(); ix++)
    {
        users.push_back(currentContacts_[ix].contact);

        std::string n = Logic::getContactListModel()->getDisplayName(currentContacts_[ix].contact.c_str()).toUtf8().data();
        friendlyNames.push_back(n);
    }

    if (!!topPanelOutgoing_)
    {
        topPanelOutgoing_->setAvatars(users);
    }

    auto name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8());
    assert(!name.empty());

    if (!!topPanelSimple_)
    {
        topPanelSimple_->setCallName(name);
    }

    if (!!topPanelOutgoing_)
    {
        topPanelOutgoing_->setTitle(name.c_str());
    }

    callDescription.name = name;
    updateWindowTitle();
}

bool Ui::VideoWindow::isActiveWindow()
{
    return AspectRatioResizebleWnd::isActiveWindow() || (topPanelSimple_ && topPanelSimple_->isActiveWindow())
        || (topPanelOutgoing_ && topPanelOutgoing_->isActiveWindow()) || (videoPanel_ && videoPanel_->isActiveWindow())
        || (detachedWnd_ && detachedWnd_->isActiveWindow());
}


