#include "stdafx.h"
#include "DetachedVideoWnd.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../utils/utils.h"
#include "VideoPanelHeader.h"
#include "VideoWindow.h"

#ifdef _WIN32
    #include "QDesktopWidget"
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

Ui::AspectRatioResizebleWnd::AspectRatioResizebleWnd()
: QWidget(NULL)
, _first_time_use_aspect_ratio(true)
, _use_aspect(true)
, _aspect_ratio(0.0f) {
    _self_resize_effect = new UIEffects(*this);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), this, SLOT(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);
}

Ui::AspectRatioResizebleWnd::~AspectRatioResizebleWnd() {
    
}

bool Ui::AspectRatioResizebleWnd::isInFullscreen() const {
    return isFullScreen();
}

void Ui::AspectRatioResizebleWnd::switchFullscreen() {
	if (!isFullScreen()) {
		showFullScreen();
	} else {
		showNormal();

        if (_aspect_ratio > 0.001f && _self_resize_effect) {
            const QRect rc = rect();
            const QPoint p = mapToGlobal(rc.topLeft());
            QRect end_rc(p.x(), p.y(), rc.width(), rc.width() / _aspect_ratio);

            _self_resize_effect->geometryTo(end_rc, 500);
        }
	}
}

void Ui::AspectRatioResizebleWnd::onVoipFrameSizeChanged(const voip_manager::FrameSize& fs) {
    if ((quintptr)fs.hwnd == getContentWinId() && fabs(fs.aspect_ratio - _aspect_ratio) > 0.001f) {
        const float was_ar = _aspect_ratio;
        _aspect_ratio = fs.aspect_ratio;
        _applyFrameAspectRatio(was_ar);
        
#ifdef __APPLE__
        platform_macos::setAspectRatioForWindow(*this, _aspect_ratio);
#endif
    }
}

void Ui::AspectRatioResizebleWnd::_applyFrameAspectRatio(float was_ar) {
    if (_use_aspect && _aspect_ratio > 0.001f && _self_resize_effect && !isFullScreen()) {
        const QRect rc = rect();
        const QPoint p = mapToGlobal(rc.topLeft());

        QRect end_rc;
        if (was_ar > 0.001f && fabs((1.0f / _aspect_ratio) - was_ar) < 0.0001f) {
            end_rc = QRect(p.x(), p.y(), rc.height(), rc.width());
        } else {
            end_rc = QRect(p.x(), p.y(), rc.width(), rc.width() / _aspect_ratio);
        }

        const QSize minimum_size = minimumSize();
        if (end_rc.width() < minimum_size.width()) {
            const int w = minimum_size.width();
            const int h = w / _aspect_ratio;
            end_rc.setRight(end_rc.left() + w);
            end_rc.setBottom(end_rc.top() + h);
        }
        if (end_rc.height() < minimum_size.height()) {
            const int h = minimum_size.height();
            const int w = h * _aspect_ratio;
            end_rc.setRight(end_rc.left() + w);
            end_rc.setBottom(end_rc.top() + h);
        }

        QDesktopWidget dw;
        const auto screen_rect = dw.availableGeometry(dw.primaryScreen());

        if (end_rc.right() > screen_rect.right()) {
            const int w = end_rc.width();
            end_rc.setRight(screen_rect.right());
            end_rc.setLeft(end_rc.right() - w);
        }

        if (end_rc.bottom() > screen_rect.bottom()) {
            const int h = end_rc.height();
            end_rc.setBottom(screen_rect.bottom());
            end_rc.setTop(end_rc.bottom() - h);
        }

        if (screen_rect.width() < end_rc.width()) {
            end_rc.setLeft(screen_rect.left());
            end_rc.setRight(screen_rect.right());

            const int h = end_rc.width() / _aspect_ratio;
            end_rc.setTop(end_rc.bottom() - h);
        }

        if (screen_rect.height() < end_rc.height()) {
            end_rc.setTop(screen_rect.top());
            end_rc.setBottom(screen_rect.bottom());

            const int w = end_rc.height() * _aspect_ratio;
            end_rc.setLeft(end_rc.right() - w);
        }

        if (_first_time_use_aspect_ratio) {
            {
                const int best_w = 0.6f * screen_rect.width();
                if (end_rc.width() > best_w) {
                    const int best_h = best_w / _aspect_ratio;
                    end_rc.setLeft((screen_rect.x() + screen_rect.width() - best_w) / 2);
                    end_rc.setRight(end_rc.left() + best_w);

                    end_rc.setTop((screen_rect.y() + screen_rect.height() - best_h) / 2);
                    end_rc.setBottom(end_rc.top() + best_h);
                }
            }
            {/* NEED TO EXECUTE 2 TIMES, BECAUSE CALC FOR BEST W NOT MEANS USING BEST H*/
                const int best_h = 0.8f * screen_rect.height();
                if (end_rc.height() > best_h) {
                    const int best_w = best_h * _aspect_ratio;
                    end_rc.setLeft((screen_rect.x() + screen_rect.width() - best_w) / 2);
                    end_rc.setRight(end_rc.left() + best_w);

                    end_rc.setTop((screen_rect.y() + screen_rect.height() - best_h) / 2);
                    end_rc.setBottom(end_rc.top() + best_h);
                }
            }
            _first_time_use_aspect_ratio = false;
        }

        _self_resize_effect->geometryTo(end_rc, 500);
    }
}

void Ui::AspectRatioResizebleWnd::keyReleaseEvent(QKeyEvent* e) {
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        escPressed();
    }
}

void Ui::AspectRatioResizebleWnd::useAspect() {
    _use_aspect = true;
    _applyFrameAspectRatio(0.0f);
}

void Ui::AspectRatioResizebleWnd::unuseAspect() {
    _use_aspect = false;
}

#ifdef _WIN32
bool Ui::AspectRatioResizebleWnd::_onWMSizing(RECT& rc, unsigned wParam) {
    const int cw = rc.right - rc.left;
    const int ch = rc.bottom - rc.top;

    if (!_use_aspect || _aspect_ratio < 0.001f) {
        return false;
    }

    switch(wParam) {
    case WMSZ_TOP:
    case WMSZ_BOTTOM:
        {
            int w = ch * _aspect_ratio;
            if (w >= minimumWidth()) {
                rc.right = rc.left + w;
            } else {
                rc.right = rc.left + minimumWidth();

                if (wParam == WMSZ_BOTTOM)
                    rc.bottom = rc.top + minimumWidth() / _aspect_ratio;
                else
                    rc.top = rc.bottom - minimumWidth() / _aspect_ratio;
            }
        }
        break;

    case WMSZ_LEFT:
    case WMSZ_RIGHT:
        {
            int h = cw / _aspect_ratio;
            rc.bottom = rc.top + h;
        }
        break;

    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        {
            int h = cw / _aspect_ratio;
            rc.top = rc.bottom - h;
        }
        break;

    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        {
            int h = cw / _aspect_ratio;
            rc.bottom = rc.top + h;
        }
        break;

    default: return false;
    }

    return true;
}
#endif

bool Ui::AspectRatioResizebleWnd::nativeEvent(const QByteArray&, void *message, long *result) {
#ifdef _WIN32
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (isVisible() && msg->hwnd == (HWND)winId()) {
        if (msg->message == WM_SIZING) {
            if (_aspect_ratio < 0.001f) {
                return false;
            }

            RECT* rc = (RECT*)msg->lParam;
            if (!rc) {
                return false;
            }

            *result = TRUE;
            return _onWMSizing(*rc, msg->wParam);
        }/* else if (msg->message == WM_WINDOWPOSCHANGING) {
            if (_aspect_ratio < 0.001f) {
                return false;
            }

            WINDOWPOS* wp = (WINDOWPOS*)msg->lParam;
            if (!wp) {
                return false;
            }

            *result = 0;
            return _onWindowPosChanging(*wp);
        }*/
    }
#else
#endif
    return false;
}

void Ui::AspectRatioResizebleWnd::onVoipCallCreated(const voip_manager::ContactEx& contact_ex) {
    if (contact_ex.call_count == 1) {
        _first_time_use_aspect_ratio = true;
    }
}

Ui::UIEffects::UIEffects(QWidget& obj)
: _fade_effect(new QGraphicsOpacityEffect(&obj))
, _fadedIn(true)
, _obj(obj)
, _resize_animation(new QPropertyAnimation(&obj, "geometry"))
, _animation(new QPropertyAnimation(_fade_effect, "opacity")) {
	obj.setGraphicsEffect(_fade_effect);
	_animation->setEasingCurve(QEasingCurve::InOutQuad);
    _resize_animation->setEasingCurve(QEasingCurve::InOutQuad);
}

Ui::UIEffects::~UIEffects() {
	delete _animation;
}

void Ui::UIEffects::geometryTo(const QRect& rc, unsigned interval) {
    if (_resize_animation) {
        _resize_animation->stop();
        _resize_animation->setDuration(interval);
        _resize_animation->setEndValue(rc);
        _resize_animation->start();
    }
}

void Ui::UIEffects::fadeOut(unsigned interval) {
#ifdef __APPLE__
    platform_macos::fadeOut(&_obj);
#else
	if (1) {//(_animation && _fadedIn) {
		_animation->stop();
		_animation->setDuration(interval);
		_animation->setEndValue(0.01);
		_animation->start();
		_fadedIn = false;
	}
#endif
}

void Ui::UIEffects::fadeIn(unsigned interval) {
#ifdef __APPLE__
    platform_macos::fadeIn(&_obj);
#else
	if (1) {// //if (_animation && !_fadedIn) {
		_animation->stop();
		_animation->setDuration(interval);
		_animation->setEndValue(1.0);
		_animation->start();
		_fadedIn = true;
	}
#endif
}

#define SHOW_HEADER_PANEL false

Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* parent)
    : AspectRatioResizebleWnd()
    , parent_(parent)
	, closed_manualy_(false)
	, show_panel_timer_(this)
    , video_panel_header_effect_(NULL) {

        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("detachedVideoWnd"));
        this->resize(400, 300);
        this->setMinimumSize(QSize(0, 0));
        this->setMaximumSize(QSize(16777215, 16777215));
        this->setProperty("DetachedVideoWindowCommon", QVariant(true));
        horizontal_layout_ = new QHBoxLayout(this);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setAlignment(Qt::AlignVCenter);
        QMetaObject::connectSlotsByName(this);
        
#ifdef _WIN32
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::SubWindow);
#else
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window | Qt::WindowDoesNotAcceptFocus/*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif

        if (SHOW_HEADER_PANEL) {
            video_panel_header_.reset(new VideoPanelHeader(this, kVPH_ShowNone));
        }
	    
        if (!!video_panel_header_) {
            video_panel_header_effect_ = new UIEffects(*video_panel_header_.get());
        }
#ifndef __linux__
        std::vector<QWidget*> panels;
        if (!!video_panel_header_) {
            panels.push_back(video_panel_header_.get());
        }
        _rootWidget = platform_specific::GraphicsPanel::create(this, panels);
        _rootWidget->setContentsMargins(0, 0, 0, 0);
        //_rootWidget->setProperty("WidgetWithBG", true);
        _rootWidget->setAttribute(Qt::WA_UpdatesDisabled);
        _rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        layout()->addWidget(_rootWidget);
#endif //__linux__

        std::vector<QWidget*> topPanels;
        if (!!video_panel_header_) {
            topPanels.push_back(video_panel_header_.get());
        }
        std::vector<QWidget*> bottomPanels;
        event_filter_ = new video_window::ResizeEventFilter(topPanels, bottomPanels, this);
		installEventFilter(event_filter_);

        setAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_ShowWithoutActivating);

		connect(&show_panel_timer_, SIGNAL(timeout()), this, SLOT(_check_panels_vis()), Qt::QueuedConnection);
		show_panel_timer_.setInterval(1500);

		if (!!video_panel_header_) {
            video_panel_header_->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            video_panel_header_->setAttribute(Qt::WA_NoSystemBackground, true);
            video_panel_header_->setAttribute(Qt::WA_TranslucentBackground, true);  

			connect(video_panel_header_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
			connect(video_panel_header_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);

			connect(video_panel_header_.get(), SIGNAL(onClose()), this, SLOT(onPanelClickedClose()), Qt::QueuedConnection);
			connect(video_panel_header_.get(), SIGNAL(onMinimize()), this, SLOT(onPanelClickedMinimize()), Qt::QueuedConnection);
			connect(video_panel_header_.get(), SIGNAL(onMaximize()), this, SLOT(onPanelClickedMaximize()), Qt::QueuedConnection);
		}

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);

        setMinimumSize(Utils::scale_value(320), Utils::scale_value(80));
        
        QDesktopWidget dw;
        const auto screen_rect = dw.screenGeometry(dw.primaryScreen());
        const auto detached_wnd_rect = rect();

        auto detached_wnd_pos = screen_rect.topRight();
        detached_wnd_pos.setX(detached_wnd_pos.x() - detached_wnd_rect.width() - 0.01f * screen_rect.width());
        detached_wnd_pos.setY(detached_wnd_pos.y() + 0.05f * screen_rect.height());

        const QRect rc(detached_wnd_pos.x(), detached_wnd_pos.y(), detached_wnd_rect.width(), detached_wnd_rect.height());
        setGeometry(rc);
}

void Ui::DetachedVideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex) {
    if (contact_ex.call_count <= 1) { // in this moment destroyed call is active, e.a. call_count + 1
    	closed_manualy_ = false;
    }
}

void Ui::DetachedVideoWindow::onPanelClickedClose() {
	closed_manualy_ = true;
	hide();
}
void Ui::DetachedVideoWindow::onPanelClickedMinimize() {
	//showMinimized();
}
void Ui::DetachedVideoWindow::onPanelClickedMaximize() {
	//if (_parent) {
	//	_parent->showNormal();
	//	_parent->activateWindow();
	//	hide();
	//}
}

void Ui::DetachedVideoWindow::onPanelMouseEnter() {
	show_panel_timer_.stop();
    if (video_panel_header_effect_) {
	    video_panel_header_effect_->fadeIn(500);
    }
}

void Ui::DetachedVideoWindow::onPanelMouseLeave() {
	const QPoint p = mapFromGlobal(QCursor::pos());
	if (!rect().contains(p)) {
		show_panel_timer_.start();
	}
}

void Ui::DetachedVideoWindow::mousePressEvent(QMouseEvent* e) {
    pos_drag_begin_ = e->pos();
}

void Ui::DetachedVideoWindow::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        QPoint diff = e->pos() - pos_drag_begin_;
        QPoint newpos = this->pos() + diff;

        this->move(newpos);
    }
}

void Ui::DetachedVideoWindow::enterEvent(QEvent* e) {
	QWidget::enterEvent(e);
	show_panel_timer_.stop();
    if (video_panel_header_effect_) {
    	video_panel_header_effect_->fadeIn(500);
    }
}

void Ui::DetachedVideoWindow::leaveEvent(QEvent* e) {
	QWidget::leaveEvent(e);
	show_panel_timer_.start();
}

void Ui::DetachedVideoWindow::mouseDoubleClickEvent(QMouseEvent* /*e*/) {
	if (parent_) {
        if (parent_->isMinimized()) {
            parent_->showNormal();
        }
		parent_->raise();
		parent_->activateWindow();
		//hide();
	}
}

quintptr Ui::DetachedVideoWindow::getContentWinId() {
    return (quintptr)_rootWidget->frameId();
}

void Ui::DetachedVideoWindow::onVoipWindowRemoveComplete(quintptr win_id) {
    if (win_id == _rootWidget->frameId()) {
        hide();
    }
}

void Ui::DetachedVideoWindow::onVoipWindowAddComplete(quintptr win_id) {
    if (win_id == _rootWidget->frameId()) {
        showNormal();
    }
}

void Ui::DetachedVideoWindow::showFrame() {
    assert(_rootWidget->frameId());
    if (_rootWidget->frameId()) {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)_rootWidget->frameId(), false, false, 0);
    }
}

void Ui::DetachedVideoWindow::hideFrame() {
    assert(_rootWidget->frameId());
    if (_rootWidget->frameId()) {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)_rootWidget->frameId());
    }
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* e) {
    if (!!video_panel_header_) {
	    video_panel_header_->show();
    }
    QWidget::showEvent(e);
	show_panel_timer_.stop();

    if (video_panel_header_effect_) {
        video_panel_header_effect_->fadeIn(500);
    }
	show_panel_timer_.start();
}

bool Ui::DetachedVideoWindow::closedManualy() {
	return closed_manualy_;
}

void Ui::DetachedVideoWindow::hideEvent(QHideEvent* e) {
    if (!!video_panel_header_) {
	    video_panel_header_->hide();
    }
    QWidget::hideEvent(e);
}

quintptr Ui::DetachedVideoWindow::get_video_frame_id() const {
    return (quintptr)_rootWidget->frameId();
}

Ui::DetachedVideoWindow::~DetachedVideoWindow() {
	removeEventFilter(event_filter_);
	delete event_filter_;
}

void Ui::DetachedVideoWindow::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
}

void Ui::DetachedVideoWindow::_check_panels_vis() {
	show_panel_timer_.stop();
    if (video_panel_header_effect_) {
    	video_panel_header_effect_->fadeOut(500);
    }
}
