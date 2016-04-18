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

    enum class PlaceholdersType
    {
        min = -1,

        PlaceholdersType_FindFriend,
        PlaceholdersType_HideFindFriend,

        PlaceholdersType_IntroduceYourself,
        PlaceholdersType_HideIntroduceYourself,
        PlaceholdersType_SetExistanseOnIntroduceYourself,
        PlaceholdersType_SetExistanseOffIntroduceYourself,

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

        void showPlaceholder(PlaceholdersType);
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
        void updateFocus();
        
        void schemeUrlClicked(QString);
        
    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        Ui::MainWindow* getMainWindow() const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;

        void setDragOverlay(bool enable);
        bool isDragOverlay() const;

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;
    };
}
