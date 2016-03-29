#include "stdafx.h"
#include "TrayIcon.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../controls/ContextMenu.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../sounds/SoundsManager.h"
#include "RecentMessagesAlert.h"

#include "../../my_info.h"

#ifdef _WIN32
#include "toast_notifications/ToastManager.h"
typedef HRESULT (__stdcall *QueryUserNotificationState)(QUERY_USER_NOTIFICATION_STATE *pquns);
typedef BOOL (__stdcall *QuerySystemParametersInfo)(__in UINT uiAction, __in UINT uiParam, __inout_opt PVOID pvParam, __in UINT fWinIni);
#else

#ifdef __APPLE__
#include "notification_center/NotificationCenterManager.h"
#include "../../utils/mac_support.h"
#endif

#endif //_WIN32

#ifdef _WIN32
#include <Shobjidl.h>
extern HICON qt_pixmapToWinHICON(const QPixmap &p);
QString pinPath = "\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\ICQ.lnk";
#endif // _WIN32

namespace Ui
{
	TrayIcon::TrayIcon(MainWindow* parent)
		: QObject(parent)
		, MainWindow_(parent)
		, Icon_(new QSystemTrayIcon(this))
		, Menu_(new ContextMenu(0))
		, MessageAlert_(new RecentMessagesAlert(new Logic::RecentItemDelegate(this)))
		, Base_(new QIcon(":/resources/main_window/appicon.ico"))
		, Unreads_(new QIcon(":/resources/main_window/appicon_unread.ico"))
#ifdef _WIN32
		, TrayBase_(new QIcon(":/resources/main_window/appicon_tray.ico"))
        , TrayUnreads_(new QIcon(":/resources/main_window/appicon_tray_unread.ico"))
#else
        , TrayBase_(new QIcon(":/resources/main_window/appicon.ico"))
        , TrayUnreads_(new QIcon(":/resources/main_window/appicon_unread.ico"))
#endif //_WIN32
        , TaskBarOverlay_(new QPixmap(":/resources/main_window/appicon_overlay.png"))
		, HaveUnreads_(false)
#ifdef _WIN32
        , ptbl(0)
#endif //_WIN32
	{
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
            if (FAILED(hr))
                ptbl = 0;
        }
#endif //_WIN32
		init();
        
        connect(Ui::GetDispatcher(), SIGNAL(login_complete()), this, SLOT(loggedIn()), Qt::QueuedConnection);
		connect(MessageAlert_, SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
		connect(Logic::GetContactListModel(), SIGNAL(selectedContactChanged(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
		connect(Logic::GetContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(updateIcon()), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(myInfo()), this, SLOT(myInfo()));
        connect(Ui::GetDispatcher(), SIGNAL(needLogin()), this, SLOT(loggedOut()), Qt::QueuedConnection);
        
#ifdef __APPLE__
        ncSupported(); //setup notification manager
#endif //__APPLE__
        
	}

	TrayIcon::~TrayIcon()
	{
        disconnect(get_gui_settings());
	}

	void TrayIcon::Hide()
	{
		MessageAlert_->hide();
		MessageAlert_->markShowed();
        forceUpdateIcon();
	}

    void TrayIcon::forceUpdateIcon()
    {
        bool unreads = Logic::GetRecentsModel()->totalUnreads() != 0;
        HaveUnreads_ = unreads;
#ifdef _WIN32
        if (ptbl)
            ptbl->SetOverlayIcon((HWND)MainWindow_->winId(), unreads ? qt_pixmapToWinHICON(*TaskBarOverlay_) : HICON(), L"");
        else
            MainWindow_->setWindowIcon(unreads ? *Unreads_ : *Base_);
#endif //_WIN32

        if (!platform::is_apple())
        {
            Icon_->setIcon(unreads ? *TrayUnreads_ : *TrayBase_);
        }
        else
        {
            setMacIcon();
        }

        updateStatus();
    }

    void TrayIcon::setMacIcon()
    {
#ifdef __APPLE__
        QString state = MyInfo()->state().toLower();
        
        if (state.length() == 0 ||
            state == "mobile")
        {
            state = "online";
        }
        
        bool unreads = Logic::GetRecentsModel()->totalUnreads() != 0;
        QString iconResource(QString(":resources/main_window/mac_tray/icq_osxlogo_%1_%2%3_100.png").
                             arg(state).arg(MacSupport::currentTheme()).
                             arg(unreads?"_unread":""));
        QIcon icon(Utils::parse_image_name(iconResource));
        Icon_->setIcon(icon);
#endif
    }
    
    void TrayIcon::myInfo()
    {
        setMacIcon();
        updateStatus();
    }
    
    void TrayIcon::updateStatus()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }
        
        onlineAction_->setCheckable(true);
        dndAction_->setCheckable(true);
        invAction_->setCheckable(true);
        
        auto state = MyInfo()->state().toLower();
        if (state == "invisible")
        {
            onlineAction_->setChecked(false);
            dndAction_->setChecked(false);
            invAction_->setChecked(true);
        }
        else if (state == "dnd")
        {
            onlineAction_->setChecked(false);
            dndAction_->setChecked(true);
            invAction_->setChecked(false);
        }
        //        else if (state == "offline")
        //        {
        //            setStateOffline();
        //    }
        else
        {
            onlineAction_->setChecked(true);
            dndAction_->setChecked(false);
            invAction_->setChecked(false);
        }
    }
    
	void TrayIcon::updateIcon()
	{
		bool unreads = Logic::GetRecentsModel()->totalUnreads() != 0;
		if (unreads != HaveUnreads_)
		{
			HaveUnreads_ = unreads;
#ifdef _WIN32
            if (ptbl)
                ptbl->SetOverlayIcon((HWND)MainWindow_->winId(), unreads ? qt_pixmapToWinHICON(*TaskBarOverlay_) : HICON(), L"");
            else
                MainWindow_->setWindowIcon(unreads ? *Unreads_ : *Base_);
#endif //_WIN32
            
            if (!platform::is_apple())
            {
                Icon_->setIcon(unreads ? *TrayUnreads_ : *TrayBase_);
            }
            else
            {
                setMacIcon();
            }
		}
        
        updateStatus();
	}

	void TrayIcon::clearNotifications(QString aimId)
	{
#if defined (_WIN32)
		if (toastSupported())
			ToastManager_->HideNotifications(aimId);
#elif defined (__APPLE__)
        if (ncSupported())
        {
            NotificationCenterManager_->HideNotifications(aimId);
        }
#endif //_WIN32
	}

	void TrayIcon::dlgState(Data::DlgState state)
	{
		bool canNotify = state.Visible_ && (!ShowedMessages_.contains(state.AimId_) || (state.LastMsgId_ != -1 && ShowedMessages_[state.AimId_] < state.LastMsgId_));
        if (state.GetText().isEmpty())
            canNotify = false;

		if (!state.Outgoing_)
		{
			if (state.UnreadCount_ != 0 && canNotify && !Logic::GetContactListModel()->isMuted(state.AimId_))
			{
				ShowedMessages_[state.AimId_] = state.LastMsgId_;
				if (canShowNotifications())
					showMessage(state);
				GetSoundsManager()->playIncomingMessage();
			}
		}
		else if (state.Visible_ && ((!ShowedMessages_.contains(state.AimId_) && state.LastMsgId_ == -1) || state.LastMsgId_ == -1))
		{
			ShowedMessages_[state.AimId_] = state.LastMsgId_;
			GetSoundsManager()->playOutgoingMessage();
		}

		updateIcon();
        if (state.Visible_)
		    markShowed(state.AimId_);
        
#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(Logic::GetRecentsModel()->totalUnreads());
#endif
	}

	void TrayIcon::messageClicked(QString aimId)
	{
#if defined (_WIN32)
		if (!toastSupported())
#elif defined (__APPLE__)
        if (!ncSupported())
#endif //_WIN32
		{
			MessageAlert_->hide();
			MessageAlert_->markShowed();
		}
        
        if (!aimId.isEmpty())
        {
            if (Logic::GetContactListModel()->selectedContact() != aimId)
                Utils::InterConnector::instance().getMainWindow()->skipRead();
            Logic::GetContactListModel()->setCurrent(aimId, true);
        }
        
        MainWindow_->activateFromEventLoop();
	    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_click);
    }

	void TrayIcon::showMessage(const Data::DlgState& state)
	{
#if defined (_WIN32)
		if (toastSupported())
		{
			ToastManager_->DisplayToastMessage(state.AimId_, state.GetText());
			return;
		}
#elif defined (__APPLE__)
        if (ncSupported())
        {
            NotificationCenterManager_->DisplayNotification(state.AimId_, state.senderNick_, state.GetText());
            return;
        }
#endif //_WIN32
		MessageAlert_->addAlert(state);

		TrayPosition pos = getTrayPosition();
		QRect availableGeometry = QDesktopWidget().availableGeometry();

		int screenMargin = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();

		switch (pos)
		{
		case Ui::TOP_RIGHT:
			MessageAlert_->move(availableGeometry.topRight().x() - MessageAlert_->width() - screenMargin, availableGeometry.topRight().y() + screenMargin);
			break;

		case Ui::BOTTOM_LEFT:
			MessageAlert_->move(availableGeometry.bottomLeft().x() + screenMargin, availableGeometry.bottomLeft().y() - MessageAlert_->height() - screenMargin);
			break;

		case Ui::BOTOOM_RIGHT:
			MessageAlert_->move(availableGeometry.bottomRight().x() - MessageAlert_->width() - screenMargin, availableGeometry.bottomRight().y() - MessageAlert_->height() - screenMargin);
			break;

		case Ui::TOP_LEFT:
		default:
            MessageAlert_->move(availableGeometry.topLeft().x() + screenMargin, availableGeometry.topLeft().y() + screenMargin);
			break;
		}

		MessageAlert_->show();
	}

	TrayPosition TrayIcon::getTrayPosition() const
	{
		QRect availableGeometry = QDesktopWidget().availableGeometry();
        QRect iconGeometry = Icon_->geometry();

#ifdef __linux__
        if (iconGeometry.isEmpty())
            return TOP_RIGHT;
#endif //__linux__

		bool top = abs(iconGeometry.y() - availableGeometry.topLeft().y()) < abs(iconGeometry.y() - availableGeometry.bottomLeft().y());
		if (abs(iconGeometry.x() - availableGeometry.topLeft().x()) < abs(iconGeometry.x() - availableGeometry.topRight().x()))
			return top ? TOP_LEFT : BOTTOM_LEFT;
		else
			return top ? TOP_RIGHT : BOTOOM_RIGHT;
	}

	void TrayIcon::init()
	{
		MessageAlert_->hide();

		MainWindow_->setWindowIcon(*Base_);
        if (!platform::is_apple())
        {
            Icon_->setIcon(*TrayBase_);
        }
        else
        {
            setMacIcon();
        }
        
        onlineAction_ = dndAction_ = invAction_ = NULL;
        
        
#ifdef __APPLE__
        
        onlineAction_ = Menu_->addAction(QIcon(":/resources/content_status_online_200.png"), QT_TRANSLATE_NOOP("tray_menu", "Online"), this, SLOT(menuStateOnline()));
        dndAction_ = Menu_->addAction(QIcon(":/resources/content_status_dnd_200.png"), QT_TRANSLATE_NOOP("tray_menu", "Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
        invAction_ = Menu_->addAction(QIcon(":/resources/content_status_invisible_200.png"), QT_TRANSLATE_NOOP("tray_menu", "Invisible"), this, SLOT(menuStateInvisible()));
        
        updateStatus();
        
        Menu_->addSeparator();
        
//#ifdef UPDATES
//        Menu_->addAction(QIcon(), QT_TRANSLATE_NOOP("context_menu", "Check for updates..."), parent(), SLOT(checkForUpdates()));
//        Menu_->addSeparator();
//#endif
        Menu_->addAction(QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#else
#ifdef __linux__
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Show"), parent(), SLOT(activate()));
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#else
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quit_100.png")), QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#endif //__linux__
#endif
        
		Icon_->setContextMenu(Menu_);
		connect(Icon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)), Qt::QueuedConnection);
        Icon_->show();
	}

	void TrayIcon::markShowed(const QString& aimId)
	{
		Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("aimid", aimId);
		Ui::GetDispatcher()->post_message_to_core("dlg_state/hide", collection.get());
	}

	bool TrayIcon::canShowNotifications() const
	{
        // TODO: must be based on the type of notification - is it message, birthday-notify or contact-coming-notify.
        if (!get_gui_settings()->get_value<bool>(settings_notify_new_messages, true))  return false;

#ifdef _WIN32
		if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
		{
			static QueryUserNotificationState query;
			if (!query)
			{
				HINSTANCE shell32 = LoadLibraryW(L"shell32.dll");
				if (shell32)
				{
					query = (QueryUserNotificationState)GetProcAddress(shell32, "SHQueryUserNotificationState");
				}
			}

			if (query)
			{
				QUERY_USER_NOTIFICATION_STATE state;
				if (query(&state) == S_OK && state != QUNS_ACCEPTS_NOTIFICATIONS)
					return false;
			}
		}
        else if (QSysInfo().windowsVersion() == QSysInfo::WV_XP)
        {
            static QuerySystemParametersInfo query;
            if (!query)
            {
                HINSTANCE user32 = LoadLibraryW(L"user32.dll");
                if (user32)
                {
                    query = (QuerySystemParametersInfo)GetProcAddress(user32, "SystemParametersInfoW");
                }
            }

            if (query)
            {
                BOOL result = FALSE;      
                if (query(SPI_GETSCREENSAVERRUNNING, 0, &result, 0) && result)
                    return false;
            }
        }

#endif //_WIN32
		return Icon_->isSystemTrayAvailable() && Icon_->supportsMessages() && !MainWindow_->isActive();
	}

	void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
	{
        if (platform::is_windows())
        {
            if (reason == QSystemTrayIcon::Trigger)
                MainWindow_->activate();
        }
        
        updateStatus();
	}

    void TrayIcon::loggedIn()
    {
        connect(Logic::GetRecentsModel(), SIGNAL(dlgStateHandled(Data::DlgState)), this, SLOT(dlgState(Data::DlgState)), Qt::QueuedConnection);
    }

    void TrayIcon::loggedOut()
    {
#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(0);
#endif
    }

#if defined (_WIN32)
	bool TrayIcon::toastSupported()
	{
		return false;
		if (QSysInfo().windowsVersion() > QSysInfo::WV_WINDOWS8_1)
		{
			if (!ToastManager_.get())
			{
				ToastManager_.reset(new ToastManager());
				connect(ToastManager_.get(), SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
			}
			return true;
		}
		return false;
	}
#elif defined (__APPLE__)
    bool TrayIcon::ncSupported()
    {
        if (QSysInfo().macVersion() > QSysInfo::MV_10_7)
        {
            if (!NotificationCenterManager_.get())
            {
                NotificationCenterManager_.reset(new NotificationCenterManager());
                connect(NotificationCenterManager_.get(), SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
                connect(NotificationCenterManager_.get(), SIGNAL(osxThemeChanged()), this, SLOT(myInfo()), Qt::QueuedConnection);
            }
            return true;
        }
        return false;
    }
#endif //_WIN32
    
    void TrayIcon::menuStateOnline()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }
        
        dndAction_->setChecked(false);
        invAction_->setChecked(false);
        
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "online");
        collection.set_value_as_string("aimid",  MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
    void TrayIcon::menuStateDoNotDisturb()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }
        
        onlineAction_->setChecked(false);
        dndAction_->setChecked(false);
        
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "dnd");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
    void TrayIcon::menuStateInvisible()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }
        
        onlineAction_->setChecked(false);
        dndAction_->setChecked(false);
        
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "invisible");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
}
