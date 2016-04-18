#pragma once

#include "../../types/typing.h"

namespace Logic
{
	class RecentItemDelegate;
	class ContactListItemDelegate;
    class ChatMembersModel;
    class AbstractSearchModel;

	enum MembersWidgetRegim {CONTACT_LIST, SELECT_MEMBERS, DELETE_MEMBERS, IGNORE_LIST};
    
    bool is_delete_members_regim(int _regim);
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
        QPainter* Painter_;
        bool Hover_;
        bool Select_;
    };

    class EmptyIgnoreListLabel : public QWidget
    {
        Q_OBJECT
    public:
        EmptyIgnoreListLabel(QWidget* _parent);

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
		RecentsButton(QWidget* _parent);

	protected:
		void paintEvent(QPaintEvent *);

	private:
		QPainter* Painter_;
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

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);

        void messagesReceived(QString, QVector<QString>);
        
		void showPopupMenu(QAction* _action);

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
        void show_contact_list();
        void setEmptyIgnoreLabelVisible(bool _is_visible);

	private:

		void updateTabState(bool _save);

        void UpdateCheckedButtons();

        void updateSettingsState();
		void show_recents_popup_menu(const QModelIndex& _current);
		void show_contacts_popup_menu(const QModelIndex& _current);
		void selectionChanged(const QModelIndex &);

        QString getAimid(const QModelIndex &_current);
        void searchUpOrDownPressed(bool _isUp);

        void showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action);
        void showNoContactsYet(QWidget *_parent, QWidget *_list, QLayout *_layout);
        void hideNoContactsYet(QWidget *_list, QLayout *_layout);
        void hideNoRecentsYet(QWidget *_list, QLayout *_layout);


		Ui::ContextMenu*	popupMenu_;

		unsigned CurrentTab_;

		Logic::RecentItemDelegate*		recentsDelegate_;
		Logic::ContactListItemDelegate*	clDelegate_;
		RecentsButton*					recentsTabButton_;
        SettingsTab*					settingsTab_;
		Logic::MembersWidgetRegim       regim_;
        Logic::ChatMembersModel*        chatMembersModel_;
        QVBoxLayout *mainLayout_;
        QVBoxLayout *contactListLayout_;
        QVBoxLayout *recentsLayout_;
        QVBoxLayout *searchLayout_;
        QHBoxLayout *buttonsLayout_;
        QSpacerItem *horizontal_spacer_;
        QSpacerItem *horizontal_spacer_2_;
        QStackedWidget *stackedWidget_;
        QWidget *recentsPage_;
        FocusableListView *recentsView_;
        QWidget *contactListPage_;
        FocusableListView *contactListView_;
        QWidget *searchPage_;
        FocusableListView *searchView_;
        QWidget *widget_;
        QFrame *buttonsFrame_;
        QPushButton *clTabButton_;
        QPushButton *settingsTabButton_;
        QSpacerItem *vertical_spacer_;
        bool noContactsYetShown_;
        bool noRecentsYetShown_;
        QWidget *noContactsYet_;
        QWidget *noRecentsYet_;
        bool TapAndHold_;
        RCLEventFilter* ListEventFilter_;
        ButtonEventFilter* ButtonEventFilter_;
        EmptyIgnoreListLabel* emptyIgnoreListLabel_;
	};
}