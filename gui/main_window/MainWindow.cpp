#include "stdafx.h"
#include "MainWindow.h"
#include "MainPage.h"
#include "LoginPage.h"
#include "tray/TrayIcon.h"
#include "contact_list/RecentsModel.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../theme_settings.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../previewer/Previewer.h"
#include "../controls/Alert.h"
#include "../controls/BackgroundWidget.h"
#include "../cache/stickers/stickers.h"
#include "sounds/SoundsManager.h"

#include "contact_list/ContactListModel.h"
#include "history_control/MessagesModel.h"
#include "history_control/HistoryControlPage.h"
#include "../../common.shared/crash_handler.h"
#include "history_control/MessagesScrollArea.h"

#include "ContactDialog.h"

#ifdef _WIN32
#include <windowsx.h>
#endif //_WIN32

#ifdef __APPLE__
#include "AccountsPage.h"
#include "mac_support.h"
#include "mac_migration.h"
#endif //__APPLE__

namespace
{
	const int SIZE_BOX_WIDTH = 4;
	enum PagesIndex
	{
		LOGIN_PAGE_INDEX = 0,
        MAIN_PAGE_INDEX = 1,
	};
}

namespace Ui
{
    ShadowWindow::ShadowWindow(QBrush brush, int shadowWidth)
        : QWidget(0, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , ShadowWidth_(shadowWidth)
        , Brush_(brush)
        , IsActive_(true)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void ShadowWindow::setActive(bool value)
    {
        IsActive_ = value;
        repaint();
    }

    void ShadowWindow::paintEvent(QPaintEvent * /*e*/)
    {
        QRect origin = rect();

        QRect right = QRect(QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1), QPoint(origin.width(), origin.height() - ShadowWidth_ - 1));
        QRect left = QRect(QPoint(origin.x(), origin.y() + ShadowWidth_ + 1), QPoint(origin.x() + ShadowWidth_, origin.height() - ShadowWidth_ - 1));
        QRect top = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.y()), QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_));
        QRect bottom = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_), QPoint(origin.width() - ShadowWidth_ - 1, origin.height()));

        QRect topLeft = QRect(origin.topLeft(), QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_));
        QRect topRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.y()), QPoint(origin.width(), origin.y() + ShadowWidth_));
        QRect bottomLeft = QRect(QPoint(origin.x(), origin.height() - ShadowWidth_), QPoint(origin.x() + ShadowWidth_, origin.height()));
        QRect bottomRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_), origin.bottomRight());

        QPainter p(this);

        QRect body = origin;
        body.setX(origin.x() + ShadowWidth_);
        body.setY(origin.y() + ShadowWidth_);
        body.setWidth(origin.width() - ShadowWidth_ * 2);
        body.setHeight(origin.height() - ShadowWidth_ * 2);
        p.fillRect(body, Brush_);

        QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
        setGradientColor(lg);
        p.fillRect(right, QBrush(lg));

        lg = QLinearGradient(left.topRight(), left.topLeft());
        setGradientColor(lg);
        p.fillRect(left, QBrush(lg));

        lg = QLinearGradient(top.bottomLeft(), top.topLeft());
        setGradientColor(lg);
        p.fillRect(top, QBrush(lg));

        lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
        setGradientColor(lg);
        p.fillRect(bottom, QBrush(lg));

        QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topLeft, QBrush(g));

        g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topRight, QBrush(g));

        g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomLeft, QBrush(g));

        g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomRight, QBrush(g));
    }

    void ShadowWindow::setGradientColor(QGradient& gradient)
    {
        gradient.setColorAt(0, QColor(0, 0, 0, 50));
        gradient.setColorAt(0.2, QColor(0, 0, 0, IsActive_ ? 20 : 10));
        gradient.setColorAt(0.6, IsActive_ ? QColor(0, 0, 0, 5) : Qt::transparent);
        gradient.setColorAt(1, Qt::transparent);
    }

	TitleWidgetEventFilter::TitleWidgetEventFilter(QObject* parent)
		: QObject(parent)
	{
	}

	bool TitleWidgetEventFilter::eventFilter(QObject* obj, QEvent* event)
	{
		switch (event->type())
		{
		case QEvent::MouseButtonDblClick:
			emit doubleClick();
			break;

		case QEvent::MouseButtonPress:
			clickPos = static_cast<QMouseEvent*>(event)->pos();
			event->accept();
			break;

		case QEvent::MouseMove:
			emit moveRequest(static_cast<QMouseEvent*>(event)->globalPos() - clickPos - QPoint(get_gui_settings()->get_shadow_width(), get_gui_settings()->get_shadow_width()));
			break;

		default:
			break;
		}

		return QObject::eventFilter(obj, event);
	}

    void MainWindow::hide_taskbar_icon()
    {
#ifdef _WIN32
        HWND parent = (HWND)::GetWindowLong((HWND) winId(), GWL_HWNDPARENT);
        if (!parent)
            ::SetWindowLong((HWND) winId(), GWL_HWNDPARENT, (LONG) fake_parent_window_);
#endif //_WIN32
        tray_icon_->forceUpdateIcon();
    }

    void MainWindow::show_taskbar_icon()
    {
#ifdef _WIN32
        ::SetWindowLong((HWND) winId(), GWL_HWNDPARENT, 0L);

        std::unique_ptr<QWidget> w(new QWidget(this));
        w->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        w->show();
        w->activateWindow();
        tray_icon_->forceUpdateIcon();
#endif //_WIN32
    }

    MainWindow::MainWindow(QApplication* app)
		: main_page_(nullptr)
		, login_page_(nullptr)
#ifdef __APPLE__
        , accounts_page_(nullptr)
#endif //_APPLE__
		, app_(app)
		, event_filter_(new TitleWidgetEventFilter(this))
		, tray_icon_(new TrayIcon(this))
                , backgroundPixmap_(QPixmap())
        , Shadow_(0)
        , SkipRead_(false)
        , TaskBarIconHidden_(false)
	{
        Utils::InterConnector::instance().setMainWindow(this);

#ifdef _WIN32
        Utils::init_crash_handlers_in_core();
        core::dump::crash_handler chandler;
        chandler.set_process_exception_handlers();
        chandler.set_thread_exception_handlers();
#endif //_WIN32

		setStyleSheet(Utils::LoadStyle(":/main_window/main_window.qss", Utils::get_scale_coefficient(), true));
#ifdef __APPLE__
        mac_support_ = new MacSupport(this);
        mac_support_->enableMacCrashReport();
#endif

        app_->installNativeEventFilter(this);

        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("main_window"));
        this->resize(329, 331);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
        this->setSizePolicy(sizePolicy);
        this->setLayoutDirection(Qt::LeftToRight);
        this->setAutoFillBackground(false);
        main_widget_ = new QWidget(this);
        main_widget_->setObjectName(QStringLiteral("main_widget"));
        sizePolicy.setHeightForWidth(main_widget_->sizePolicy().hasHeightForWidth());
        main_widget_->setSizePolicy(sizePolicy);
        vertical_layout_ = new QVBoxLayout(main_widget_);
        vertical_layout_->setSpacing(0);
        vertical_layout_->setObjectName(QStringLiteral("verticalLayout_9"));
        vertical_layout_->setSizeConstraint(QLayout::SetDefaultConstraint);
        vertical_layout_->setContentsMargins(0, 0, 0, 0);
        title_widget_ = new QWidget(main_widget_);
        title_widget_->setObjectName(QStringLiteral("title_widget"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(title_widget_->sizePolicy().hasHeightForWidth());
        title_widget_->setSizePolicy(sizePolicy1);
        title_widget_->setProperty("TitleWidget", QVariant(true));
        horizontal_layout_ = new QHBoxLayout(title_widget_);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        logo_ = new QPushButton(title_widget_);
        logo_->setObjectName(QStringLiteral("logo"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(logo_->sizePolicy().hasHeightForWidth());
        logo_->setSizePolicy(sizePolicy2);
        logo_->setProperty("WindowIcon", QVariant(true));
        horizontal_layout_->addWidget(logo_);
        title_ = new QLabel(title_widget_);
        title_->setObjectName(QStringLiteral("title"));
        title_->setProperty("Title", QVariant(true));
        horizontal_layout_->addWidget(title_);
        horizontal_spacer_ = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontal_layout_->addItem(horizontal_spacer_);
        hide_button_ = new QPushButton(title_widget_);
        hide_button_->setObjectName(QStringLiteral("hide_button"));
        hide_button_->setProperty("HideButton", QVariant(true));
        horizontal_layout_->addWidget(hide_button_);
        maximize_button_ = new QPushButton(title_widget_);
        maximize_button_->setObjectName(QStringLiteral("maximize_button"));
        sizePolicy2.setHeightForWidth(maximize_button_->sizePolicy().hasHeightForWidth());
        maximize_button_->setSizePolicy(sizePolicy2);
        maximize_button_->setProperty("MaximizeButton", QVariant(true));
        horizontal_layout_->addWidget(maximize_button_);
        close_button_ = new QPushButton(title_widget_);
        close_button_->setObjectName(QStringLiteral("close_button"));
        sizePolicy2.setHeightForWidth(close_button_->sizePolicy().hasHeightForWidth());
        close_button_->setSizePolicy(sizePolicy2);
        close_button_->setProperty("CloseButton", QVariant(true));
        horizontal_layout_->addWidget(close_button_);
        vertical_layout_->addWidget(title_widget_);
        stacked_widget_ = new BackgroundWidget(main_widget_, "");
        stacked_widget_->setObjectName(QStringLiteral("stacked_widget"));
        
        QPixmap p(":/resources/main_window/pat_100.png");
        setBackgroundPixmap(p, true);
        
        //Utils::InterConnector::instance().setMainWindow(this);
        get_qt_theme_settings()->setOrLoadDefaultTheme();
        vertical_layout_->addWidget(stacked_widget_);
        this->setCentralWidget(main_widget_);

        logo_->setText(QString());
        hide_button_->setText(QString());
        maximize_button_->setText(QString());
        close_button_->setText(QString());

        stacked_widget_->setCurrentIndex(-1);
        QMetaObject::connectSlotsByName(this);

        if (!get_gui_settings()->get_value(settings_keep_logged_in, true))// || !get_gui_settings()->contains_value(settings_keep_logged_in))
        {
            showLoginPage();
        }
        else
        {
            showMainPage();
        }

		title_widget_->installEventFilter(event_filter_);
		title_->setText("ICQ");
		title_->setAttribute(Qt::WA_TransparentForMouseEvents);
		logo_->setAttribute(Qt::WA_TransparentForMouseEvents);

		setWindowTitle("ICQ");
#ifdef _WIN32
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowMinimizeButtonHint);
        fake_parent_window_ = Utils::create_fake_parent_window();
#else
        title_widget_->hide();
#endif

		title_->setMouseTracking(true);

		connect(hide_button_, SIGNAL(clicked()), this, SLOT(minimize()), Qt::QueuedConnection);
		connect(maximize_button_, SIGNAL(clicked()), this, SLOT(maximize()), Qt::QueuedConnection);
		connect(close_button_, SIGNAL(clicked()), this, SLOT(hideWindow()), Qt::QueuedConnection);

		hide_button_->setCursor(Qt::PointingHandCursor);
		maximize_button_->setCursor(Qt::PointingHandCursor);
		close_button_->setCursor(Qt::PointingHandCursor);

		connect(event_filter_, SIGNAL(doubleClick()), this, SLOT(maximize()), Qt::QueuedConnection);
		connect(event_filter_, SIGNAL(moveRequest(QPoint)), this, SLOT(moveRequest(QPoint)), Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipResetComplete()), this, SLOT(onVoipResetComplete()), Qt::QueuedConnection);

		connect(Ui::GetDispatcher(), SIGNAL(needLogin()), this, SLOT(showLoginPage()), Qt::DirectConnection);
		connect(&Utils::InterConnector::instance(), SIGNAL(showIconInTaskbar(bool)), this, SLOT(showIconInTaskbar(bool)), Qt::QueuedConnection);

        connect(this, SIGNAL(needActivate()), this, SLOT(activate()), Qt::QueuedConnection);

        connect(get_gui_settings(), SIGNAL(changed(QString)), this, SLOT(guiSettingsChanged(QString)), Qt::QueuedConnection);

		QFont f = QApplication::font();
		f.setStyleStrategy(QFont::PreferAntialias);
		QApplication::setFont(f);

        if (platform::is_windows())
        {
            int shadowWidth = get_gui_settings()->get_shadow_width();
            QBrush b = stacked_widget_->palette().background();
            QMatrix m;
            m.translate(shadowWidth, title_widget_->height() + shadowWidth);
            b.setMatrix(m);
            Shadow_ = new ShadowWindow(b, shadowWidth);
            QPoint pos = mapToGlobal(QPoint(rect().x(), rect().y()));
            Shadow_->move(pos.x(), pos.y());
            Shadow_->resize(rect().width(), rect().height());
            Shadow_->setActive(true);
            Shadow_->show();
        }

        initSettings();
#ifdef _WIN32
        DragAcceptFiles((HWND)winId(), TRUE);
#endif //_WIN32

        if (!get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
            hide_taskbar_icon();

#ifdef __APPLE__
        mac_support_->enableMacUpdater();
        mac_support_->enableMacPreview(this->winId());
#endif

        Alert::setMainWindow(this);
	}

	MainWindow::~MainWindow()
	{
#ifdef _WIN32
        if (fake_parent_window_)
            ::DestroyWindow(fake_parent_window_);
#endif
	}

	void MainWindow::activate()
	{
        setVisible(true);
        tray_icon_->Hide();
        activateWindow();
#ifdef _WIN32
        ShowWindow((HWND)winId(), get_gui_settings()->get_value<bool>(settings_window_maximized, false) ? SW_MAXIMIZE : SW_RESTORE);
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
#ifdef __APPLE__
        mac_support_->activateWindow(winId());
        mac_support_->updateMainMenu();
#endif //__APPLE__
	}

    void MainWindow::activateFromEventLoop()
    {
        emit needActivate();
    }

    bool MainWindow::isActive() const
    {
#ifdef _WIN32
        return GetForegroundWindow() == (HWND)winId();
#else
        return isActiveWindow();
#endif //_WIN32
    }

    int MainWindow::getScreen() const
    {
        return QApplication::desktop()->screenNumber(this);
    }

    void MainWindow::skipRead()
    {
        SkipRead_ = true;
    }

    HistoryControlPage* MainWindow::getHistoryPage(const QString& aimId) const
    {
        return main_page_->getHistoryPage(aimId);
    }

    const QString & MainWindow::activeAimId() const
    {
        assert(main_page_);
        return main_page_->getContactDialog()->currentAimId();
    }
    
    MainPage* MainWindow::getMainPage() const
    {
        assert(main_page_);
        return main_page_;
    }

	bool MainWindow::nativeEventFilter(const QByteArray &data, void *message, long *result)
	{
#ifdef _WIN32
		MSG* msg = (MSG*)(message);
		if (msg->message == WM_NCHITTEST)
		{
			if (msg->hwnd != (HANDLE)winId())
			{
				return false;
			}

			int boxWidth = Utils::scale_value(SIZE_BOX_WIDTH);
			if (isMaximized())
			{
				*result = HTCLIENT;
				return true;
			}

			int x = GET_X_LPARAM(msg->lParam);
			int y = GET_Y_LPARAM(msg->lParam);

			QPoint topLeft = QWidget::mapToGlobal(rect().topLeft());
			QPoint bottomRight = QWidget::mapToGlobal(rect().bottomRight());

			if (x <= topLeft.x() + boxWidth)
			{
				if (y <= topLeft.y() + boxWidth)
					*result = HTTOPLEFT;
				else if (y >= bottomRight.y() - boxWidth)
					*result = HTBOTTOMLEFT;
				else
					*result = HTLEFT;
			}
			else if (x >= bottomRight.x() - boxWidth)
			{
				if (y <= topLeft.y() + boxWidth)
					*result = HTTOPRIGHT;
				else if (y >= bottomRight.y() - boxWidth)
					*result = HTBOTTOMRIGHT;
				else
					*result = HTRIGHT;
			}
			else
			{
				if (y <= topLeft.y() + boxWidth)
					*result = HTTOP;
				else if (y >= bottomRight.y() - boxWidth)
					*result = HTBOTTOM;
				else
					*result = HTCLIENT;
			}
			return true;
		}
		else if ((msg->message == WM_SYSCOMMAND && msg->wParam == SC_RESTORE) || (msg->message == WM_SHOWWINDOW && msg->hwnd == (HWND)winId() && msg->wParam == TRUE))
		{
			setVisible(true);
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            tray_icon_->Hide();
            if (!SkipRead_)
			    Logic::GetRecentsModel()->sendLastRead();
            if (!TaskBarIconHidden_)
                SkipRead_ = false;
            TaskBarIconHidden_ = false;
		}
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_CLOSE)
        {
            hideWindow();
            return true;
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam  == SC_MINIMIZE)
        {
            minimize();
            return true;
        }
        else if (msg->message == WM_WINDOWPOSCHANGING || msg->message == WM_WINDOWPOSCHANGED)
        {
            if (msg->hwnd != (HANDLE)winId())
            {
                return false;
            }

            WINDOWPOS* pos = (WINDOWPOS*)msg->lParam;
            if (pos->flags == 0x8170 || pos->flags == 0x8130)
            {
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                return false;
            }
            if (Shadow_)
            {
                if (!(pos->flags & SWP_NOSIZE) && !(pos->flags & SWP_NOMOVE) && !(pos->flags & SWP_DRAWFRAME))
                {
                    int shadowWidth = get_gui_settings()->get_shadow_width();
                    SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), pos->x - shadowWidth, pos->y - shadowWidth, pos->cx + shadowWidth * 2, pos->cy + shadowWidth * 2, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
                }
                else if (!(pos->flags & SWP_NOZORDER))
                {
                    UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE;
                    if (pos->flags & SWP_SHOWWINDOW)
                        flags |= SWP_SHOWWINDOW;
                    if (pos->flags & SWP_HIDEWINDOW)
                        flags |= SWP_HIDEWINDOW;

                    SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, flags);
                }
            }
        }
        else if (msg->message == WM_ACTIVATE)
        {
            if (!Shadow_)
                return false;

            if (msg->hwnd == (HWND)Shadow_->winId() && msg->wParam != WA_INACTIVE)
            {
                activate();
                return false;
            }
        }
        else if (msg->message == WM_DEVICECHANGE)
        {
            GetSoundsManager()->reinit();
        }
#else
        
#ifdef __APPLE__
        return MacSupport::nativeEventFilter(data, message, result);
#endif
        
#endif //_WIN32
		return false;
	}

	void MainWindow::resizeEvent(QResizeEvent* event)
	{        
        if (isMaximized())
        {
            get_gui_settings()->set_value(settings_window_maximized, true);
        }
        else
        {
            QRect rc = Ui::get_gui_settings()->get_value(settings_main_window_rect, QRect());
            rc.setWidth(event->size().width());
            rc.setHeight(event->size().height());

            get_gui_settings()->set_value(settings_main_window_rect, rc);
            get_gui_settings()->set_value(settings_window_maximized, false);
        }
        // TODO : limit call this stats
        // GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::main_window_resize);
        
#ifdef __APPLE__
        mac_support_->updateMainMenu();
#endif
	}

	void MainWindow::moveEvent(QMoveEvent* event)
	{
        if (!isMaximized())
        {
		    auto rc = Ui::get_gui_settings()->get_value<QRect>(settings_main_window_rect, QRect());
		    rc.moveTo(event->pos().x(), event->pos().y());
            Ui::get_gui_settings()->set_value<QRect>(settings_main_window_rect, rc);
        }
	}

	void MainWindow::changeEvent(QEvent* event)
	{
		if (event->type() == QEvent::WindowStateChange)
		{
            if (platform::is_apple() && !isMaximized())
            {
                emit Utils::InterConnector::instance().closeAnyPopupWindow();
            }
			maximize_button_->setProperty("MinimizeButton", isMaximized());
			maximize_button_->setProperty("MaximizeButton", !isMaximized());
			maximize_button_->setStyle(QApplication::style());
			get_gui_settings()->set_value<bool>(settings_window_maximized, isMaximized());
		}
        else if (event->type() == QEvent::ActivationChange)
        {
            if (isActiveWindow())
            {
                tray_icon_->Hide();
                if (!SkipRead_)
                    Logic::GetRecentsModel()->sendLastRead();
                SkipRead_ = false;
            }
            
            if (Shadow_)
            {
                Shadow_->setActive(isActiveWindow());
            }
        }

        if (event->type() == QEvent::ApplicationStateChange)
        {
            if (Shadow_)
            {
                Shadow_->setActive(isActiveWindow());
            }
        }

        QMainWindow::changeEvent(event);
	}

	void MainWindow::closeEvent(QCloseEvent* event)
	{
		Previewer::ClosePreview();

        if (!platform::is_windows())
        {
            if (event->spontaneous())
            {
                event->ignore();

                hideWindow();
            }
        }
	}

    void MainWindow::keyPressEvent(QKeyEvent* event)
    {
        QWidget* w = stacked_widget_->currentWidget();
        if (w && qobject_cast<MainPage*>(w) && event->matches(QKeySequence::Find))
            main_page_->setSearchFocus();

#ifndef __APPLE__
        if (w && qobject_cast<MainPage*>(w) && event->key() == Qt::Key_Escape)
            minimize();
#endif
        
#ifdef __linux__
        if (w && qobject_cast<MainPage*>(w) && event->modifiers() | Qt::ControlModifier && event->key() == Qt::Key_Q)
            exit();
#endif //__linux__

        QMainWindow::keyPressEvent(event);
    }
    
    //void MainWindow::paintEvent(QPaintEvent *_e)
    //{
    //  QWidget::paintEvent(_e);
    //}
    
    void MainWindow::setBackgroundPixmap(QPixmap& _pixmap, const bool _tiling)
    {
        Utils::check_pixel_ratio(_pixmap);
        if (_pixmap.isNull())
        {
            _pixmap = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_100.png"));
        }

        stacked_widget_->setImage(_pixmap, _tiling);
    }
    
	void MainWindow::initSettings()
	{
        auto main_rect = Ui::get_gui_settings()->get_value<QRect>(
            settings_main_window_rect,
            QRect(0, 0, Utils::scale_value(1000), Utils::scale_value(600)));

		resize(main_rect.width(), main_rect.height());
		setMinimumHeight(Utils::scale_value(550));
		setMinimumWidth(Utils::scale_value(700));

        if (main_rect.left() == 0 && main_rect.top() == 0)
		{
			QRect rc = main_rect;

            QRect desktopRect = QDesktopWidget().availableGeometry(this);

            QPoint center = desktopRect.center();

			move(center.x() - width()*0.5, center.y()-height()*0.5);

            get_gui_settings()->set_value(settings_main_window_rect, geometry());
		}
        else
        {
            move(main_rect.left(), main_rect.top());
        }

        bool isMaximized = get_gui_settings()->get_value<bool>(settings_window_maximized, false);
		isMaximized ? showMaximized() : show();
        maximize_button_->setProperty("MinimizeButton", isMaximized);
        maximize_button_->setProperty("MaximizeButton", !isMaximized);
        maximize_button_->setStyle(QApplication::style());

#ifdef _WIN32
        if (isMaximized)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
	}

	void MainWindow::maximize()
	{
		if (isMaximized())
		{
			showNormal();
            auto main_rect = Ui::get_gui_settings()->get_value<QRect>(
                settings_main_window_rect,
                QRect(0, 0, Utils::scale_value(1000), Utils::scale_value(600)));

			resize(main_rect.width(), main_rect.height());
            move(main_rect.x(), main_rect.y());
#ifdef _WIN32
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
		}
        else
        {
#ifdef _WIN32
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
			showMaximized();
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::main_window_fullscreen);
	}

    void MainWindow::minimize()
    {
        if (get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
        {
            showMinimized();
#ifdef _WIN32
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
        }
        else
        {
            hideWindow();
        }
    }

	void MainWindow::moveRequest(QPoint point)
	{
		if (isMaximized())
			maximize();
		else
			move(point);
	}

    void MainWindow::guiSettingsChanged(QString key)
    {
        if (key == settings_language || key == settings_scale_coefficient)
        {
            //showLoginPage();
            //showMainPage();
        }
    }

    void MainWindow::clear_global_objects()
    {
        // delete main page
        if (main_page_)
        {
            stacked_widget_->removeWidget(main_page_);
            MainPage::reset();
            main_page_ = 0;
        }

        Logic::ResetContactListModel();
        Logic::ResetRecentsModel();
        Logic::ResetMessagesModel();

        Ui::stickers::reset_cache();

        tray_icon_->forceUpdateIcon();
    }
    
    void MainWindow::showMigrateAccountPage(QString accountId)
    {
#ifdef __APPLE__
        MacMigrationManager * manager = new MacMigrationManager(accountId);
        
        if (manager->getProfiles().size() == 1)
        {
            manager->migrateProfile(manager->getProfiles()[0]);
            showMainPage();
        }
        else
        {
            if (!accounts_page_)
            {
                accounts_page_ = new AccountsPage(this, manager);
                stacked_widget_->addWidget(accounts_page_);
                
                connect(accounts_page_, SIGNAL(account_selected()), this, SLOT(showMainPage()), Qt::QueuedConnection);
            }
            
            stacked_widget_->setCurrentWidget(accounts_page_);
            
            clear_global_objects();
        }
        
        delete manager;
#endif
    }

	void MainWindow::showLoginPage()
	{
#ifdef __APPLE__
        mac_support_->createMenuBar(true);
        
        mac_support_->forceEnglishInputSource();
        
        if (!get_gui_settings()->get_value<bool>(settings_mac_accounts_migrated, false))
        {
            QString accountId = MacMigrationManager::canMigrateAccount();
            
            if (accountId.length() > 0)
            {
                // Move it out of ifdef-block if it's needed for other platforms
                main_page_ = nullptr;
                MainPage::reset();

                showMigrateAccountPage(accountId);
                return;
            }
        }
#endif

        if (!login_page_)
        {
            login_page_ = new LoginPage(this, true /* is_login */);
            stacked_widget_->addWidget(login_page_);

			connect(login_page_, SIGNAL(loggedIn()), this, SLOT(showMainPage()), Qt::QueuedConnection);
        }
        
		stacked_widget_->setCurrentWidget(login_page_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_phone);

        login_page_->enableKeepLogedIn();

        clear_global_objects();
    }

	void MainWindow::showMainPage()
	{
        if (!main_page_)
        {
            main_page_ = MainPage::instance(this);
            stacked_widget_->addWidget(main_page_);
        }
#ifdef __APPLE__
        mac_support_->createMenuBar(false);
#endif
        stacked_widget_->setCurrentWidget(main_page_);
	}

    void MainWindow::checkForUpdates()
    {
#ifdef __APPLE__
        mac_support_->runMacUpdater();
#endif
    }

    void MainWindow::showIconInTaskbar(bool _show)
    {
        if (_show)
        {
            show_taskbar_icon();
        }
        else
        {
            hide_taskbar_icon();
        }
    }

    void MainWindow::copy()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            bool handled = false;
            
            if (platform::is_apple())
            {
                Ui::MessagesScrollArea * area = dynamic_cast<Ui::MessagesScrollArea*>(focused);

                if (area)
                {
                    QString text = area->getSelectedText();
#ifdef __APPLE__
                    MacSupport::replacePasteboard(text);
#endif
                    
                    handled = true;
                }
            }
            
            if (!handled)
            {
                QApplication::postEvent( focused,
                                        new QKeyEvent( QEvent::KeyPress,
                                                      Qt::Key_C,
                                                      Qt::ControlModifier ));
                QApplication::postEvent( focused,
                                        new QKeyEvent( QEvent::KeyRelease,
                                                      Qt::Key_C,
                                                      Qt::ControlModifier ));
            }
        }
    }
    
    void MainWindow::quote()
    {
        QClipboard * clip = QApplication::clipboard();
        
        QString text = clip->text();
        
        if (text.isEmpty()) return;
        
        QStringList lines = text.split('\n');
        text.clear();
        for (auto line : lines)
        {
            if (!line.isEmpty())
                text += ">";
            text += line;
            text += "\n";
        }
        
        const QString & aimId = activeAimId();
        HistoryControlPage * page = main_page_->getHistoryPage(aimId);
        page->quote(text);
    }
    
    void MainWindow::cut()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_X,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_X,
                                                  Qt::ControlModifier ));
        }
    }
    
    void MainWindow::paste()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_V,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_V,
                                                  Qt::ControlModifier ));
        }
    }
    
    
    void MainWindow::undo()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier ));
        }
    }
    
    
    void MainWindow::redo()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier|Qt::ShiftModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier|Qt::ShiftModifier ));
        }
    }
    
    void MainWindow::activateSettings()
    {
        activate();
        main_page_->settingsTabActivate();
    }
    
    void MainWindow::activateNextUnread()
    {
        activate();
        main_page_->recentsTabActivate(true);
    }
    
    void MainWindow::activateNextChat()
    {
        activate();
        main_page_->recentsTabActivate(false);
        const QString & aimId = activeAimId();
        main_page_->selectRecentChat(Logic::GetRecentsModel()->nextAimId(aimId));
    }
    
    void MainWindow::activatePrevChat()
    {
        activate();
        main_page_->recentsTabActivate(false);
        const QString & aimId = activeAimId();
        main_page_->selectRecentChat(Logic::GetRecentsModel()->prevAimId(aimId));
    }
    
    void MainWindow::activateContactSearch()
    {
        activate();
        main_page_->contactListActivate(true);
    }
    
    void MainWindow::activateAbout()
    {
        activate();
        main_page_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_About);
    }
    
    void MainWindow::activateProfile()
    {
        activate();
        main_page_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Profile);
    }
    
    void MainWindow::closeCurrent()
    {
        activate();
        const QString & aimId = activeAimId();
        Logic::GetRecentsModel()->hideChat(aimId);
    }
    
    void MainWindow::toggleFullScreen()
    {
#ifdef __APPLE__
        MacSupport::toggleFullScreen(this->winId());
#endif
    }
    
    void MainWindow::updateMainMenu()
    {
#ifdef __APPLE__
        mac_support_->updateMainMenu();
#endif
    }
    
    void MainWindow::exit() {
#ifdef STRIP_VOIP
        QApplication::exit();
#else
        
#ifdef _WIN32
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif
        
        Ui::GetDispatcher()->getVoipController().voipReset();
#endif //STRIP_VOIP
    }

    void MainWindow::onVoipResetComplete()
    {
        QApplication::exit();
	}

    void MainWindow::hideWindow()
    {
        TaskBarIconHidden_ = true;
        hide();
#ifdef _WIN32
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
    }
    
    void MainWindow::pasteEmoji()
    {
        getMainPage()->getContactDialog()->onSmilesMenu();
    }
}
