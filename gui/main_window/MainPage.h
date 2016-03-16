#pragma once

#include "../utils/InterConnector.h"

class QStandardItemModel;

namespace voip_manager {
    struct Contact;
    struct ContactEx;
}

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
	class main_page;
    class WidgetsNavigator;
	class ContactList;
	class SearchWidget;
	class CountrySearchCombobox;
    class VideoWindow;
    class VideoSettings;
    class IncomingCallWindow;
    class CallPanelMain;
	class ContactDialog;
	class SearchContactsWidget;
    class ProfileSettingsWidget;
    class GeneralSettingsWidget;
	class SelectContactsWidget;
    class HistoryControlPage;
    class ThemesSettingsWidget;
    class ContextMenu;

	class MainPage : public QWidget
	{
		Q_OBJECT
	private Q_SLOTS:
		void searchBegin();
		void searchEnd();
		void onContactSelected(QString _contact);
		void onAddContactClicked();
        void onFinishSelectMembers(bool _isAccept);
        void addButtonClicked();
        void createGroupChat();
		void clearSearchMembers();
        // settings
        void onProfileSettingsShow(QString uin);
        void onGeneralSettingsShow(int type);
        void onThemesSettingsShow(bool,QString);
        //voip
        void onVoipShowVideoWindow(bool);
        void onVoipCallIncoming(const std::string&, const std::string&);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& contact_ex);
        void onVoipCallCreated(const voip_manager::ContactEx& contact_ex);
        void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);

        void hideNoContactsYetSuggestions();
        void showNoContactsYetSuggestions();

        void show_popup_menu(QAction* _action);
        void post_stats_with_settings();

    private:
		MainPage(QWidget* parent);
        static MainPage* _instance;

    public:
        static MainPage* instance(QWidget* _parent = 0);
        static void reset();
		~MainPage();
        void setSearchFocus();
        void selectRecentChat(QString aimId);
        void recentsTabActivate(bool selectUnread = false);
        void contactListActivate(bool addContact = false);
        void settingsTabActivate(Utils::CommonSettingsType item = Utils::CommonSettingsType::CommonSettingsType_None);
        void hideInput();
        void cancelSelection();
        void createGroupChat(QStringList _members_aimIds);

        void raiseVideoWindow();

        ContactDialog* getContactDialog() const;
        HistoryControlPage* getHistoryPage(const QString& aimId) const;

	protected:
		virtual void resizeEvent(QResizeEvent* event);

	private:

        QWidget* showNoContactsYetSuggestions(QWidget *parent, std::function<void()> addNewContactsRoutine);

	private:
		ContactList*                contact_list_widget_;
		SearchWidget*               search_widget_;
		CallPanelMain*              video_panel_;
		VideoWindow*                video_window_;
        VideoSettings*              video_settings_;
		WidgetsNavigator*           pages_;
		ContactDialog*              contact_dialog_;
		QVBoxLayout*                pages_layout_;
        SearchContactsWidget*       search_contacts_;
        GeneralSettingsWidget*      general_settings_;
		ProfileSettingsWidget*      profile_settings_;
        ThemesSettingsWidget*       themes_settings_;
        QHBoxLayout*                horizontal_layout_;
        QWidget* noContactsYetSuggestions_;
        ContextMenu*                add_contact_menu_;
        QTimer*                     settings_timer_;
        std::map<std::string, std::shared_ptr<IncomingCallWindow> > incoming_call_windows_;
		void _destroy_incoming_call_window(const std::string& account, const std::string& contact);
	};
}