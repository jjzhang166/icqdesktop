
#include "stdafx.h"
#include "CommonUI.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"


Ui::ResizeEventFilter::ResizeEventFilter(
    std::vector<QWidget*>& _topPanels,
    std::vector<QWidget*>& _bottomPanels,
	ShadowWindow* shadow,
    QObject* _parent) 
    : QObject(_parent) 
    , topPanels_(_topPanels)
    , bottomPanels_(_bottomPanels)
	, shadow_(shadow)
{

}

bool Ui::ResizeEventFilter::eventFilter(QObject* _obj, QEvent* _e)
{
//    qDebug() << "EVENT TYPE RECEIVED " << e->type() << " rc:{ " << rc.left() << ", " << rc.top() << " }";
    if (_e->type() == QEvent::Resize || 
        _e->type() == QEvent::Move || 
        _e->type() == QEvent::WindowActivate || 
        _e->type() == QEvent::NonClientAreaMouseButtonPress ||
        _e->type() == QEvent::ZOrderChange ||
        _e->type() == QEvent::ShowToParent ||
        _e->type() == QEvent::WindowStateChange ||
        _e->type() == QEvent::UpdateRequest) {

        QWidget* parent = qobject_cast<QWidget*>(_obj);
        const QRect rc = parent ? parent->geometry() : QRect();

		bool bActive = parent->isActiveWindow();

         //qDebug() << "+ HANDLED " << e->type() << " rc:{ " << rc.left() << ", " << rc.top() << " }";
        for (unsigned ix = 0; ix < bottomPanels_.size(); ix++)
        {
            QWidget* panel = bottomPanels_[ix];
            if (!panel)
            {
                continue;
            }

			bActive = bActive || panel->isActiveWindow();

#ifdef __APPLE__
            // We disable setWindowPosition for fullscreen,
            // because it broke fullscreen animation under mac.
            if (!parent->isFullScreen())
            {
                platform_macos::setWindowPosition(*panel, *parent, false);
            }
#else
            panel->move(rc.x(), rc.y() + rc.height() - panel->rect().height());
            panel->setFixedWidth(rc.width());
#endif

            //if (needToRaise)
            //    panel->raise();
        }

        for (unsigned ix = 0; ix < topPanels_.size(); ix++)
        {
            QWidget* panel = topPanels_[ix];
            if (!panel) {
                continue;
            }

			bActive = bActive || panel->isActiveWindow();

#ifdef __APPLE__
            // We disable setWindowPosition for fullscreen,
            // because it broke fullscreen animation under mac.
            if (!parent->isFullScreen())
            {
                platform_macos::setWindowPosition(*panel, *parent, true);
            }
#else
            panel->move(rc.x(), rc.y());
            panel->setFixedWidth(rc.width());
#endif

            //if (needToRaise)
            //    panel->raise();
        }

		//QPoint pos = mapToGlobal(QPoint(rect().x(), rect().y()));
		if (shadow_)
		{
			int shadowWidth = get_gui_settings()->get_shadow_width();
			shadow_->move(rc.topLeft().x() - shadowWidth, rc.topLeft().y() - shadowWidth);
			shadow_->resize(rc.width() + 2 * shadowWidth, rc.height() + 2 * shadowWidth);

			shadow_->setActive(bActive);

#ifdef _WIN32
			SetWindowPos((HWND)shadow_->winId(), (HWND)parent->winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE);
#endif
		}
    }
    return QObject::eventFilter(_obj, _e);
}


Ui::ShadowWindowParent::ShadowWindowParent(QWidget* parent) : shadow_(nullptr)
{
	if (platform::is_windows())
	{
		int shadowWidth = get_gui_settings()->get_shadow_width();

		QBrush b(Qt::transparent);
		QMatrix m;
		m.translate(shadowWidth, shadowWidth);
		b.setMatrix(m);

		shadow_ = new ShadowWindow(b, shadowWidth);

		QPoint pos = parent->mapToGlobal(QPoint(parent->rect().x(), parent->rect().y()));
		shadow_->move(pos.x() - shadowWidth, pos.y() - shadowWidth);
		shadow_->resize(parent->rect().width() + 2 * shadowWidth, parent->rect().height() + 2 * shadowWidth);
	}
}

void Ui::ShadowWindowParent::showShadow()
{
	if (shadow_)
	{
		shadow_->show();
	}
}

void Ui::ShadowWindowParent::hideShadow()
{
	if (shadow_)
	{
		shadow_->hide();
	}
}

Ui::ShadowWindow* Ui::ShadowWindowParent::getShadowWidget()
{
	return shadow_;
}

void Ui::ShadowWindowParent::setActive(bool _value)
{
	if (shadow_)
	{
		shadow_->setActive(_value);
	}
}

Ui::AspectRatioResizebleWnd::AspectRatioResizebleWnd()
    : QWidget(NULL)
    , firstTimeUseAspectRatio_(true)
    , useAspect_(true)
    , aspectRatio_(0.0f)
{
    selfResizeEffect_ = new UIEffects(*this);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), this, SLOT(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);
}

Ui::AspectRatioResizebleWnd::~AspectRatioResizebleWnd()
{
    
}

bool Ui::AspectRatioResizebleWnd::isInFullscreen() const
{
    return isFullScreen();
}

void Ui::AspectRatioResizebleWnd::switchFullscreen()
{
    if (!isFullScreen())
    {
        showFullScreen();
    }
    else
    {
        showNormal();

        if (aspectRatio_ > 0.001f && selfResizeEffect_)
        {
            const QRect rc = rect();
            const QPoint p = mapToGlobal(rc.topLeft());
            QRect endRc(p.x(), p.y(), rc.width(), rc.width() / aspectRatio_);

            selfResizeEffect_->geometryTo(endRc, 500);
        }
    }
}

void Ui::AspectRatioResizebleWnd::onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs)
{
    if ((quintptr)_fs.hwnd == getContentWinId() && fabs(_fs.aspect_ratio - aspectRatio_) > 0.001f)
    {
        const float wasAr = aspectRatio_;
        aspectRatio_ = _fs.aspect_ratio;
        applyFrameAspectRatio(wasAr);
        
#ifdef __APPLE__
        platform_macos::setAspectRatioForWindow(*this, aspectRatio_);
#endif
    }
}

void Ui::AspectRatioResizebleWnd::applyFrameAspectRatio(float _wasAr)
{
    if (useAspect_ && aspectRatio_ > 0.001f && selfResizeEffect_ && !isFullScreen())
    {
        QRect rc = rect();

#ifndef __APPLE__
        const QPoint p = mapToGlobal(rc.topLeft());
#else
        // On Mac we have wrong coords with mapToGlobal. Maybe because we attach own view to widnow.
        const QPoint p(x(), y());
#endif
        
        QRect endRc;
        if (_wasAr > 0.001f && fabs((1.0f / aspectRatio_) - _wasAr) < 0.0001f)
        {
            endRc = QRect(p.x(), p.y(), rc.height(), rc.width());
        }
        else
        {
            endRc = QRect(p.x(), p.y(), rc.width(), rc.width() / aspectRatio_);
        }

        const QSize minSize = minimumSize();
        if (endRc.width() < minSize.width())
        {
            const int w = minSize.width();
            const int h = w / aspectRatio_;
            endRc.setRight(endRc.left() + w);
            endRc.setBottom(endRc.top() + h);
        }
        if (endRc.height() < minSize.height())
        {
            const int h = minSize.height();
            const int w = h * aspectRatio_;
            endRc.setRight(endRc.left() + w);
            endRc.setBottom(endRc.top() + h);
        }

        QDesktopWidget dw;
        const auto screenRect = dw.availableGeometry(dw.primaryScreen());

        if (endRc.right() > screenRect.right())
        {
            const int w = endRc.width();
            endRc.setRight(screenRect.right());
            endRc.setLeft(endRc.right() - w);
        }

        if (endRc.bottom() > screenRect.bottom())
        {
            const int h = endRc.height();
            endRc.setBottom(screenRect.bottom());
            endRc.setTop(endRc.bottom() - h);
        }

        if (screenRect.width() < endRc.width())
        {
            endRc.setLeft(screenRect.left());
            endRc.setRight(screenRect.right());

            const int h = endRc.width() / aspectRatio_;
            endRc.setTop(endRc.bottom() - h);
        }

        if (screenRect.height() < endRc.height())
        {
            endRc.setTop(screenRect.top());
            endRc.setBottom(screenRect.bottom());

            const int w = endRc.height() * aspectRatio_;
            endRc.setLeft(endRc.right() - w);
        }

        if (firstTimeUseAspectRatio_)
        {
            {
                const int bestW = 0.6f * screenRect.width();
                if (endRc.width() > bestW)
                {
                    const int bestH = bestW / aspectRatio_;
                    endRc.setLeft((screenRect.x() + screenRect.width() - bestW) / 2);
                    endRc.setRight(endRc.left() + bestW);

                    endRc.setTop((screenRect.y() + screenRect.height() - bestH) / 2);
                    endRc.setBottom(endRc.top() + bestH);
                }
            }
            {/* NEED TO EXECUTE 2 TIMES, BECAUSE CALC FOR BEST W NOT MEANS USING BEST H*/
                const int bestH = 0.8f * screenRect.height();
                if (endRc.height() > bestH)
                {
                    const int bestW = bestH * aspectRatio_;
                    endRc.setLeft((screenRect.x() + screenRect.width() - bestW) / 2);
                    endRc.setRight(endRc.left() + bestW);

                    endRc.setTop((screenRect.y() + screenRect.height() - bestH) / 2);
                    endRc.setBottom(endRc.top() + bestH);
                }
            }
            firstTimeUseAspectRatio_ = false;
        }

        selfResizeEffect_->geometryTo(endRc, 500);
    }
}

void Ui::AspectRatioResizebleWnd::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        escPressed();
    }
}

void Ui::AspectRatioResizebleWnd::useAspect()
{
    useAspect_ = true;
    applyFrameAspectRatio(0.0f);
}

void Ui::AspectRatioResizebleWnd::unuseAspect()
{
    useAspect_ = false;
}

#ifdef _WIN32
bool Ui::AspectRatioResizebleWnd::onWMSizing(RECT& _rc, unsigned _wParam)
{
    const int cw = _rc.right - _rc.left;
    const int ch = _rc.bottom - _rc.top;

    if (!useAspect_ || aspectRatio_ < 0.001f)
    {
        return false;
    }

    switch(_wParam)
    {
    case WMSZ_TOP:
    case WMSZ_BOTTOM:
        {
            int w = ch * aspectRatio_;
            if (w >= minimumWidth())
            {
                _rc.right = _rc.left + w;
            }
            else
            {
                _rc.right = _rc.left + minimumWidth();

                if (_wParam == WMSZ_BOTTOM)
                    _rc.bottom = _rc.top + minimumWidth() / aspectRatio_;
                else
                    _rc.top = _rc.bottom - minimumWidth() / aspectRatio_;
            }
        }
        break;

    case WMSZ_LEFT:
    case WMSZ_RIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.bottom = _rc.top + h;
        }
        break;

    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.top = _rc.bottom - h;
        }
        break;

    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.bottom = _rc.top + h;
        }
        break;

    default: return false;
    }

    return true;
}
#endif

bool Ui::AspectRatioResizebleWnd::nativeEvent(const QByteArray&, void* _message, long* _result)
{
#ifdef _WIN32
    MSG* msg = reinterpret_cast<MSG*>(_message);
    if (isVisible() && msg->hwnd == (HWND)winId())
    {
        if (msg->message == WM_SIZING)
        {
            if (aspectRatio_ < 0.001f)
            {
                return false;
            }

            RECT* rc = (RECT*)msg->lParam;
            if (!rc)
            {
                return false;
            }

            *_result = TRUE;
            return onWMSizing(*rc, msg->wParam);
        }/* else if (msg->_message == WM_WINDOWPOSCHANGING) {
            if (aspectRatio_ < 0.001f) {
                return false;
            }

            WINDOWPOS* wp = (WINDOWPOS*)msg->lParam;
            if (!wp) {
                return false;
            }

            *_result = 0;
            return _onWindowPosChanging(*wp);
        }*/
    }
#else
#endif
    return false;
}

void Ui::AspectRatioResizebleWnd::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count == 1)
    {
        firstTimeUseAspectRatio_ = true;
    }
}

Ui::UIEffects::UIEffects(QWidget& _obj)
    : fadeEffect_(new QGraphicsOpacityEffect(&_obj))
    , fadedIn_(true)
    , obj_(_obj)
    , resizeAnimation_(new QPropertyAnimation(&_obj, "geometry"))
    , animation_(new QPropertyAnimation(fadeEffect_, "opacity"))
{
    _obj.setGraphicsEffect(fadeEffect_);
    animation_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimation_->setEasingCurve(QEasingCurve::InOutQuad);
}

Ui::UIEffects::~UIEffects()
{
    delete animation_;
}

void Ui::UIEffects::geometryTo(const QRect& _rc, unsigned _interval)
{
    if (resizeAnimation_)
    {
        resizeAnimation_->stop();
        resizeAnimation_->setDuration(_interval);
        resizeAnimation_->setEndValue(_rc);
        resizeAnimation_->start();
    }
}

void Ui::UIEffects::fadeOut(unsigned _interval)
{
#ifdef __APPLE__
    platform_macos::fadeOut(&obj_);
#else
    if (1)
    {//(animation_ && fadedIn_) {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(0.01);
        animation_->start();
        fadedIn_ = false;
    }
#endif
}

void Ui::UIEffects::fadeIn(unsigned _interval)
{
#ifdef __APPLE__
    platform_macos::fadeIn(&obj_);
#else
    if (1)
    {// //if (animation_ && !fadedIn_) {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(1.0);
        animation_->start();
        fadedIn_ = true;
    }
#endif
}
