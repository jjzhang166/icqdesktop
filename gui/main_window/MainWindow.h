#pragma once

#ifdef __APPLE__
class MacSupport;
#endif
#include "../voip/CallPanelMain.h"

class QApplication;

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    class main_window;
    class MainPage;
    class LoginPage;
    class PromoPage;
#ifdef __APPLE__
    class AccountsPage;
#endif
    class TrayIcon;
    class HistoryControlPage;
    class BackgroundWidget;
    
    class ShadowWindow : public QWidget
    {
        Q_OBJECT

    public:

        ShadowWindow(QBrush brush, int shadowWidth);
        void setActive(bool value);

    protected:
        virtual void paintEvent(QPaintEvent *e);

    private:
        void setGradientColor(QGradient& gradient);

    private:
        int ShadowWidth_;
        QBrush Brush_;
        bool IsActive_;
    };

    class TitleWidgetEventFilter : public QObject
    {
        Q_OBJECT
Q_SIGNALS:

        void doubleClick();
        void moveRequest(QPoint);
        void checkPosition();

    public:
        TitleWidgetEventFilter(QObject* parent);

    protected:
        bool eventFilter(QObject* obj, QEvent* event);

    private:
        QPoint clickPos;
    };

    class MainWindow : public QMainWindow, QAbstractNativeEventFilter
    {
        Q_OBJECT

    Q_SIGNALS:
        void needActivate();

    public Q_SLOTS:

        void showPromoPage();
        void closePromoPage();
        void showLoginPage();
        void showMainPage();
        void showMigrateAccountPage(QString accountId);
        void checkForUpdates();
        void showIconInTaskbar(bool);
        void activate();
        void updateMainMenu();
        void gotoSleep();
        void gotoWake();
        void exit();

    private Q_SLOTS:

        void maximize();
        void moveRequest(QPoint);
        void minimize();
        void guiSettingsChanged(QString);
        void onVoipResetComplete();
        void hideWindow();
        void copy();
        void cut();
        void paste();
        void quote();
        void undo();
        void redo();
        void activateSettings();
        void activateContactSearch();
        void activateAbout();
        void activateProfile();
        void closeCurrent();
        void activateNextUnread();
        void activateNextChat();
        void activatePrevChat();
        void toggleFullScreen();
        void pasteEmoji();
        void checkPosition();

        void onOpenChat(const std::string& contact);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& contact_ex);
        void onVoipCallCreated(const voip_manager::ContactEx& contact_ex);
        void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);

    public:
        MainWindow(QApplication* app);
        ~MainWindow();

        void activateFromEventLoop();
        bool isActive() const;
        bool isMainPage() const;
        void setBackgroundPixmap(QPixmap& _pixmap, const bool _tiling);

        int getScreen() const;
        void skipRead(); //skip next sending last read by window activation

        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        MainPage* getMainPage() const;

        void insertTopWidget(const QString& aimId, QWidget* widget);
        void removeTopWidget(const QString& aimId);

        void showSidebar(const QString& aimId, int page);
        void setSidebarVisible(bool show);
        bool isSidebarVisible() const;
        
        void showMenuBarIcon(bool show);

    private:
        void initSizes();
        void initSettings();

    protected:
        bool nativeEventFilter(const QByteArray &, void * message, long * result);

        virtual void resizeEvent(QResizeEvent* event);
        virtual void moveEvent(QMoveEvent* event);
        virtual void changeEvent(QEvent* event);
        virtual void closeEvent(QCloseEvent* event);
        virtual void keyPressEvent(QKeyEvent* event);
        void hide_taskbar_icon();
        void show_taskbar_icon();
        void clear_global_objects();

    private:
        MainPage* main_page_;
        LoginPage* login_page_;
        PromoPage* promo_page_;
        QApplication* app_;
        TitleWidgetEventFilter* event_filter_;
        TrayIcon* tray_icon_;
        QPixmap backgroundPixmap_;
        QWidget *main_widget_;
        QVBoxLayout *vertical_layout_;
        QWidget *title_widget_;
        QHBoxLayout *horizontal_layout_;
        QPushButton *logo_;
        QLabel *title_;
        QSpacerItem *horizontal_spacer_;
        QPushButton *hide_button_;
        QPushButton *maximize_button_;
        QPushButton *close_button_;
        BackgroundWidget *stacked_widget_;
        ShadowWindow* Shadow_;
        CallPanelMainEx* callPanelMainEx;
        bool SkipRead_;
        bool TaskBarIconHidden_;

#ifdef _WIN32
        HWND fake_parent_window_;
#endif //_WIN32
#ifdef __APPLE__
        MacSupport*   mac_support_;
        AccountsPage* accounts_page_;
#endif
	};
}