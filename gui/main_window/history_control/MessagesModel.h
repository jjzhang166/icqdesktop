#pragma once

#include "../../types/message.h"

namespace core
{
	enum class message_type;
}

namespace HistoryControl
{
	typedef std::shared_ptr<class FileSharingInfo> FileSharingInfoSptr;
	typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;
	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

}

namespace Ui
{
	class MessageItem;
	class ServiceMessageItem;
	class HistoryControlPageItem;
	enum class MessagesBuddiesOpt;
}

namespace Logic
{
    enum control_type
    {
        ct_date,
        ct_message
    };

	class MessageKey
	{
	public:
        static const MessageKey MAX;
        static const MessageKey MIN;

		MessageKey();

		MessageKey(qint64 id, qint64 prev, const QString& internalId, int pendingId, qint32 time, core::message_type type, bool outgoing, bool isPreview, control_type _control_type);

		bool isEmpty() const
		{
			return Id_ == -1 && Prev_ == -1 && InternalId_.isEmpty();
		}

		bool isPending() const
		{
			return Id_ == -1 && !InternalId_.isEmpty();
		}

		bool operator==(const MessageKey& other) const
		{
			const auto idsEqual = (hasId() && Id_ == other.Id_);
			const auto internalIdsEqual = (!InternalId_.isEmpty() && (InternalId_ == other.InternalId_));
			if (idsEqual || internalIdsEqual)
			{
				return control_type_ == other.control_type_;
			}

			return false;
		}

        bool operator!=(const MessageKey& other) const
        {
            return !operator==(other);
        }

		bool operator<(const MessageKey& other) const;

		bool hasId() const;

		bool checkInvariant() const;

		bool isChatEvent() const;

		bool isOutgoing() const;

		bool isFileSharing() const;

		bool isSticker() const;

        bool isDate() const;

		void setOutgoing(const bool isOutgoing);

		QString toLogStringShort() const;

		qint64 Id_;

		qint64 Prev_;

		QString InternalId_;

		core::message_type Type_;

        control_type control_type_;

        qint32 Time_;

		int PendingId_;

        bool IsPreview_;

	private:
		bool Outgoing_;

		bool compare(const MessageKey& rhs) const;

	};

	typedef std::set<MessageKey> MessageKeySet;

    typedef std::vector<MessageKey> MessageKeyVector;

	class InternalIndex
	{
	public:
		InternalIndex()
			: MaxTime_(-1)
			, MinTime_(-1)
			, LastIndex_(-1)
		{
		}

		bool operator==(const InternalIndex& other) const
		{
			return Key_ == other.Key_;
		}

		bool operator<(const InternalIndex& other) const
		{
			return Key_ < other.Key_;
		}

		bool contains(qint64 id) const
		{
			for (auto iter : MsgKeys_)
			{
				if (iter.Id_ == id)
					return true;
			}
			return false;
		}

		bool containsNotLast(qint64 id) const
		{
			int i = 0;
			for (auto iter : MsgKeys_)
			{
				++i;
				if (iter.Id_ == id)
					return i != MsgKeys_.size();
			}
			return false;
		}

		bool IsBase() const;

		bool IsChatEvent() const;

		bool IsDate() const;

		bool IsFileSharing() const;

		bool IsPending() const;

		bool IsStandalone() const;

		bool IsSticker() const;

        bool IsVoipEvent() const;

        bool IsPreview() const;

		const HistoryControl::ChatEventInfoSptr& GetChatEvent() const;

		const HistoryControl::FileSharingInfoSptr& GetFileSharing() const;

		const HistoryControl::StickerInfoSptr& GetSticker() const;

        const HistoryControl::VoipEventInfoSptr& GetVoipEvent() const;

		void SetChatEvent(const HistoryControl::ChatEventInfoSptr& info);

		void SetFileSharing(const HistoryControl::FileSharingInfoSptr& info);

		void SetSticker(const HistoryControl::StickerInfoSptr& info);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr& info);

		void InsertMessageKey(const MessageKey& key) const;

		void InsertMessageKeys(const MessageKeySet& keys) const;

		const MessageKeySet& GetMessageKeys() const;

		void RemoveMessageKey(MessageKey key) const;

		QString AimId_;
		MessageKey Key_;
		mutable qint32 MaxTime_;
		mutable qint32 MinTime_;
		QDate Date_;
		mutable QString ChatSender_;
		mutable QString ChatFriendly_;
		qint64 LastIndex_;

	private:
		HistoryControl::FileSharingInfoSptr FileSharing_;

		HistoryControl::StickerInfoSptr Sticker_;

		HistoryControl::ChatEventInfoSptr ChatEvent_;

        HistoryControl::VoipEventInfoSptr VoipEvent_;

		mutable MessageKeySet MsgKeys_;

	};

	class MessageInternal
	{
	public:
		bool operator<(const MessageInternal& other) const
		{
			return Key_ < other.Key_;
		}

		bool operator ==(const MessageInternal& other) const
		{
			return Key_ == other.Key_;
		}

		MessageInternal(const MessageKey& id)
			: Key_(id)
		{
		}

		MessageInternal(const MessageKey& id, Data::MessageBuddy* buddy)
			: Key_(id)
			, Buddy_(buddy)
		{
		}

		MessageInternal(std::shared_ptr<Data::MessageBuddy> buddy)
			: Buddy_(buddy)
		{
            Key_ = Buddy_->ToKey();
		}

		std::shared_ptr<Data::MessageBuddy> Buddy_;
		MessageKey Key_;
	};

	class MessagesModel : public QObject
	{
		Q_OBJECT

	public:
		enum UpdateMode
		{
			BASE = 0,
			NEW_PLATE,
			HOLE,
            PENDING,
            REQUESTED,
		};

	Q_SIGNALS:
		void ready(QString);
		void updated(QList<Logic::MessageKey>, QString, unsigned);
		void deleted(QList<Logic::MessageKey>, QString);
        void deliveredToClient(qint64);
		void deliveredToClient(QString);
		void deliveredToServer(QString);
		void messageIdFetched(QString, Logic::MessageKey);
        void pttPlayed(qint64);
        void canFetchMore(QString);

	private Q_SLOTS:
		void messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64);
		void dlgState(Data::DlgState);

	public Q_SLOTS:
		void contactChanged(QString);

	public:
		explicit MessagesModel(QObject *parent = 0);

		QModelIndexList indexes(const QList<int>& rows);
		int getUnreadCount() const;
		void setItemWidth(int width);
		QMap<MessageKey, QWidget*> tail(const QString& aimId, QWidget* parent, qint64 newId);
		QMap<MessageKey, QWidget*> more(const QString& aimId, QWidget* parent, qint64 newId);
		QWidget* getById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId);
		unsigned preloadCount() const;
        unsigned moreCount() const;
		void setLastKey(const MessageKey& key, const QString& aimId);
		void removeDialog(const QString& aimId);
		void updateNew(const QString& aimId, qint64 newId, bool hide = false);
		QString formatRecentsText(const Data::MessageBuddy &buddy) const;

	private:

		Data::MessageBuddy item(const InternalIndex& index);
		void insertDateMessage(const MessageKey& key, const QString& aimId, const QDate& date);
		void requestMessages(const QString& aimId);
		int generatedDomUid();
		Ui::HistoryControlPageItem* fill(const Data::MessageBuddy& msg, QWidget* parent) const;
		QWidget* fillNew(const InternalIndex& index, QWidget* parent, qint64 newId);
		QWidget* fillItemById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId);
		void setFirstMessage(const QString& aimId, qint64 id);
		std::set<MessageInternal>::iterator previousMsg(const QString& aimId, qint64 id);
		void processPendingMessage(Data::MessageBuddies& msgs, const QString& aimId, const Ui::MessagesBuddiesOpt state);
		bool tryInsertPendingMessageToLast(std::shared_ptr<Data::MessageBuddies> msgs, const QString& aimId);
		void sendDeliveryNotifications(const Data::MessageBuddies &msgs);
        void emitUpdated(const QList<Logic::MessageKey>& list, const QString& aimId, unsigned mode);
        void emitDeleted(const QList<Logic::MessageKey>& list, const QString& aimId);

		// widgets factory
		void createFileSharingWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;
        void createImagePreviewWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;
		void createStickerWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;

	private:
		QHash<QString, std::set<MessageInternal>> Messages_;
		QHash<QString, std::set<InternalIndex>> Indexes_;
		QHash<QString, std::set<InternalIndex>> PendingMessages_;
		QHash<QString, qint64> LastRequested_;
		QStringList Requested_;
        QStringList Subscribed_;
		QList<qint64> Sequences_;

		QHash<QString, MessageKey> LastKey_;
		QHash<QString, QList<QDate>> Dates_;
		QHash<QString, QList<InternalIndex>> TmpDates_;

		int ItemWidth_;
		int DomUid_;
	};

	MessagesModel* GetMessagesModel();
    void ResetMessagesModel();
}