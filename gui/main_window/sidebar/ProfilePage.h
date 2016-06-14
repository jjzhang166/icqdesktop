#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"
#include "../../../corelib/enumerations.h"

namespace Ui
{
    class ContactAvatarWidget;
    class CustomButton;
    class TextEditEx;
    class LabelEx;
    class LiveChatMembersControl;
    class TextEmojiWidget;
    class LineWidget;
    class ActionButton;
    class FlatMenu;

    class InfoPlate : public QWidget
    {
        Q_OBJECT
    public:
        InfoPlate(QWidget* parent, int leftMargin);
        void setHeader(const QString& header);
        void setInfo(const QString& info);
        void elideText(int width);
        void setAttachPhone(bool value);

    private Q_SLOTS:
        void menuRequested();
        void menu(QAction*);

    private:
        QLabel* header_;
        TextEmojiWidget* info_;
        QLabel* phoneInfo_;
        QString infoStr_;
        bool attachPhone_;
    };

    class ProfilePage : public SidebarPage
    {
        Q_OBJECT
    public:
        ProfilePage(QWidget* parent);
        virtual void initFor(const QString& aimId);

    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
        virtual void updateWidth();

    private Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void moreClicked();
        void removeFromIgnore();
        void contactChanged(QString);
        void contactRemoved(QString);
        void changed();
        void addClicked();
        void chatClicked();
        void callClicked();
        void videoClicked();
        void back();
        void ignore();
        void spam();
        void remove();
        void quit();
        void rename();
        void menuStateOnline();
        void menuStateDoNotDisturb();
        void menuStateInvisible();
        void ignoreList();
        void editClicked();
        void attachOld();
        void recvFlags(int);
        void statusClicked();
        void changeDesc();
        void publicChanged(int);
        void avatarChanged();

    private:
        void init();
        void initDescription(const QString& description, bool full = false);
        void updateStatus();
        void setState(const core::profile_state _state);

    private:
        QString currentAimId_;
        ContactAvatarWidget* avatar_;
        CustomButton* backButton_;
        TextEditEx* name_;
        TextEditEx* description_;
        LabelEx* moreLabel_;
        QWidget* buttonsMargin_;
        QWidget* ignoreWidget_;
        QWidget* buttonWidget_;
        LineWidget* firstLine_;
        LineWidget* secondLine_;
        QWidget* rightWidget_;
        QWidget* avatarBottomSpace_;
        LabelEx* ignoreLabel_;
        LabelEx* editLabel_;
        std::shared_ptr<Data::ChatInfo> info_;
        CustomButton* addButton_;
        CustomButton* chatButton_;
        CustomButton* callButton_;
        CustomButton* videoCall_button_;
        ActionButton* renameContact_;
        ActionButton* ignoreButton_;
        ActionButton* spamButton_;
        ActionButton* deleteButton_;
        ActionButton* quiAndDelete;
        ActionButton* renameButton_;
        ActionButton* ignoreListButton;
        ActionButton* attachOldAcc;
        ActionButton* changeDescription;
        CustomButton* publicButton_;
        QCheckBox* publicCheckBox_;
        InfoPlate* uin_;
        InfoPlate* phone_;
        InfoPlate* firstName_;
        InfoPlate* lastName_;
        InfoPlate* nickName_;
        LiveChatMembersControl* members_;
        QWidget* membersTopSpace_;
        QWidget* membersBottomSpace_;
        QWidget* nameMargin_;
        QWidget* statusWidget_;
        QPushButton* statusButton_;
        FlatMenu* statusMenu_;
        LabelEx* statusLabel_;
        QLabel* membersLabel_;
        QVBoxLayout* subBackButtonLayout_;
        QVBoxLayout* subAvatarLayout_;
        QVBoxLayout* mainBackButtonLayout_;
        QVBoxLayout* mainAvatarLayout_;
        QHBoxLayout* nameLayout_;
        QVBoxLayout* editLayout_;
        QVBoxLayout* subEditLayout_;
        bool connectOldVisible_;
    };
}
