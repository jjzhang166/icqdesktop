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

class QScrollArea;

namespace core
{
    enum class group_chat_info_errors;
}

namespace Ui
{
	class history_control_page;
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
        ClickWidget(QWidget* parent);

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *);
    };

    class TopWidget : public QStackedWidget
    {
        Q_OBJECT
    public:
        TopWidget(QWidget* parent, const QString& aimId);

        void setWidgets(QWidget* main, QWidget* theme);
        void updateThemeWidget(bool toCurrent, ThemePanelCallback callback);
        void showThemeWidget(bool show);

    private:
        QString AimId_;
    };

	class MessagesWidgetEventFilter : public QObject
	{
		Q_OBJECT

	public:
		MessagesWidgetEventFilter(
			QWidget* buttonsWidget,
			const QString& contactName,
            QTextBrowser *contactNameWidget,
            MessagesScrollArea *scrollArea,
			QWidget* firstOverlay,
			QWidget* secondOverlay,
			NewMessagesPlate* newMessaesPlate,
			HistoryControlPage* dialog
        );

		void resetNewPlate();

        void ResetContactName(QString _contact_name);

        QString getContactName() const;

	protected:
		bool eventFilter(QObject* obj, QEvent* event);

	private:
		QWidget* ButtonsWidget_;

		MessagesScrollArea* ScrollArea_;
        MessagesWidget* MessagesWidget_;

		QScrollArea* BottomWidget_;

		QWidget* FirstOverlay_;
		QWidget* SecondOverlay_;
		QDate Date_;
		int Width_;
		bool NewPlateShowed_;
		QTextBrowser* ContactNameWidget_;
		QString ContactName_;
		HistoryControlPage* Dialog_;
		NewMessagesPlate* NewMessagesPlate_;
		QPoint MousePos_;
		QTimer* Timer_;
		bool ScrollDirectionDown_;
	};

	struct ItemData
    {
        ItemData(const Logic::MessageKey& key, QWidget* widget, unsigned mode)
            : Key_(key)
            , Widget_(widget)
            , Mode_(mode)
        {
        }

        Logic::MessageKey Key_;
        QWidget* Widget_;
        unsigned Mode_;
    };

    namespace themes
    {
        class theme;
    }

	class HistoryControlPage : public QWidget
	{
        enum class State;

        friend QTextStream& operator<<(QTextStream &oss, const State arg);

	    Q_OBJECT

	Q_SIGNALS:
		void requestMoreMessagesSignal();
		void insertNextMessageSignal();
		void needRemove(Logic::MessageKey);
		void quote(QString);
        void updateMembers();

	public Q_SLOTS:
		void messageKeyUpdated(QString, Logic::MessageKey);
        void scrollMovedToBottom();

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);
        void indentChanged(Logic::MessageKey, bool);

	public:
        HistoryControlPage(QWidget* parent, QString aimId);
        void updateTopThemeButtonsVisibility();

        ~HistoryControlPage();

		void updateNewPlate(bool);
		qint64 getNewPlateId() const;
		void newPlateShowed();
		bool newPlateBlocked() const;
		void open();
        bool requestMoreMessagesAsync(const char *dbgWhere);
        QString aimId() const;
        void cancelSelection();

        bool touchScrollInProgress() const;
        void updateWidgetsTheme();

        void showThemesTopPanel(bool _show, bool _showSetToCurrent, ThemePanelCallback callback);

        void update(QString);
        void updateMoreButton();

	protected:
		virtual void focusOutEvent(QFocusEvent* _event) override;
        virtual void wheelEvent(QWheelEvent *_event) override;
        virtual void showEvent(QShowEvent* _event) override;

	private Q_SLOTS:
		void callVideoButtonClicked();
		void callAudioButtonClicked();
		void moreButtonClicked();
		void sourceReady(QString aimId);
		void updated(QList<Logic::MessageKey>, QString, unsigned);
		void deleted(QList<Logic::MessageKey>, QString);
		void requestMoreMessagesSlot();
		void downPressed();
		void autoScroll(bool);
		void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatInfoFailed(qint64 seq, core::group_chat_info_errors);
        void updateChatInfo();
        void onReachedFetchingDistance();
        void fetchMore(QString);
        void nameClicked();
        void edit_members();

		void contactChanged(QString);
		void insertNextMessageSlot();
		void removeWidget(Logic::MessageKey);

		void copy(QString);
		void quoteText(QString);

		void contact_authorized(QString _aimid, bool _res);
		void auth_add_contact(QString _aimid);
		void auth_spam_contact(QString _aimid);
		void auth_delete_contact(QString _aimid);

		void add_member();
        void unloadWidgets(QList<Logic::MessageKey> keysToUnload);

        void stats_spam_profile(QString _aimid);
        void stats_spam_auth(QString _aimid);
        void stats_ignore_auth(QString _aimid);
        void stats_add_user_auth(QString _aimid);
        void stats_delete_contact_auth(QString _aimid);

        void readByClient(QString _aimid, qint64 _id);

	private:
	    void updateName();
		class PositionInfo;
        int set_theme_id_;
        QSpacerItem* h_spacer_3_;
		typedef std::shared_ptr<PositionInfo> PositionInfoSptr;
		typedef std::list<PositionInfoSptr> PositionInfoList;
		typedef PositionInfoList::iterator PositionInfoListIter;

		enum class WidgetRemovalResult
		{
			Min,

			Removed,
			NotFound,
			PersistentWidget,

			Max
		};

        struct TypingWidgets
        {
            TextEmojiWidget *twt;
            QLabel *twa;
            QMovie *twm;
        }
        typingWidgets_;

        QWidget *TypingWidget_;

        QSet< QString > typingChattersAimIds_;

        void updateTypingWidgets();
        void hideTypingWidgets();

		void initStatus();
		void appendAuthControlIfNeed();
		bool isScrolling() const;
		QWidget* getWidgetByKey(const Logic::MessageKey& key);
		WidgetRemovalResult removeExistingWidgetByKey(const Logic::MessageKey& key);
        void replaceExistingWidgetByKey(const Logic::MessageKey& key, QWidget* widget);

        void loadChatInfo(bool is_full_list_loaded_);
        void rename_chat();
        void rename_contact();

        void setState(const State state, const char *dbgWhere);
        bool isState(const State state) const;
        bool isStateFetching() const;
        bool isStateIdle() const;
        bool isStateInserting() const;
        void postInsertNextMessageSignal(const char *dbgWhere);
        void postponeMessagesRequest(const char *dbgWhere);
        void switchToIdleState(const char *dbgWhere);
        void switchToInsertingState(const char *dbgWhere);
        void switchToFetchingState(const char *dbgWhere);
        void setContactStatusClickable(bool _is_enabled);

        bool isChatAdmin() const;

        void onDeleteHistory();

		QString									aimId_;
		ServiceMessageItem*						messages_overlay_first_;
		ServiceMessageItem*						messages_overlay_second_;
		qint64									new_plate_position_;
		MessagesWidgetEventFilter*				event_filter_;
		NewMessagesPlate*						new_messages_plate_;
		qint64									chat_info_sequence_;
		AuthWidget*								auth_widget_;
		qint32									next_local_position_;
        std::set<Logic::MessageKey>				remove_requests_;
        std::list<ItemData>                     items_data_;
        LabelEx*                                contact_status_;
        Logic::ChatMembersModel*                chat_members_model_;
        QPushButton*                            favorite_star_;
        QPushButton*                            official_mark_;
        MessagesScrollArea*						messages_area_;

        State                                   state_;

        bool                                    is_messages_request_postponed_;
        char const*                             dbg_where_postponed_;

        TopWidget* top_widget_;
        QWidget *contact_widget_;
        QTextBrowser *contact_name_;
        QWidget* contact_status_widget_;
        QSpacerItem *horizontalSpacer_contact_status_;
        QSpacerItem *vertical_spacer_;
        QPushButton *call_button_;
        QPushButton *video_call_button_;
		QPushButton *add_member_button_;
        QPushButton *more_button_;
        bool is_contact_status_clickable_;
        bool is_public_chat_;
	};
}