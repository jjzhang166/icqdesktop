#pragma once

namespace core
{
	class coll_helper;

	enum class message_type;
}

namespace HistoryControl
{
	typedef std::shared_ptr<class FileSharingInfo> FileSharingInfoSptr;

	typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;

	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

    typedef std::shared_ptr<class VoipEventInfo> VoipEventInfoSptr;
}

namespace Logic
{
	class MessageKey;
}

namespace Data
{
	class MessageBuddy
	{
	public:
		MessageBuddy();

        bool IsEmpty() const;

		bool CheckInvariant() const;

        bool ContainsPreviewableLink() const;

        bool ContainsPttAudio() const;
        
        bool ContainsImage() const;

		bool IsBase() const;

		bool IsChatEvent() const;

		bool IsDeliveredToClient() const;

		bool IsDeliveredToServer() const;

		bool IsFileSharing() const;

		bool IsOutgoing() const;

        bool IsOutgoingVoip() const;

		bool IsSticker() const;

		bool IsPending() const;

        bool IsServiceMessage() const;

		bool IsStandalone() const;

        bool IsVoipEvent() const;

		const HistoryControl::ChatEventInfoSptr& GetChatEvent() const;

		const QDate& GetDate() const;

		const HistoryControl::FileSharingInfoSptr& GetFileSharing() const;

        QStringRef GetFirstUriFromText() const;

        int GetPttDuration() const;

        bool GetIndentBefore() const;

		const HistoryControl::StickerInfoSptr& GetSticker() const;

		const QString& GetText() const;

		const qint32 GetTime() const;

		qint64 GetLastId() const;

		const QStringList& GetNotificationKeys() const;

        const HistoryControl::VoipEventInfoSptr& GetVoipEvent() const;

        bool HasAvatar() const;

		bool HasId() const;

		bool HasText() const;

		void MarkAsDeliveredToClient();

		void FillFrom(const MessageBuddy &buddy, const bool merge);

		void SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent);

		void SetDate(const QDate &date);

		void SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing);

        void SetHasAvatar(const bool hasAvatar);

        void SetIndentBefore(const bool indentBefore);

		void SetLastId(const qint64 lastId);

		void SetNotificationKeys(const QStringList &keys);

		void SetOutgoing(const bool isOutgoing);

		void SetSticker(const HistoryControl::StickerInfoSptr &sticker);

		void SetText(const QString &text);

		void SetTime(const qint32 time);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voipEvent);

		Logic::MessageKey ToKey() const;

		QString AimId_;
		QString InternalId_;
		qint64 Id_;
		qint64 Prev_;
		core::message_type Type_;
        int PendingId_;
        qint32 Time_;

		bool Chat_;
		QString ChatSender_;
		QString ChatFriendly_;

		//filled by model
		bool Unread_;
		bool Filled_;

	private:
		qint64 LastId_;

		QString Text_;

		QStringList NotificationKeys_;

		QDate Date_;

		bool DeliveredToClient_;

        bool HasAvatar_;

        bool IndentBefore_;

		bool Outgoing_;

		HistoryControl::FileSharingInfoSptr FileSharing_;

		HistoryControl::StickerInfoSptr Sticker_;

		HistoryControl::ChatEventInfoSptr ChatEvent_;

        HistoryControl::VoipEventInfoSptr VoipEvent_;

	};

	class DlgState
	{
	public:
		DlgState()
			: UnreadCount_(0)
			, LastMsgId_(-1)
			, YoursLastRead_(-1)
			, TheirsLastRead_(-1)
			, TheirsLastDelivered_(-1)
			, Time_(-1)
			, Outgoing_(false)
			, Chat_(false)
			, Visible_(true)
            , senderNick_(QString())
            , FavoriteTime_(-1)
		{
		}

		bool operator==(const DlgState& other) const
		{
			return AimId_ == other.AimId_;
		}

		const QString& GetText() const;

        bool HasText() const;

		void SetText(const QString &text);

		QString AimId_;
		qint64 UnreadCount_;
		qint64 LastMsgId_;
		qint64 YoursLastRead_;
		qint64 TheirsLastRead_;
		qint64 TheirsLastDelivered_;
        qint64 FavoriteTime_;
		qint32 Time_;
		bool Outgoing_;
		bool Chat_;
		bool Visible_;
		QString LastMessageFriendly_;
        QString senderNick_;

	private:
		QString Text_;

	};

	typedef std::shared_ptr<MessageBuddy> MessageBuddySptr;
	typedef QList<MessageBuddySptr> MessageBuddies;
	typedef std::shared_ptr<MessageBuddies> MessageBuddiesSptr;

	void UnserializeMessageBuddies(core::coll_helper* helper, const QString &myAimid, Out QString &aimId, Out bool &havePending, Out MessageBuddies& messages);

	void SerializeDlgState(core::coll_helper* helper, const DlgState& state);
	void UnserializeDlgState(core::coll_helper* helper, const QString &myAimId, Out DlgState& state);
}

Q_DECLARE_METATYPE(Data::MessageBuddy);
Q_DECLARE_METATYPE(Data::DlgState);
