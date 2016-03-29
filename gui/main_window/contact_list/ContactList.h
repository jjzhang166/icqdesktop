#pragma once

namespace Logic
{
	class RecentItemDelegate;
	class ContactListItemDelegate;
    class ChatMembersModel;
    class AbstractSearchModel;

	enum MembersWidgetRegim {CONTACT_LIST, SELECT_MEMBERS, DELETE_MEMBERS, IGNORE_LIST};
    
    bool is_delete_members(int _regim);
}

namespace Data
{
	class Contact;
}

namespace Ui
{
	class contact_list;
    class SettingsTab;
    class ContextMenu;
    class ContactList;

    class AddContactButton : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void clicked();
    public:
        AddContactButton(QWidget* parent);

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void enterEvent(QEvent *);
        virtual void leaveEvent(QEvent *);
        virtual void mousePressEvent(QMouseEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        QPainter* Painter_;
        bool Hover_;
        bool Select_;
    };

    class EmptyIgnoreListLabel : public QWidget
    {
        Q_OBJECT
    public:
        EmptyIgnoreListLabel(QWidget* parent);

    protected:
        virtual void paintEvent(QPaintEvent*);

    private:
        QPainter* Painter_;
    };

    class ButtonEventFilter : public QObject
    {
        Q_OBJECT
    public:
        ButtonEventFilter();
    protected:
        bool eventFilter(QObject* obj, QEvent* event);
    };

    class RCLEventFilter : public QObject
    {
        Q_OBJECT

    public:
        RCLEventFilter(ContactList* cl);
    protected:
        bool eventFilter(QObject* obj, QEvent* event);
    private:
        ContactList* Cl_;
    };

	enum CurrentTab
	{
		RECENTS = 0,
		ALL,
		SETTINGS,
		SEARCH,
	};
    
	class RecentsButton : public QPushButton
	{
		Q_OBJECT
	public:
		RecentsButton(QWidget* parent);

	protected:
		void paintEvent(QPaintEvent *);

	private:
		QPainter* Painter_;
	};

    class FocusableListView: public QListView
    {
    public:
        FocusableListView(QWidget *parent = 0);
        ~FocusableListView();
        
    protected:
        virtual void enterEvent(QEvent *e) override;
        virtual void leaveEvent(QEvent *e) override;
    };
    
	class ContactList : public QWidget
	{
		Q_OBJECT
        
	Q_SIGNALS:
		void itemSelected(QString);
		void groupClicked(int);
		void searchEnd();
		void addContactClicked();
        void needSwitchToRecents();

	public Q_SLOTS:
		void searchResult();
		void searchUpPressed();
		void searchDownPressed();
        void onSendMessage(QString);
        void select(QString);

        void changeSelected(QString _aimId, bool _isRecent);
		void recentsClicked();
		void allClicked();
		void settingsClicked();
        void switchToRecents();
        
	private Q_SLOTS:
		void searchResults(const QModelIndex &, const QModelIndex &);
		void searchResultsFromModel();
		void itemClicked(const QModelIndex&);
		void itemPressed(const QModelIndex&);
		void statsRecentItemPressed(const QModelIndex&);
		void statsSearchItemPressed(const QModelIndex&);
		void statsCLItemPressed(const QModelIndex&);
		void searchClicked(const QModelIndex&);
        
        void guiSettingsChanged();
		void recentOrderChanged();
		void touchScrollStateChangedRecents(QScroller::State);
		void touchScrollStateChangedCl(QScroller::State);
        void showNoContactsYet();
        void showNoRecentsYet();
        void hideNoContactsYet();
        void hideNoRecentsYet();

        void typingAimId(QString, QString);
        void typingName(QString, QString);
        void stopTypingAimId(QString, QString);
        void stopTypingName(QString, QString);
        void messagesReceived(QString, QVector<QString>);
        
		void show_popup_menu(QAction* _action);

	public:

		ContactList(QWidget* parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _chatMembersModel);
		~ContactList();

		void setSearchMode(bool);
		bool isSearchMode() const;
		bool isContactListMode() const;
        bool shouldHideSearch() const;
		void changeTab(CurrentTab _curr_tab);
        void triggerTapAndHold(bool value);
        bool tapAndHoldModifier() const;
        void dragPositionUpdate(const QPoint& pos);
        void dropFiles(const QPoint& pos, const QList<QUrl> files);
        void show_contact_list();
        void setEmptyIgnoreLabelVisible(bool _is_visible);

	private:

		void updateTabState(bool save);

        void UpdateCheckedButtons();

        void updateSettingsState();
		void show_recents_popup_menu(const QModelIndex& _current);
		void show_contacts_popup_menu(const QModelIndex& _current);
		void selectionChanged(const QModelIndex &);

        QString getAimid(const QModelIndex & current);
        void searchUpOrDownPressed(bool _isUp);

        void showNoRecentsYet(QWidget *parent, QWidget *list, QLayout *layout, std::function<void()> action);
        void showNoContactsYet(QWidget *parent, QWidget *list, QLayout *layout);
        void hideNoContactsYet(QWidget *list, QLayout *layout);
        void hideNoRecentsYet(QWidget *list, QLayout *layout);


		Ui::ContextMenu*	popup_menu_;

		unsigned CurrentTab_;

		Logic::RecentItemDelegate*		recents_delegate_;
		Logic::ContactListItemDelegate*	cl_delegate_;
		RecentsButton*					recents_button_;
        SettingsTab*					settings_tab_;
		Logic::MembersWidgetRegim       regim_;
        Logic::ChatMembersModel*        chat_members_model_;
        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QVBoxLayout *vertical_layout_3_;
        QVBoxLayout *vertical_layout_4_;
        QHBoxLayout *horizontal_layout_;
        QHBoxLayout *horizontal_layout_2_;
        QSpacerItem *horizontal_spacer_;
        QSpacerItem *horizontal_spacer_2_;
        QStackedWidget *stacked_widget_;
        QWidget *recents_page_;
        FocusableListView *recents_view_;
        QWidget *contact_list_page_;
        FocusableListView *contact_list_view_;
        QWidget *search_page_;
        FocusableListView *search_view_;
        QWidget *widget_;
        QFrame *frame_;
        QPushButton *all_tab_button_;
        QPushButton *settings_tab_button_;
        QSpacerItem *vertical_spacer_;
        bool noContactsYetShown_;
        bool noRecentsYetShown_;
        QWidget *noContactsYet_;
        QWidget *noRecentsYet_;
        bool TapAndHold_;
        RCLEventFilter* ListEventFilter_;
        ButtonEventFilter* ButtonEventFilter_;
        EmptyIgnoreListLabel* empty_ignore_list_label_;
	};
}