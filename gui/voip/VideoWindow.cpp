#include "stdafx.h"
#include "VideoWindow.h"
#include "DetachedVideoWnd.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../gui_settings.h"

#include "VideoFrame.h"
#include "VoipTools.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

extern std::string getFotmatedTime(unsigned ts);
namespace {
	enum { kPreviewBorderOffset = 12 };
    enum { kMinimumW = 328, kMinimumH = 164 };
    enum { kDefaultW = 640, kDefaultH = 480 };
    enum { kAnimationDefDuration = 500 };

    const std::string videoWndWName = "video_window_w";
    const std::string videoWndHName = "video_window_h";

    bool windowIsOverlapped(platform_specific::GraphicsPanel* window, quintptr* exclude, int size) {
        if (!window) {
            return false;
        }
        
#ifdef _WIN32
        HWND target = (HWND)window->frameId();
        if (!::IsWindowVisible(target)) {
            return false;
        }

        RECT r;
        ::GetWindowRect(target, &r);

        const int overlapDepthPts = 90;
        typedef std::vector<POINT> ptList;
        ptList pts;

        int ptsNumY = (r.bottom - r.top)/overlapDepthPts;
        int ptsNumX = (r.right - r.left)/overlapDepthPts;
        for( int j=0; j<ptsNumY; ++j ) 
        {
            for( int i=0; i<ptsNumX; ++i ) 
            {
                int ptX = r.left + overlapDepthPts*i;
                int ptY = r.top	 + overlapDepthPts*j;

                POINT pt = { ptX, ptY };
                pts.push_back(pt);
            }
        }

        int ptsCounter = 0;
        for (ptList::const_iterator it = pts.begin(); it != pts.end(); ++it) {
            const HWND top = ::WindowFromPoint( *it );

            bool isMyWnd = top == target;
            for (int i = 0; i < size; i++) {
                isMyWnd |= top == (HWND)exclude[i];
            }

            if (!isMyWnd) {
                ++ptsCounter;
            }
        }

        return (ptsCounter * 10) >= int(pts.size() * 4); // 40 % overlapping
#elif defined (__APPLE__)
        return platform_macos::windowIsOverlapped(window);
#endif
    }
}

Ui::video_window::ResizeEventFilter::ResizeEventFilter(std::vector<QWidget*>& top_panels, std::vector<QWidget*>& bottom_panels, QObject* parent) 
: QObject(parent) 
, _top_panels(top_panels)
, _bottom_panels(bottom_panels) {

}

bool Ui::video_window::ResizeEventFilter::eventFilter(QObject* obj, QEvent* e) {
//    qDebug() << "EVENT TYPE RECEIVED " << e->type() << " rc:{ " << rc.left() << ", " << rc.top() << " }";
    if (e->type() == QEvent::Resize || 
        e->type() == QEvent::Move || 
        e->type() == QEvent::WindowActivate || 
        e->type() == QEvent::NonClientAreaMouseButtonPress ||
        e->type() == QEvent::ZOrderChange ||
        e->type() == QEvent::ShowToParent ||
        e->type() == QEvent::WindowStateChange ||
        e->type() == QEvent::UpdateRequest) {

        QWidget* parent = qobject_cast<QWidget*>(obj);
        const QRect rc = parent ? parent->geometry() : QRect();

        bool needToRaise = parent->isActiveWindow();
        for (unsigned ix = 0; ix < _bottom_panels.size(); ix++) {
            QWidget* panel = _bottom_panels[ix];
            if (!panel) {
                continue;
            }

            needToRaise |= panel->isActiveWindow();
        }

        for (unsigned ix = 0; ix < _top_panels.size(); ix++) {
            QWidget* panel = _top_panels[ix];
            if (!panel) {
                continue;
            }

            needToRaise |= panel->isActiveWindow();
        }

        //qDebug() << "+ HANDLED " << e->type() << " rc:{ " << rc.left() << ", " << rc.top() << " }";
        for (unsigned ix = 0; ix < _bottom_panels.size(); ix++) {
            QWidget* panel = _bottom_panels[ix];
            if (!panel) {
                continue;
            }

#ifdef __APPLE__
            platform_macos::setWindowPosition(*panel, *parent, false);
#else
            panel->move(rc.x(), rc.y() + rc.height() - panel->rect().height());
            panel->setFixedWidth(rc.width());
#endif

            if (needToRaise)
                panel->raise();
        }

        for (unsigned ix = 0; ix < _top_panels.size(); ix++) {
            QWidget* panel = _top_panels[ix];
            if (!panel) {
                continue;
            }

#ifdef __APPLE__
            platform_macos::setWindowPosition(*panel, *parent, true);
#else
            panel->move(rc.x(), rc.y());
            panel->setFixedWidth(rc.width());
#endif

            if (needToRaise)
                panel->raise();
        }
    }
    return QObject::eventFilter(obj, e);
}


Ui::VideoWindow::VideoWindow()
: AspectRatioResizebleWnd()
, check_overlapped_timer_(this)
, show_panel_timer_(this)
, have_remote_video_(false)
, video_panel_header_with_avatars_(new voipTools::BoundBox<VoipSysPanelHeader>(this))
, detached_wnd_(new DetachedVideoWindow(this))
, video_panel_(new(std::nothrow) voipTools::BoundBox<VideoPanel>(this, this))
, video_panel_header_(new(std::nothrow) voipTools::BoundBox<VideoPanelHeader>(this, kVPH_ShowName | kVPH_ShowTime | kVPH_ShowMin | kVPH_ShowClose)) {

    callDescription.time = 0;

    setProperty("VideoWindowCommon", true);
    setProperty("VideoWindow", true);
    
#ifdef _WIN32
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
#else
    setWindowFlags(Qt::Window | Qt::WindowDoesNotAcceptFocus/*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif
    
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    
    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignVCenter);
    setLayout(root_layout);
    
    std::vector<QWidget*> panels;
    panels.push_back(video_panel_.get());
    panels.push_back(video_panel_header_.get());
    panels.push_back(video_panel_header_with_avatars_.get());
#ifndef __linux__
    _rootWidget = platform_specific::GraphicsPanel::create(this, panels);
    _rootWidget->setContentsMargins(0, 0, 0, 0);
    //_rootWidget->setProperty("WidgetWithBG", true);
    _rootWidget->setAttribute(Qt::WA_UpdatesDisabled);
    _rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(_rootWidget);
#endif //__linux__
    QIcon icon(":/resources/main_window/appicon.ico");
    setWindowIcon(icon);

	video_panel_effect_ = new UIEffects(*video_panel_.get());
	video_panel_header_effect_ = new UIEffects(*video_panel_header_.get());
    video_panel_header_effect_with_avatars_ = new UIEffects(*video_panel_header_with_avatars_.get());

    std::vector<QWidget*> topPanels;
    topPanels.push_back(video_panel_header_.get());
    topPanels.push_back(video_panel_header_with_avatars_.get());

    std::vector<QWidget*> bottomPanels;
    bottomPanels.push_back(video_panel_.get());

	event_filter_ = new video_window::ResizeEventFilter(topPanels, bottomPanels, this);
    installEventFilter(event_filter_);

    if (video_panel_) {
		video_panel_->setFullscreenMode(isInFullscreen());

        connect(video_panel_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
		connect(video_panel_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
		connect(video_panel_.get(), SIGNAL(onFullscreenClicked()), this, SLOT(onPanelFullscreenClicked()), Qt::QueuedConnection);
        connect(video_panel_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(_escPressed()), Qt::QueuedConnection);
    }

    if (!!video_panel_header_) {
        video_panel_header_->setFullscreenMode(isInFullscreen());

        connect(video_panel_header_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
		connect(video_panel_header_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(video_panel_header_.get(), SIGNAL(onClose()), this, SLOT(onPanelClickedClose()), Qt::QueuedConnection);
        connect(video_panel_header_.get(), SIGNAL(onMinimize()), this, SLOT(onPanelClickedMinimize()), Qt::QueuedConnection);
        connect(video_panel_header_.get(), SIGNAL(onMaximize()), this, SLOT(onPanelClickedMaximize()), Qt::QueuedConnection);
        connect(video_panel_header_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(_escPressed()), Qt::QueuedConnection);
    }

    if (!!video_panel_header_with_avatars_) {
        video_panel_header_with_avatars_->setTitle("");
        video_panel_header_with_avatars_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Outgoing call").toUtf8());

        connect(video_panel_header_with_avatars_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
		connect(video_panel_header_with_avatars_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(video_panel_header_with_avatars_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(_escPressed()), Qt::QueuedConnection);
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMouseTapped(quintptr,const std::string&)), this, SLOT(onVoipMouseTapped(quintptr,const std::string&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaRemoteVideo(bool)), this, SLOT(onVoipMediaRemoteVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
    
    connect(&check_overlapped_timer_, SIGNAL(timeout()), this, SLOT(_check_overlap()), Qt::QueuedConnection);
    check_overlapped_timer_.setInterval(1000);
    check_overlapped_timer_.start();

	connect(&show_panel_timer_, SIGNAL(timeout()), this, SLOT(_check_panels_vis()), Qt::QueuedConnection);

	show_panel_timer_.setInterval(1500);
    detached_wnd_->hideFrame();

    setMinimumSize(Utils::scale_value(kMinimumW), Utils::scale_value(kMinimumH));

    int storedW = Utils::scale_value(kDefaultW);
    int storedH = Utils::scale_value(kDefaultH);
    if (qt_gui_settings* settings = Ui::get_gui_settings()) {
        const int tmpW = settings->get_value<int>(videoWndWName.c_str(), -1);
        const int tmpH = settings->get_value<int>(videoWndHName.c_str(), -1);
        
        if (tmpW > 0 && tmpH > 0) {
            storedW = tmpW;
            storedH = tmpH;
        }
    }
    resize(storedW, storedH);
}

Ui::VideoWindow::~VideoWindow() {
    check_overlapped_timer_.stop();

	removeEventFilter(event_filter_);
	delete event_filter_;
}

quintptr Ui::VideoWindow::getContentWinId() {
    return _rootWidget->frameId();
}

void Ui::VideoWindow::onPanelMouseEnter() {
	show_panel_timer_.stop();
	video_panel_effect_->fadeIn(kAnimationDefDuration);
	video_panel_header_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);

	Ui::GetDispatcher()->getVoipController().setWindowOffsets(
		(quintptr)_rootWidget->frameId(),
		Utils::scale_value(kPreviewBorderOffset),
		Utils::scale_value(kPreviewBorderOffset), 
		Utils::scale_value(kPreviewBorderOffset), 
		video_panel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset)
		);
}

void Ui::VideoWindow::onVoipMediaRemoteVideo(bool enabled) {
    have_remote_video_ = enabled;

    _checkPanelsVisibility();
    if (have_remote_video_) {
        show_panel_timer_.start();
        useAspect();
    } else {
        unuseAspect();
        video_panel_effect_->fadeIn(kAnimationDefDuration);
        video_panel_header_effect_->fadeIn(kAnimationDefDuration);
        video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);

        Ui::GetDispatcher()->getVoipController().setWindowOffsets(
            (quintptr)_rootWidget->frameId(),
            Utils::scale_value(kPreviewBorderOffset),
            Utils::scale_value(kPreviewBorderOffset), 
            Utils::scale_value(kPreviewBorderOffset),
            video_panel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset)
            );
    }
}

void Ui::VideoWindow::onPanelMouseLeave() {
    if (have_remote_video_) {
        show_panel_timer_.start();
    }
}

#ifndef _WIN32 // on win32 we have our own mouse event handler
void Ui::VideoWindow::enterEvent(QEvent* e) {
    QWidget::enterEvent(e);
    show_panel_timer_.stop();
    
    video_panel_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);
    
    if (have_remote_video_) {
	    show_panel_timer_.start();
	}
}

void Ui::VideoWindow::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
    show_panel_timer_.start();
}

void Ui::VideoWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    QWidget::mouseDoubleClickEvent(e);
    _switchFullscreen();
}

void Ui::VideoWindow::mouseMoveEvent(QMouseEvent* e) {
    show_panel_timer_.stop();
    
    video_panel_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);
    
    Ui::GetDispatcher()->getVoipController().setWindowOffsets(
                                                              (quintptr)_rootWidget->frameId(),
                                                              Utils::scale_value(kPreviewBorderOffset),
                                                              Utils::scale_value(kPreviewBorderOffset),
                                                              Utils::scale_value(kPreviewBorderOffset),
                                                              video_panel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset)
                                                              );
    
    if (have_remote_video_) {
        show_panel_timer_.start();
    }
}

#endif

void Ui::VideoWindow::_check_overlap() {
    if (!isVisible()) {
        return;
    }
    
    assert(!!_rootWidget);
    if (!_rootWidget) {
        return;
    }

    quintptr friendlyWnds[] = { 
        !!video_panel_ ? video_panel_->winId() : NULL,
        !!video_panel_header_ ? video_panel_header_->winId() : NULL,
        !!video_panel_header_with_avatars_ ? video_panel_header_with_avatars_->winId() : NULL
    };
    if (have_remote_video_ && windowIsOverlapped(_rootWidget, friendlyWnds, sizeof(friendlyWnds) / sizeof(friendlyWnds[0])) && !detached_wnd_->closedManualy()) {
        QApplication::alert(this);
		detached_wnd_->showFrame();
    } else {
        detached_wnd_->hideFrame();
    }
}

void Ui::VideoWindow::_check_panels_vis() {
	show_panel_timer_.stop();
	video_panel_effect_->fadeOut(kAnimationDefDuration);
    video_panel_header_effect_->fadeOut(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeOut(kAnimationDefDuration);
	Ui::GetDispatcher()->getVoipController().setWindowOffsets(
		(quintptr)_rootWidget->frameId(),
		Utils::scale_value(kPreviewBorderOffset), 
		Utils::scale_value(kPreviewBorderOffset), 
		Utils::scale_value(kPreviewBorderOffset), 
		Utils::scale_value(kPreviewBorderOffset)
		);
}

void Ui::VideoWindow::onVoipCallTimeChanged(unsigned sec_elapsed, bool have_call) {
    if (video_panel_header_) {
        video_panel_header_->setTime(sec_elapsed, have_call);
        callDescription.time = have_call ? sec_elapsed : 0;
        updateWindowTitle_();
    }

    if (have_call) {
        video_panel_header_with_avatars_->setStatus(getFotmatedTime(sec_elapsed).c_str());
    } else {
        video_panel_header_with_avatars_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Outgoing call").toUtf8());
    }
}

void Ui::VideoWindow::onPanelClickedMinimize() {
    showMinimized();
}

void Ui::VideoWindow::onPanelClickedMaximize() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    video_panel_->setFullscreenMode(isInFullscreen());
    video_panel_header_->setFullscreenMode(isInFullscreen());
}

void Ui::VideoWindow::onPanelClickedClose() {
	Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::hideFrame() {
    Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)_rootWidget->frameId());
}

void Ui::VideoWindow::showFrame() {
    Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)_rootWidget->frameId(), true, false, video_panel_->geometry().height() + Utils::scale_value(5));
	Ui::GetDispatcher()->getVoipController().setWindowOffsets(
		(quintptr)_rootWidget->frameId(),
		Utils::scale_value(kPreviewBorderOffset),
		Utils::scale_value(kPreviewBorderOffset), 
		Utils::scale_value(kPreviewBorderOffset), 
		video_panel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset)
		);
}

void Ui::VideoWindow::onVoipWindowRemoveComplete(quintptr win_id) {
    if (win_id == _rootWidget->frameId()) {
        hide();
    }
}

void Ui::VideoWindow::updatePanels_() {
    if (!_rootWidget) {
        assert(false);
        return;
    }
    _rootWidget->clearPanels();
    
    std::vector<QWidget*> panels;
    if (video_panel_->isVisible()) {
        panels.push_back(video_panel_.get());
    }
    
    if (have_remote_video_) {
        if (video_panel_header_->isVisible()) {
            panels.push_back(video_panel_header_.get());
        }
    } else {
        if (video_panel_header_with_avatars_->isVisible()) {
            panels.push_back(video_panel_header_with_avatars_.get());
        }
    }
    
    //qDebug() << "+ UPDATE PANELS CALLED";
    _rootWidget->addPanels(panels);
    
#ifdef __APPLE__
    if (video_panel_->isVisible()) {
        platform_macos::setWindowPosition(*video_panel_.get(), *this, false);
    }

    if (video_panel_header_->isVisible()) {
        platform_macos::setWindowPosition(*video_panel_header_.get(), *this, true);
    }

    if (video_panel_header_with_avatars_->isVisible()) {
        platform_macos::setWindowPosition(*video_panel_header_with_avatars_.get(), *this, true);
    }
#endif
}

void Ui::VideoWindow::onVoipWindowAddComplete(quintptr win_id) {
    if (win_id == _rootWidget->frameId()) {
        updatePanels_();
        show();
    }
}

void Ui::VideoWindow::hideEvent(QHideEvent* ev) {
    AspectRatioResizebleWnd::hideEvent(ev);
    _checkPanelsVisibility(true);
    detached_wnd_->hideFrame();
}

void Ui::VideoWindow::showEvent(QShowEvent* ev) {
    AspectRatioResizebleWnd::showEvent(ev);
    _checkPanelsVisibility();

    show_panel_timer_.stop();
    video_panel_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);

    if (have_remote_video_) {
        show_panel_timer_.start();
    }

    detached_wnd_->hideFrame();

    showNormal();
	activateWindow();
    video_panel_->setFullscreenMode(isInFullscreen());
    video_panel_header_->setFullscreenMode(isInFullscreen());
}

void Ui::VideoWindow::onVoipMouseTapped(quintptr hwnd, const std::string& tap_type) {
	const bool dbl_tap = tap_type == "double";
	const bool over = tap_type == "over";

    if (detached_wnd_->get_video_frame_id() == hwnd) {
        if (dbl_tap) {
            raise();
        }
    } else if ((quintptr)_rootWidget->frameId() == hwnd) {
        if (dbl_tap) {
			_switchFullscreen();
        } else if (over) {
			show_panel_timer_.stop();

			video_panel_effect_->fadeIn(kAnimationDefDuration);
			video_panel_header_effect_->fadeIn(kAnimationDefDuration);
            video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);

			Ui::GetDispatcher()->getVoipController().setWindowOffsets(
				(quintptr)_rootWidget->frameId(),
				Utils::scale_value(kPreviewBorderOffset), 
				Utils::scale_value(kPreviewBorderOffset), 
				Utils::scale_value(kPreviewBorderOffset), 
				video_panel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset)
				);

            if (have_remote_video_) {
			    show_panel_timer_.start();
            }
        }
    }
}

void Ui::VideoWindow::onPanelFullscreenClicked() {
    _switchFullscreen();
}

void Ui::VideoWindow::_checkPanelsVisibility(bool forceHide /*= false*/) {
    video_panel_->setFullscreenMode(isInFullscreen());
    video_panel_header_->setFullscreenMode(isInFullscreen());

    if (isHidden() || isMinimized() || forceHide) {
        video_panel_->hide();
        video_panel_header_->hide();
        video_panel_header_with_avatars_->hide();
        return;
    }

    video_panel_->show();
#ifdef __APPLE__
    if (isInFullscreen()) {
#else
    if (false) {
#endif
        video_panel_header_->hide();
        video_panel_header_with_avatars_->hide();
    } else {
        if (have_remote_video_) {
            video_panel_header_->show();
            video_panel_header_with_avatars_->hide();
        } else {
            video_panel_header_->hide();
            video_panel_header_with_avatars_->show();
        }
    }
        
    if (!!_rootWidget) {
        _rootWidget->clearPanels();
        updatePanels_();
    }
}

void Ui::VideoWindow::_switchFullscreen() {
	switchFullscreen();
    _checkPanelsVisibility();
    
    if (!!_rootWidget) {
        _rootWidget->fullscreenModeChanged(isInFullscreen());
    }
    
    video_panel_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_->fadeIn(kAnimationDefDuration);
    video_panel_header_effect_with_avatars_->fadeIn(kAnimationDefDuration);
    
    if (have_remote_video_) {
        show_panel_timer_.start();
    }
}

void Ui::VideoWindow::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts) {
    if(contacts.empty()) {
        return;
    }

    std::vector<std::string> users;
    std::vector<std::string> friendly_names;
    for(unsigned ix = 0; ix < contacts.size(); ix++) {
        users.push_back(contacts[ix].contact);
        
        std::string n = Logic::GetContactListModel()->getDisplayName(contacts[ix].contact.c_str()).toUtf8().data();
        friendly_names.push_back(n);
    }

    video_panel_header_with_avatars_->setAvatars(users);

    auto name = voip_proxy::VoipController::formatCallName(friendly_names, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8());
    assert(!name.empty());

    video_panel_header_->setCallName(name);
    video_panel_header_with_avatars_->setTitle(name.c_str());

    callDescription.name = name;
    updateWindowTitle_();
}

void Ui::VideoWindow::updateWindowTitle_() {
    if (!!video_panel_header_) {
        std::stringstream fullName;
        fullName << "[ " << getFotmatedTime(callDescription.time) << " ]  " << callDescription.name;

        setWindowTitle(fullName.str().c_str());
    }
}

void Ui::VideoWindow::paintEvent(QPaintEvent *e) {
    return AspectRatioResizebleWnd::paintEvent(e);
}

void Ui::VideoWindow::changeEvent(QEvent* e) {
    AspectRatioResizebleWnd::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            video_panel_->blockSignals(true);
            video_panel_->raise();
            video_panel_->blockSignals(false);
            
            video_panel_header_with_avatars_->blockSignals(true);
            video_panel_header_with_avatars_->raise();
            video_panel_header_with_avatars_->blockSignals(false);
            
            video_panel_header_->blockSignals(true);
            video_panel_header_->raise();
            video_panel_header_->blockSignals(false);
        }
    }
}

void Ui::VideoWindow::closeEvent(QCloseEvent* e) {
    AspectRatioResizebleWnd::closeEvent(e);
	Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::resizeEvent(QResizeEvent* e) {
    AspectRatioResizebleWnd::resizeEvent(e);
    
    if (qt_gui_settings* settings = Ui::get_gui_settings()) {
        settings->set_value<int>(videoWndWName.c_str(), width());
        settings->set_value<int>(videoWndHName.c_str(), height());
    }

#ifdef _WIN32
    int border_width = Utils::scale_value(2);
    const auto fg = frameGeometry();
    const auto ge = geometry();

    border_width = std::min(ge.top() - fg.top(), 
                   std::min(fg.right() - ge.right(),
                   std::min(fg.bottom() - ge.bottom(),
                   std::min(ge.left() - fg.left(), 
                   border_width))));

    QRegion reg(-border_width, -border_width, ge.width() + 2*border_width, ge.height() + 2*border_width);
    setMask(reg);
#endif
}

void Ui::VideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex) {
    if (contact_ex.call_count <= 1) {
        unuseAspect();
        have_remote_video_ = false;
        _escPressed();

        video_panel_header_with_avatars_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Outgoing call").toUtf8());
    }
}

void Ui::VideoWindow::escPressed() {
    _escPressed();
}

void Ui::VideoWindow::_escPressed() {
    if (isInFullscreen()) {
        _switchFullscreen();
    }
}
