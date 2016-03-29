#pragma once

namespace Ui
{
    class MainWindow;
    class HistoryControlPage;
    class ContactDialog;
}

namespace Utils
{
    enum class CommonSettingsType
    {
        min = -1,
        CommonSettingsType_None,
        CommonSettingsType_Profile,
        CommonSettingsType_General,
        CommonSettingsType_VoiceVideo,
        CommonSettingsType_Notifications,
        CommonSettingsType_Themes,
        CommonSettingsType_About,
        CommonSettingsType_ContactUs,
        CommonSettingsType_AttachPhone,
        CommonSettingsType_AttachUin,

        max
    };

    class InterConnector : public QObject
    {
        Q_OBJECT

    public:

Q_SIGNALS:
        void profileSettingsShow(QString uin);
        void profileSettingsBack();

        void generalSettingsShow(int type);
        void generalSettingsBack();
        void themesSettingsShow(bool, QString);
        void themesSettingsBack();
        void profileSettingsDoMessage(QString uin);
        void profileSettingsUnknownAdd(QString uin);
        void profileSettingsUnknownIgnore(QString uin);
        void profileSettingsUnknownSpam(QString uin);
        void profileSettingsUpdateInterface();

        void attachPhoneBack();
        void attachUinBack();

        void makeSearchWidgetVisible(bool);
        void showIconInTaskbar(bool _show);

        void popPagesToRoot();

        void showNoContactsYetSuggestions();
        void hideNoContactsYetSuggestions();
        void showNoContactsYet();
        void hideNoContactsYet();
        void showNoRecentsYet();
        void hideNoRecentsYet();

        void onThemes();
        void cancelTheme(QString);
        void setToAllTheme(QString);
        void setTheme(QString);

        void closeAnyPopupWindow();

        void forceRefreshList(QAbstractItemModel *, bool);
        
    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        Ui::MainWindow* getMainWindow() const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        Ui::MainWindow* MainWindow_;
    };
}
