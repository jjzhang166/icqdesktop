#include "stdafx.h"
#include "VideoWindow.h"
#include "DetachedVideoWnd.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../gui_settings.h"
#include "MaskManager.h"

#include "VideoFrame.h"
#include "VoipTools.h"
#include "VideoPanel.h"
#include "VideoPanelHeader.h"
#include "VoipSysPanelHeader.h"
#include "MaskPanel.h"
#include "secureCallWnd.h"


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

    bool windowIsOverlapped(platform_specific::GraphicsPanel* _window, const std::vector<QWidget*>& _exclude)
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

        const int ptsNumY = (r.bottom - r.top)/overlapDepthPts;
        const int ptsNumX = (r.right - r.left)/overlapDepthPts;

		std::vector<POINT> pts;
        pts.reserve(ptsNumY * ptsNumX);

		for (int j = 0; j < ptsNumY; ++j)
		{
			int ptY = r.top + overlapDepthPts*j;
			for (int i = 0; i < ptsNumX; ++i)
			{
				int ptX = r.left + overlapDepthPts*i;

				const POINT pt = { ptX, ptY };
				pts.push_back(pt);
			}
		}

        int ptsCounter = 0;
        for (auto it = pts.begin(); it != pts.end(); ++it)
        {
            const HWND top = ::WindowFromPoint( *it );

            bool isMyWnd = top == target;
            for (auto itWindow = _exclude.begin(); itWindow != _exclude.end(); itWindow++)
			{
				if (*itWindow)
				{
					isMyWnd |= top == (HWND)(*itWindow)->winId();
				}
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
	, maskPanel_(nullptr)
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

    topPanelSimple_    = std::unique_ptr<VideoPanelHeader>(new(std::nothrow) voipTools::BoundBox<VideoPanelHeader>(this, kVPH_ShowName | kVPH_ShowTime | kVPH_ShowMin | kVPH_ShowClose));
    topPanelOutgoing_  = std::unique_ptr<VoipSysPanelHeader>(new(std::nothrow) voipTools::BoundBox<VoipSysPanelHeader>(this));

	// TODO: use height of panels.
	maskPanel_ = std::unique_ptr<MaskPanel>(new(std::nothrow) voipTools::BoundBox <MaskPanel>(this, this, Utils::scale_value(30), Utils::scale_value(56)));
	
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
    
	panels_.push_back(videoPanel_.get());
    if (!!topPanelSimple_)
    {
		panels_.push_back(topPanelSimple_.get());
    }

    if (!!topPanelOutgoing_)
    {
		panels_.push_back(topPanelOutgoing_.get());
    }

	panels_.push_back(maskPanel_.get());
    
#ifndef _WIN32
    if (!!topPanelOutgoing_)
    {
        transporentPanels_.push_back(topPanelOutgoing_.get());
    }
    transporentPanels_.push_back(maskPanel_.get());
#endif

    
#ifndef __linux__
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels_);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(rootWidget_);
#endif //__linux__

    QIcon icon(":/resources/main_window/appicon.ico");
    setWindowIcon(icon);

    eventFilter_ = new ResizeEventFilter(panels_, shadow_.getShadowWidget(), this);
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
        connect(videoPanel_.get(), &VideoPanel::isVideoWindowActive, [this](bool& isActive)
                {
                    isActive = isActiveWindow();
                });
    }

    if (!!topPanelSimple_)
    {
        topPanelSimple_->setFullscreenMode(isInFullscreen());

        connect(topPanelSimple_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onClose()), this, SLOT(onPanelClickedClose()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onMinimize()), this, SLOT(onPanelClickedMinimize()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onMaximize()), this, SLOT(onPanelClickedMaximize()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);
        connect(topPanelSimple_.get(), SIGNAL(onSecureCallClicked(const QRect&)), this, SLOT(onSecureCallClicked(const QRect&)), Qt::QueuedConnection);
    }

    if (!!topPanelOutgoing_)
    {
        topPanelOutgoing_->setTitle("");
        topPanelOutgoing_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Ringing...").toUtf8());

        connect(topPanelOutgoing_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(topPanelOutgoing_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(topPanelOutgoing_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);
    }

	if (!!maskPanel_)
	{
		connect(maskPanel_.get(), SIGNAL(makePreviewPrimary()), this, SLOT(onSetPreviewPrimary()), Qt::QueuedConnection);
        connect(maskPanel_.get(), SIGNAL(makeInterlocutorPrimary()), this, SLOT(onSetContactPrimary()), Qt::QueuedConnection);
		connect(maskPanel_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
		connect(maskPanel_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(maskPanel_.get(), SIGNAL(onShowMaskList()), this, SLOT(onShowMaskList()), Qt::QueuedConnection);
        connect(maskPanel_.get(), SIGNAL(onHideMaskList()), this, SLOT(onHideMaskList()), Qt::QueuedConnection);

        connect(maskPanel_.get(), SIGNAL(getCallStatus(bool&)), this, SLOT(getCallStatus(bool&)), Qt::DirectConnection);
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
	QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), topPanelOutgoing_.get(), SLOT(setVideoStatus(bool)), Qt::DirectConnection);
    
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

	fadeInPanels(kAnimationDefDuration);
}

void Ui::VideoWindow::onVoipMediaRemoteVideo(bool _enabled)
{
    hasRemoteVideo_ = _enabled;

    checkPanelsVisibility();
    if (hasRemoteVideo_)
    {
        tryRunPanelsHideTimer();
        useAspect();
    }
    else
    {
        showPanelTimer_.stop();
        videoPanel_->startToolTipHideTimer();
        
        unuseAspect();
		fadeInPanels(kAnimationDefDuration);
    }
}

void Ui::VideoWindow::onPanelMouseLeave()
{
    tryRunPanelsHideTimer();
}

#ifndef _WIN32 // on win32 we have our own mouse event handler
void Ui::VideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    showPanelTimer_.stop();
    
	fadeInPanels(kAnimationDefDuration);
    
    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    tryRunPanelsHideTimer();
}

#ifndef __APPLE__
// Qt has bug with double click and fullscreen switching under mac.
void Ui::VideoWindow::mouseDoubleClickEvent(QMouseEvent* _e)
{
    QWidget::mouseDoubleClickEvent(_e);
    _switchFullscreen();
}
#endif

void Ui::VideoWindow::moveEvent(QMoveEvent * event)
{
    AspectRatioResizebleWnd::moveEvent(event);
    
    hideSecurityDialog();
}


void Ui::VideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    if (!resendMouseEventToPanel(_e))
    {
        showPanelTimer_.stop();
        
        fadeInPanels(kAnimationDefDuration);
        
        tryRunPanelsHideTimer();
    }
}

void Ui::VideoWindow::mouseReleaseEvent(QMouseEvent * event)
{
    if (!resendMouseEventToPanel(event))
    {
#ifdef __APPLE__
        // Own double click processing.
        auto prevTime = doubleClickTime_.elapsed();
        if (prevTime == 0 || prevTime > platform_macos::doubleClickInterval())
        {
            doubleClickTime_.start();
        }
        
        bool doubleClick = prevTime != 0 && prevTime < platform_macos::doubleClickInterval();
        
        if (doubleClick)
        {
            QWidget::mouseDoubleClickEvent(event);
            _switchFullscreen();
        }
        else
#endif
        {
            onSetContactPrimary();
            if (maskPanel_)
            {
                maskPanel_->hideMaskList();
            }
        }
    }
}

void Ui::VideoWindow::mousePressEvent(QMouseEvent * event)
{
    if (!resendMouseEventToPanel(event))
    {
        event->ignore();
    }
}

void Ui::VideoWindow::wheelEvent(QWheelEvent * event)
{
    if (!resendMouseEventToPanel(event))
    {
        event->ignore();
    }
}

template <typename E> bool Ui::VideoWindow::resendMouseEventToPanel(E* event_)
{
    bool res = false;
    for (auto panel : transporentPanels_)
    {
        if (panel->isVisible() && (panel->rect().contains(event_->pos()) || panel->isGrabMouse()))
        {
            QApplication::sendEvent(panel, event_);
            res = true;
            break;
        }
    }
    
    return res;
}

#endif

bool Ui::VideoWindow::isOverlapped() const
{
    std::vector<QWidget*> excludeWnd;

    for (auto panel : panels_)
    {
        excludeWnd.push_back(panel);
    }

    if (secureCallWnd_)
    {
        excludeWnd.push_back(secureCallWnd_);
    }

	return windowIsOverlapped(rootWidget_, excludeWnd);
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

	fadeOutPanels(kAnimationDefDuration);

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

#ifdef __APPLE__
    
    rootWidget_->clearPanels();
    std::vector<Ui::BaseVideoPanel*> panels;
    
    for (auto panel : panels_)
    {
        if (panel && panel->isVisible())
        {
            panels.push_back(panel);
        }
    }
    
    rootWidget_->addPanels(panels);
    
    for (auto panel : panels_)
    {
        if (panel && panel->isVisible())
        {
            panel->updatePosition(*this);
        }
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
	fadeInPanels(kAnimationDefDuration);

    tryRunPanelsHideTimer();

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
    const bool over   = _tapType == "over";
	const bool single = _tapType == "single";

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

			fadeInPanels(kAnimationDefDuration);
            
            tryRunPanelsHideTimer();
        }
		else if (single)
		{
			onSetContactPrimary();
			if (maskPanel_)
			{
				maskPanel_->hideMaskList();
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
		hidePanels();
        return;
    }

    videoPanel_->show();
    showPanel(maskPanel_.get());

#ifdef __APPLE__
    if (isInFullscreen())
    {
#else
    if (false)
    {
#endif
		hidePanel(topPanelSimple_.get());
        if (!outgoingNotAccepted_)
        {
            hidePanel(topPanelOutgoing_.get());
            maskPanel_->setTopOffset(Utils::scale_value(30));
        }
        else
        {
            maskPanel_->setTopOffset(Utils::scale_value(98));
        }
    }
    else
    {
        if (outgoingNotAccepted_)
        {
			hidePanel(topPanelSimple_.get());
			showPanel(topPanelOutgoing_.get());
            videoPanel_->talkFinished();
            maskPanel_->setTopOffset(Utils::scale_value(98));
        }
        else
        {
			showPanel(topPanelSimple_.get());
			hidePanel(topPanelOutgoing_.get());
            videoPanel_->talkStarted();
            maskPanel_->setTopOffset(Utils::scale_value(30));
        }
    }
        
    if (!!rootWidget_)
    {
        updatePanels();
    }
}

void Ui::VideoWindow::_switchFullscreen()
{
    switchFullscreen();
    checkPanelsVisibility();
    
    if (!!rootWidget_)
    {
        //rootWidget_->fullscreenModeChanged(isInFullscreen());
    }
    
	fadeInPanels(kAnimationDefDuration);
    
    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& _contacts)
{
    if(_contacts.empty())
    {
        return;
    }

    if (currentContacts_.empty())
    {
        currentContacts_ = _contacts;
    }

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
	if (_e->type() == QEvent::WindowStateChange)
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

	auto rc = rect();
	if (rc.width() >= Utils::scale_value(520))
	{
		// Vertical mode
		maskPanel_->setPanelMode(QBoxLayout::TopToBottom);
	}
	else
	{
		// Horizontal mode
		maskPanel_->setPanelMode(QBoxLayout::LeftToRight);
	}
    
    hideSecurityDialog();
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

        if (maskPanel_)
        {
            maskPanel_->callDestroyed();
        }

        currentContacts_.clear();
    }

    // Remove destroyed contact from list.
    currentContacts_.erase(std::remove(currentContacts_.begin(), currentContacts_.end(), _contactEx.contact),
        currentContacts_.end());
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
        secureCallWnd_ = new SecureCallWnd(this);
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
        updatePanels();
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
    
    tryRunPanelsHideTimer();
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

void Ui::VideoWindow::fadeInPanels(int kAnimationDefDuration)
{
	std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel) {
		panel->fadeIn(kAnimationDefDuration);
	});


	offsetWindow(videoPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset));
}

void Ui::VideoWindow::fadeOutPanels(int kAnimationDefDuration)
{
	std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel) {
		panel->fadeOut(kAnimationDefDuration);
	});

	offsetWindow(Utils::scale_value(kPreviewBorderOffset));
}

void Ui::VideoWindow::hidePanels()
{
	std::for_each(panels_.begin(), panels_.end(), [] (BaseVideoPanel* panel){
		panel->hide();
	});
}

void Ui::VideoWindow::onSetPreviewPrimary()
{
	Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), PREVIEW_RENDER_NAME);
}

void Ui::VideoWindow::onSetContactPrimary()
{
    if (!currentContacts_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), currentContacts_[0].contact.c_str());
    }
}

void Ui::VideoWindow::showPanel(QWidget* widget)
{
	if (widget)
	{
		widget->show();
	}
}

void Ui::VideoWindow::hidePanel(QWidget* widget)
{
	if (widget)
	{
		widget->hide();
	}
}

void Ui::VideoWindow::onShowMaskList()
{
    if (outgoingNotAccepted_)
    {
        maskPanel_->chooseFirstMask();
    }

    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);
}

void Ui::VideoWindow::onHideMaskList()
{
    tryRunPanelsHideTimer();
}

bool Ui::VideoWindow::isActiveWindow()
{
    bool bPanelIsActive = false;
    for (auto panel : panels_)
    {
        bPanelIsActive = bPanelIsActive || (panel && panel->isActiveWindow());
    }
    return AspectRatioResizebleWnd::isActiveWindow() || bPanelIsActive;
}

void Ui::VideoWindow::getCallStatus(bool& isAccepted)
{
    isAccepted = !outgoingNotAccepted_;
}

void Ui::VideoWindow::tryRunPanelsHideTimer()
{
    const bool haveSecurityWnd = secureCallWnd_ && secureCallWnd_->isVisible();
    if (hasRemoteVideo_ && !haveSecurityWnd && (maskPanel_ == nullptr || !maskPanel_->isOpened()))
    {
        showPanelTimer_.start();
    }
}

void Ui::VideoWindow::hideSecurityDialog()
{
    if (secureCallWnd_ && secureCallWnd_->isVisible())
    {
        secureCallWnd_->hide();
        onSecureCallWndClosed();
    }
}
