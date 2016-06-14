#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"

namespace Logic
{
    class ChatMembersModel;
    class ContactListItemDelegate;
}

namespace Ui
{
    class CustomButton;
    class ContactAvatarWidget;
    class TextEditEx;
    class BackButton;
    class ContactList;
    class SearchWidget;
    class LabelEx;
    class LineWidget;
    class ClickedWidget;
    class ActionButton;
    class FocusableListView;

    class AddToChat : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();
    public:
        AddToChat(QWidget* _parent);

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void mouseReleaseEvent(QMouseEvent *);
        virtual void enterEvent(QEvent *);
        virtual void leaveEvent(QEvent *);

    private:
        QPainter* painter_;
        bool Hovered_;
    };

    class MenuPage : public SidebarPage
    {
        Q_OBJECT
    Q_SIGNALS:
        void updateMembers();
        void needUpdate();

    public:
        MenuPage(QWidget* parent);
        virtual void initFor(const QString& aimId);

    public Q_SLOTS:
        void allMemebersClicked();

    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
        virtual void updateWidth();

    private Q_SLOTS:
        void contactChanged(QString);
        void favoritesClicked();
        void themesClicked();
        void eraseHistoryClicked();
        void ignoreClicked();
        void quitClicked();
        void notificationsChecked(int);
        void scrollChaged(int, int);
        void updateSize();
        void addToChatClicked();
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatBlocked(QList<Data::ChatMemberInfo>);
        void spamClicked();
        void addContactClicked();
        void contactClicked(QString);
        void searchBegin();
        void searchEnd();
        void backButtonClicked();
        void membersClicked(const QModelIndex &);
        void moreClicked();
        void adminsClicked();
        void blockedClicked();
        void avatarClicked();
        void chatEvent(QString);
        void menu(QAction*);
        void actionResult(int);

    private:
        void init();
        void initAvatarAndName();
        void initAddContactAndSpam();
        void initFavoriteNotificationsTheme();
        void initChatMembers();
        void initEraseIgnoreDelete();
        void initListWidget();
        void connectSignals();
        void initDescription(const QString& description, bool full = false);
        void blockUser(const QString& aimId, bool block);
        void changeRole(const QString& aimId, bool moder);

    private:
        QString currentAimId_;
        ContactAvatarWidget* avatar_;
        TextEditEx* name_;
        TextEditEx* description_;
        LineWidget* firstLine_;
        LineWidget* secondLine_;
        LineWidget* thirdLine_;
        QWidget* adminsSpacer_;
        QWidget* blockSpacer_;
        QWidget* addContactSpacerTop_;
        QWidget* addContactSpacer_;
        QWidget* labelsSpacer_;
        QWidget* mainWidget_;
        QWidget* listWidget_;
        QWidget* textTopSpace_;
        CustomButton* notificationsButton_;
        ActionButton* favoriteButton_;
        ActionButton* themesButton_;
        ActionButton* eraseHistoryButton_;
        ActionButton* ignoreButton_;
        ActionButton* quitAndDeleteButton_;
        CustomButton* addContact_;
        ActionButton* spamButton_;
        CustomButton* backButton_;
        QCheckBox* notificationsCheckbox_;
        ClickedWidget* allMembers_;
        ClickedWidget* admins_;
        ClickedWidget* blockList_;
        ClickedWidget* avatarName_;
        QLabel* allMembersCount_;
        QLabel* blockCount_;
        QLabel* blockLabel_;
        QLabel* listLabel_;
        QLabel* allMembersLabel_;
        FocusableListView* members_;
        Logic::ChatMembersModel* chatMembersModel_;
        Logic::ContactListItemDelegate* delegate_;
        QSpacerItem* bottomSpacer_;
        AddToChat* addToChat_;
        std::shared_ptr<Data::ChatInfo> info_;
        LabelEx* moreLabel_;
        ContactList* cl_;
        SearchWidget* searchWidget_;
        QStackedWidget* stackedWidget_;
        QVBoxLayout* rootLayot_;
        int currentTab_;
    };
}
