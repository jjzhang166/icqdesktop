#pragma once

#include "MessagesModel.h"

#include "../../types/chat.h"
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
	class ContextMenu;
    class LabelEx;
    class TextEmojiWidget;
    class MessagesWidget;
    class MessagesScrollArea;
    class HistoryControlPageThemePanel;
    
    enum ThemePanelChoice: int;
    
	class MessagesWidgetEventFilter : public QObject
	{
		Q_OBJECT

	public:
		MessagesWidgetEventFilter(
			QWidget* top,
            QWidget* topTheme,
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
		QWidget* TopWidget_;

		MessagesScrollArea* ScrollArea_;
        MessagesWidget* MessagesWidget_;

        QWidget* TopThemeWidget_;
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
        
        void typingAimId(QString, QString);
        void typingName(QString, QString);
        void stopTypingAimId(QString, QString);
        void stopTypingName(QString, QString);

	public:
        HistoryControlPage(QWidget* parent, QString aimId);
        void updateTopThemeButtonsVisibility();

        void SetContactStatusClickable(bool _is_enabled);

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
        
        typedef std::function<void(ThemePanelChoice)> ThemePanelCallback;
        void showThemesTopPanel(bool _show, bool _showSetToCurrent, ThemePanelCallback callback);
        
        void update(QString);

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

		void contactChanged(QString);
		void insertNextMessageSlot();
		void removeWidget(Logic::MessageKey);
        void createMenu();
        void updateMenu(QString);

		void copy(QString);
		void quoteText(QString);

		void contact_authorized(QString _aimid, bool _res);
		void auth_add_contact(QString _aimid);
		void auth_spam_contact(QString _aimid);
		void auth_delete_contact(QString _aimid);

		void add_member();
		void edit_members();
		void popup_menu(QAction* _action);
        void needCleanup();

        void stats_spam_profile(QString _aimid);
        void stats_spam_auth(QString _aimid);
        void stats_ignore_auth(QString _aimid);
        void stats_add_user_auth(QString _aimid);
        void stats_delete_contact_auth(QString _aimid);

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

		void unloadWidgets();
		void initStatus();
		void appendAuthControlIfNeed();
		bool isScrolling() const;
		QWidget* getWidgetByKey(const Logic::MessageKey& key);
		WidgetRemovalResult removeExistingWidgetByKey(const Logic::MessageKey& key);
		bool hasMessageItemAt(const qint32 pos) const;
		void onFinishAddMembers(bool _isAccept);
		void clearAddMembers();
        void loadChatInfo(bool is_full_list_loaded_);
        void rename_chat();

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

		QString									aimId_;
		ServiceMessageItem*						messages_overlay_first_;
		ServiceMessageItem*						messages_overlay_second_;
		qint64									new_plate_position_;
		MessagesWidgetEventFilter*				event_filter_;
		NewMessagesPlate*						new_messages_plate_;
		qint64									chat_info_sequence_;
		AuthWidget*								auth_widget_;
		ContextMenu*							menu_;
		qint32									next_local_position_;
        std::set<Logic::MessageKey>				remove_requests_;
        std::list<ItemData>                     items_data_;
        LabelEx*                                contact_status_;
        Logic::ChatMembersModel*                chat_members_model_;
        QPushButton*                            edit_members_button_;
        QPushButton*                            favorite_star_;
        MessagesScrollArea*						messages_area_;

        State                                   state_;

        bool                                    is_messages_request_postponed_;
        char const*                             dbg_where_postponed_;

        QWidget *top_widget_;
        HistoryControlPageThemePanel *top_theme_widget_;
        QHBoxLayout *horizontal_layout_;
        QWidget *contact_widget_;
        QVBoxLayout *vertical_layout_;
        QTextBrowser *contact_name_;
        QHBoxLayout *horizontal_layout_2_;
        QHBoxLayout *contact_status_layout_;
        QWidget* contact_status_widget_;
        QSpacerItem *horizontalSpacer_contact_status_;
        QSpacerItem *horizontalSpacer_2_;
        QSpacerItem *vertical_spacer_;
        QWidget *buttons_widget_;
        QGridLayout *grid_layout_;
        QPushButton *call_button_;
        QPushButton *video_call_button_;
		QPushButton *add_member_button_;
        QPushButton *more_button_;
        QSpacerItem *vertical_spacer_2_;
        bool is_chat_member_;
        bool is_contact_status_clickable_;
        bool is_public_chat_;
	};
}