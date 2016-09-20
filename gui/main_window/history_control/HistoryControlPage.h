#pragma once

#include "MessagesModel.h"

#include "../../types/chat.h"
#include "../../types/typing.h"
#include "../contact_list/ChatMembersModel.h"
#include "../../../corelib/enumerations.h"

namespace Logic
{
	class MessageItemDelegate;
	class MessagesModel;
    class ChatMembersModel;
}

namespace core
{
    enum class group_chat_info_errors;
}

namespace Ui
{
	class ServiceMessageItem;
	class HistoryControlPage;
	class NewMessagesPlate;
	class AuthWidget;
    class LabelEx;
    class TextEmojiWidget;
    class MessagesWidget;
    class MessagesScrollArea;
    class HistoryControlPageThemePanel;

    enum ThemePanelChoice: int;
    typedef std::function<void(ThemePanelChoice)> ThemePanelCallback;

    class ClickWidget : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        ClickWidget(QWidget* _parent);

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *);
    };

    class TopWidget : public QStackedWidget
    {
        Q_OBJECT
    public:
        explicit TopWidget(QWidget* _parent);

        enum Widgets
        {
            Main = 0,
            Theme,
            Selection
        };

        void showThemeWidget(bool _toCurrent, ThemePanelCallback _callback);

        void showSelectionWidget();
        void hideSelectionWidget();

    private:
        int lastIndex_;
    };

	class MessagesWidgetEventFilter : public QObject
	{
		Q_OBJECT

	public:
		MessagesWidgetEventFilter(
			QWidget* _buttonsWidget,
			const QString& _contactName,
            QTextBrowser* _contactNameWidget,
            MessagesScrollArea* _scrollArea,
			QWidget* _firstOverlay,
			QWidget* _secondOverlay,
			NewMessagesPlate* _newMessaesPlate,
			HistoryControlPage* _dialog
        );

		void resetNewPlate();

        void ResetContactName(QString _contactName);

        void updateSizes();

        QString getContactName() const;

	protected:
		bool eventFilter(QObject* _obj, QEvent* _event);

	private:
		QWidget* ButtonsWidget_;

		MessagesScrollArea* ScrollArea_;
        MessagesWidget* MessagesWidget_;

        bool NewPlateShowed_;
        bool ScrollDirectionDown_;
        int Width_;
        HistoryControlPage* Dialog_;
        NewMessagesPlate* NewMessagesPlate_;
        QDate Date_;
        QPoint MousePos_;
        QString ContactName_;
        QTextBrowser* ContactNameWidget_;
        QTimer* Timer_;
        QWidget* FirstOverlay_;
		QWidget* SecondOverlay_;
	};

	struct ItemData
    {
        ItemData(
            const Logic::MessageKey& _key,
            QWidget* _widget,
            const unsigned _mode,
            const bool _isDeleted)
            : Key_(_key)
            , Widget_(_widget)
            , Mode_(_mode)
            , IsDeleted_(_isDeleted)
        {
        }

        Logic::MessageKey Key_;

        QWidget* Widget_;

        unsigned Mode_;

        bool IsDeleted_;
    };

    namespace themes
    {
        class theme;
    }

	class HistoryControlPage : public QWidget
	{
        enum class State;

        friend QTextStream& operator<<(QTextStream& _oss, const State _arg);

	    Q_OBJECT

	Q_SIGNALS:
		void requestMoreMessagesSignal();
		void insertNextMessageSignal();
		void needRemove(Logic::MessageKey);
		void quote(QList<Data::Quote>);
        void updateMembers();

	public Q_SLOTS:
		void messageKeyUpdated(QString, Logic::MessageKey);
        void scrollMovedToBottom();

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);
        void indentChanged(Logic::MessageKey, bool);
        void hasAvatarChanged(Logic::MessageKey, bool _hasAvatar);

	public:
        HistoryControlPage(QWidget* _parent, QString _aimId);
        void updateTopThemeButtonsVisibility();

        ~HistoryControlPage();

		void updateNewPlate(bool);
		qint64 getNewPlateId() const;
		void newPlateShowed();
		bool newPlateBlocked() const;
		void open();
        bool requestMoreMessagesAsync(const char* _dbgWhere);
        QString aimId() const;
        void cancelSelection();

        bool touchScrollInProgress() const;
        void updateWidgetsTheme();

        void showMainTopPanel();
        void showThemesTopPanel(bool _showSetToCurrent, ThemePanelCallback _callback);

        void update(QString);
        void updateMoreButton();

        bool contains(const QString& _aimId) const;

        void resumeVisibleItems();
        void suspendVisisbleItems();

        void scrollToBottom();

	protected:
		virtual void focusOutEvent(QFocusEvent* _event) override;
        virtual void wheelEvent(QWheelEvent *_event) override;
        virtual void showEvent(QShowEvent* _event) override;

	private Q_SLOTS:
		void callVideoButtonClicked();
		void callAudioButtonClicked();
		void moreButtonClicked();
		void sourceReady(QString _aimId);
		void updated(QList<Logic::MessageKey>, QString, unsigned);
		void deleted(QList<Logic::MessageKey>, QString);
		void requestMoreMessagesSlot();
		void downPressed();
		void autoScroll(bool);
		void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatInfoFailed(qint64 _seq, core::group_chat_info_errors);
        void updateChatInfo();
        void onReachedFetchingDistance();
        void fetchMore(QString);
        void nameClicked();
        void editMembers();

		void contactChanged(QString);
		void insertNextMessageSlot();
		void removeWidget(Logic::MessageKey);

		void copy(QString);
		void quoteText(QList<Data::Quote>);
        void forwardText(QList<Data::Quote>);

		void contactAuthorized(QString _aimId, bool _res);
		void authAddContact(QString _aimId);
		void authBlockContact(QString _aimId);
		void authDeleteContact(QString _aimId);

		void addMember();
        void unloadWidgets(QList<Logic::MessageKey> _keysToUnload);

        void stats_spam_profile(QString _aimId);
        void stats_spam_auth(QString _aimId);
        void stats_ignore_auth(QString _aimId);
        void stats_add_user_auth(QString _aimId);
        void stats_delete_contact_auth(QString _aimId);

        void readByClient(QString _aimid, qint64 _id);
        void adminMenuRequest(QString);
        void adminMenu(QAction* _action);
        void actionResult(int);

	private:
	    void updateName();
        void blockUser(const QString& _aimId, bool _blockUser);
        void changeRole(const QString& _aimId, bool _moder);

		class PositionInfo;
        int setThemeId_;
		typedef std::shared_ptr<PositionInfo> PositionInfoSptr;
		typedef std::list<PositionInfoSptr> PositionInfoList;
		typedef PositionInfoList::iterator PositionInfoListIter;

		enum class WidgetRemovalResult;

        struct TypingWidgets
        {
            TextEmojiWidget* twt;
            QLabel* twa;
            QMovie* twm;
        }
        typingWidgets_;
        QWidget *typingWidget_;

        QSet< QString > typingChattersAimIds_;

        void updateTypingWidgets();
        void hideTypingWidgets();

		void initStatus();
		void appendAuthControlIfNeed();
		bool isScrolling() const;
		QWidget* getWidgetByKey(const Logic::MessageKey& _key);
		WidgetRemovalResult removeExistingWidgetByKey(const Logic::MessageKey& _key);
        void replaceExistingWidgetByKey(const Logic::MessageKey& _key, QWidget* _widget);

        void loadChatInfo(bool _isFullListLoaded);
        void renameChat();
        void renameContact();

        void setState(const State _state, const char* _dbgWhere);
        bool isState(const State _state) const;
        bool isStateFetching() const;
        bool isStateIdle() const;
        bool isStateInserting() const;
        void postInsertNextMessageSignal(const char* _dbgWhere);
        void postponeMessagesRequest(const char* _dbgWhere);
        void switchToIdleState(const char* _dbgWhere);
        void switchToInsertingState(const char* _dbgWhere);
        void switchToFetchingState(const char* _dbgWhere);
        void setContactStatusClickable(bool _isEnabled);

        bool isChatAdmin() const;

        void onDeleteHistory();

        AuthWidget*								authWidget_;
        bool                                    isContactStatusClickable_;
        bool                                    isMessagesRequestPostponed_;
        bool                                    isPublicChat_;
        char const*                             dbgWherePostponed_;
        Logic::ChatMembersModel*                chatMembersModel_;
        LabelEx*                                contactStatus_;
        MessagesScrollArea*						messagesArea_;
        MessagesWidgetEventFilter*				eventFilter_;
        NewMessagesPlate*						newMessagesPlate_;
        qint32									nextLocalPosition_;
        qint64									newPlatePosition_;
        qint64									chatInfoSequence_;
        QPushButton*                            addMemberButton_;
        QPushButton*                            callButton_;
        QPushButton*                            favoriteStar_;
        QPushButton*                            moreButton_;
        QPushButton*                            officialMark_;
        QPushButton*                            videoCallButton_;
        QSpacerItem*                            verticalSpacer_;
        QString									aimId_;
        QTextBrowser*                           contactName_;
        QWidget*                                contactStatusWidget_;
        QWidget*                                contactWidget_;
        ServiceMessageItem*						messagesOverlayFirst_;
        ServiceMessageItem*						messagesOverlaySecond_;
        State                                   state_;
        std::list<ItemData>                     itemsData_;
        std::set<Logic::MessageKey>				removeRequests_;
        TopWidget*                              topWidget_;

	};
}
