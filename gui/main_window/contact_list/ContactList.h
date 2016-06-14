#pragma once

#include "../../types/typing.h"

namespace Logic
{
	class RecentItemDelegate;
	class ContactListItemDelegate;
    class ChatMembersModel;
    class AbstractSearchModel;

	enum MembersWidgetRegim {CONTACT_LIST, SELECT_MEMBERS, DELETE_MEMBERS, IGNORE_LIST, ADMIN_MEMBERS};
    
    bool is_delete_members_regim(int _regim);
    bool is_admin_members_regim(int _regim);
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
        AddContactButton(QWidget* _parent);

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void enterEvent(QEvent *);
        virtual void leaveEvent(QEvent *);
        virtual void mousePressEvent(QMouseEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        QPainter* painter_;
        bool hover_;
        bool select_;
    };

    class EmptyIgnoreListLabel : public QWidget
    {
        Q_OBJECT
    public:
        EmptyIgnoreListLabel(QWidget* _parent);

    protected:
        virtual void paintEvent(QPaintEvent*);

    private:
        QPainter* painter_;
    };

    class ButtonEventFilter : public QObject
    {
        Q_OBJECT
    public:
        ButtonEventFilter();
    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);
    };

    class RCLEventFilter : public QObject
    {
        Q_OBJECT

    public:
        RCLEventFilter(ContactList* _cl);
    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);
    private:
        ContactList* cl_;
    };

	enum CurrentTab
	{
		RECENTS = 0,
		ALL,
        LIVE_CHATS,
		SETTINGS,
		SEARCH,
	};
    
	class RecentsButton : public QPushButton
	{
		Q_OBJECT
	public:
		RecentsButton(QWidget* _parent);

	protected:
		void paintEvent(QPaintEvent *);

	private:
		QPainter* painter_;
	};

    class FocusableListView: public QListView
    {
    public:
        FocusableListView(QWidget *_parent = 0);
        ~FocusableListView();
        
    protected:
        virtual void enterEvent(QEvent *_e) override;
        virtual void leaveEvent(QEvent *_e) override;
    };
    
	class ContactList : public QWidget
	{
		Q_OBJECT
        
	Q_SIGNALS:
		void itemSelected(QString);
        void itemClicked(QString);
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
        void liveChatsClicked();
		void allClicked();
		void settingsClicked();
        void switchToRecents();
        
	private Q_SLOTS:
		void searchResults(const QModelIndex &, const QModelIndex &);
		void searchResultsFromModel();
		void itemClicked(const QModelIndex&);
		void itemPressed(const QModelIndex&);
        void liveChatsItemPressed(const QModelIndex&);
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

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);

        void messagesReceived(QString, QVector<QString>);
		void showPopupMenu(QAction* _action);
        void switchTab(QString);

	public:

		ContactList(QWidget* _parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _chatMembersModel);
		~ContactList();

		void setSearchMode(bool);
		bool isSearchMode() const;
		bool isContactListMode() const;
        bool shouldHideSearch() const;
		void changeTab(CurrentTab _currTab);
        void triggerTapAndHold(bool _value);
        bool tapAndHoldModifier() const;
        void dragPositionUpdate(const QPoint& _pos);
        void dropFiles(const QPoint& _pos, const QList<QUrl> _files);
        void showContactList();
        void setEmptyIgnoreLabelVisible(bool _is_visible);
        void setClDelegate(Logic::ContactListItemDelegate* delegate);
        void setTransparent(bool transparent);
        void clearSettingsSelection();

	private:

		void updateTabState(bool _save);
        void updateCheckedButtons();
        void updateSettingsState();
		void showRecentsPopup_menu(const QModelIndex& _current);
		void showContactsPopupMenu(const QModelIndex& _current);
		void selectionChanged(const QModelIndex &);

        QString getAimid(const QModelIndex &_current);
        void searchUpOrDownPressed(bool _isUp);

        void showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action);
        void showNoContactsYet(QWidget *_parent, QWidget *_list, QLayout *_layout);
        void hideNoContactsYet(QWidget *_list, QLayout *_layout);
        void hideNoRecentsYet(QWidget *_list, QLayout *_layout);

    private:

		Ui::ContextMenu* popupMenu_;
		Logic::RecentItemDelegate* recentsDelegate_;
		Logic::ContactListItemDelegate*	clDelegate_;
		RecentsButton* recentsButton_;
        QPushButton*	livechatsButton_;
        SettingsTab* settingsTab_;
		Logic::MembersWidgetRegim regim_;
        Logic::ChatMembersModel* chatMembersModel_;
        QVBoxLayout *contactListLayout_;
        QVBoxLayout *recentsLayout_;
        QStackedWidget *stackedWidget_;
        QWidget *recentsPage_;
        FocusableListView *recentsView_;
        QWidget *contactListPage_;
        FocusableListView *contactListView_;
        FocusableListView *searchView_;
        QWidget* liveChatsPage_;
        FocusableListView *liveChatsView_;
        QPushButton *clTabButton_;
        QPushButton *settingsTabButton_;
        QWidget *noContactsYet_;
        QWidget *noRecentsYet_;
        RCLEventFilter* listEventFilter_;
        ButtonEventFilter* buttonEventFilter_;
        EmptyIgnoreListLabel* emptyIgnoreListLabel_;

        unsigned currentTab_;
        bool noContactsYetShown_;
        bool noRecentsYetShown_;
        bool tapAndHold_;
	};
}