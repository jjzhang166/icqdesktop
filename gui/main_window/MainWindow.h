#pragma once

#ifdef __APPLE__
class MacSupport;
#endif
#include "../voip/CallPanelMain.h"

class QApplication;

namespace Data
{
    struct Image;
}

namespace Previewer
{
    class GalleryWidget;
}

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    class MainPage;
    class LoginPage;
    class PromoPage;
#ifdef __APPLE__
    class AccountsPage;
#endif
    class TrayIcon;
    class HistoryControlPage;
    class BackgroundWidget;
    class MultimediaViewer;

    class ShadowWindow : public QWidget
    {
        Q_OBJECT

    public:

        ShadowWindow(QBrush _brush, int _shadowWidth);
        void setActive(bool _value);

    protected:
        virtual void paintEvent(QPaintEvent* _e);

    private:
        void setGradientColor(QGradient& _gradient);

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
        TitleWidgetEventFilter(QObject* _parent);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);

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
        void showMigrateAccountPage(QString _accountId);
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

        void onOpenChat(const std::string& _contact);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contact_ex);
        void onVoipCallCreated(const voip_manager::ContactEx& _contact_ex);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contact_ex);
		void onShowVideoWindow();
        void onAppConfig();

    public:
        MainWindow(QApplication* _app);
        ~MainWindow();

        void openGallery(const QString& _aimId, const Data::Image& _image, const QString& _localPath);
        void closeGallery();

        void playVideo(const QString& _path);
        void closeVideo();

        void activateFromEventLoop();
        bool isActive() const;
        bool isMainPage() const;
        void setBackgroundPixmap(QPixmap& _pixmap, const bool _tiling);

        int getScreen() const;
        void skipRead(); //skip next sending last read by window activation

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        MainPage* getMainPage() const;

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        void showSidebar(const QString& _aimId, int _page);
        void setSidebarVisible(bool _show);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void showMenuBarIcon(bool _show);

    private:
        void initSizes();
        void initSettings();

    protected:
        bool nativeEventFilter(const QByteArray &, void* _message, long* _result);

        virtual void enterEvent(QEvent* _event);
        virtual void leaveEvent(QEvent* _event);
        virtual void resizeEvent(QResizeEvent* _event);
        virtual void moveEvent(QMoveEvent* _event);
        virtual void changeEvent(QEvent* _event);
        virtual void closeEvent(QCloseEvent* _event);
        virtual void keyPressEvent(QKeyEvent* _event);
        void hideTaskbarIcon();
        void showTaskbarIcon();
        void clear_global_objects();

    private:
        Previewer::GalleryWidget* gallery_;
        MainPage* mainPage_;
        LoginPage* loginPage_;
        PromoPage* promoPage_;
        QApplication* app_;
        TitleWidgetEventFilter* eventFilter_;
        TrayIcon* trayIcon_;
        QPixmap backgroundPixmap_;
        QWidget *mainWidget_;
        QVBoxLayout *mainLayout_;
        QWidget *titleWidget_;
        QHBoxLayout *titleLayout_;
        QPushButton *logo_;
        QLabel *title_;
        QSpacerItem *spacer_;
        QPushButton *hideButton_;
        QPushButton *maximizeButton_;
        QPushButton *closeButton_;
        BackgroundWidget *stackedWidget_;
        ShadowWindow* Shadow_;
        CallPanelMainEx* callPanelMainEx;
        MultimediaViewer* mplayer_;

        bool SkipRead_;
        bool TaskBarIconHidden_;

#ifdef _WIN32
        HWND fake_parent_window_;
#endif //_WIN32
#ifdef __APPLE__
        MacSupport* getMacSupport();
        AccountsPage* accounts_page_;
#endif
	};
}