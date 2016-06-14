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

        MessageKey(
            qint64 id,
            qint64 prev,
            const QString& internalId,
            int pendingId,
            qint32 time,
            core::message_type type,
            bool outgoing,
            bool isPreview,
            bool isDeleted,
            control_type _control_type);

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

        bool isDeleted() const;

        bool isOutgoing() const;

        bool isFileSharing() const;

        bool isSticker() const;

        bool isDate() const;

        void setDeleted(const bool isDeleted);

        void setId(const int64_t id);

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
        bool IsDeleted_;

        bool Outgoing_;

        bool compare(const MessageKey& rhs) const;

    };

    typedef std::set<MessageKey> MessageKeySet;

    typedef std::vector<MessageKey> MessageKeyVector;

    typedef std::set<class InternalIndex> InternalIndexSet;

    typedef InternalIndexSet::iterator InternalIndexSetIter;

    class InternalIndex
    {
    public:
        InternalIndex()
            : MaxTime_(-1)
            , MinTime_(-1)
            , LastIndex_(-1)
            , Deleted_(false)
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

        bool ContainsId(const int64_t id) const;

        bool ContainsKey(const MessageKey &needle) const;

        bool containsNotLast(const int64_t id) const
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

        bool IsDeleted() const;

        bool IsFileSharing() const;

        bool IsOutgoing() const;

        bool IsPending() const;

        bool IsStandalone() const;

        bool IsSticker() const;

        bool IsVoipEvent() const;

        bool IsPreview() const;

        const HistoryControl::ChatEventInfoSptr& GetChatEvent() const;

        const QString& GetChatSender() const;

        const HistoryControl::FileSharingInfoSptr& GetFileSharing() const;

        const MessageKey& GetFirstMessageKey() const;

        MessageKey GetKeyById(const int64_t id) const;

        const MessageKey& GetLastMessageKey() const;

        const HistoryControl::StickerInfoSptr& GetSticker() const;

        const HistoryControl::VoipEventInfoSptr& GetVoipEvent() const;

        void SetChatEvent(const HistoryControl::ChatEventInfoSptr& info);

        void SetChatSender(const QString& chatSender);

        void SetDeleted(const bool deleted);

        void SetFileSharing(const HistoryControl::FileSharingInfoSptr& info);

        void SetSticker(const HistoryControl::StickerInfoSptr& info);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr& info);

        void InsertMessageKey(const MessageKey& key) const;

        void InsertMessageKeys(const MessageKeySet& keys) const;

        const MessageKeySet& GetMessageKeys() const;

        void RemoveMessageKey(MessageKey key) const;

        void ApplyModification(const Data::MessageBuddy &modification);

        QString AimId_;
        MessageKey Key_;
        mutable qint32 MaxTime_;
        mutable qint32 MinTime_;
        QDate Date_;
        mutable QString ChatFriendly_;
        qint64 LastIndex_;

    private:
        HistoryControl::FileSharingInfoSptr FileSharing_;

        HistoryControl::StickerInfoSptr Sticker_;

        HistoryControl::ChatEventInfoSptr ChatEvent_;

        HistoryControl::VoipEventInfoSptr VoipEvent_;

        mutable MessageKeySet MsgKeys_;

        mutable QString ChatSender_;

        bool Deleted_;

        void EraseEventData();

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

        explicit MessageInternal(const MessageKey& id)
            : Key_(id)
        {
        }

        MessageInternal(const MessageKey& id, Data::MessageBuddy* buddy)
            : Key_(id)
            , Buddy_(buddy)
        {
        }

        explicit MessageInternal(std::shared_ptr<Data::MessageBuddy> buddy)
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
            MODIFIED
        };

Q_SIGNALS:
        void ready(QString);
        void updated(QList<Logic::MessageKey>, QString, unsigned);
        void deleted(QList<Logic::MessageKey>, QString);
        void readByClient(QString _aimid, qint64 _id);
        void deliveredToServer(QString);
        void messageIdFetched(QString, Logic::MessageKey);
        void pttPlayed(qint64);
        void canFetchMore(QString);
        void indentChanged(Logic::MessageKey, bool);
        void chatEvent(QString aimId);

        private Q_SLOTS:
            void messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64);

            void messagesDeletedUpTo(QString, int64_t);
            void messagesModified(QString, std::shared_ptr<Data::MessageBuddies>);
            void dlgState(Data::DlgState);
            void fileSharingUploadingResult(QString, bool, QString, bool);

            public Q_SLOTS:
                void contactChanged(QString);
                void messagesDeleted(QString, QList<int64_t>);

    public:
        explicit MessagesModel(QObject *parent = 0);

        QModelIndexList indexes(const QList<int>& rows);
        int getUnreadCount() const;
        void setItemWidth(int width);
        QMap<MessageKey, QWidget*> tail(const QString& aimId, QWidget* parent, qint64 newId);
        QMap<MessageKey, QWidget*> more(const QString& aimId, QWidget* parent, qint64 newId);
        QWidget* getById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId);
        int32_t preloadCount() const;
        int32_t moreCount() const;
        void setLastKey(const MessageKey& key, const QString& aimId);
        void removeDialog(const QString& aimId);
        void updateNew(const QString& aimId, qint64 newId, bool hide = false);
        QString formatRecentsText(const Data::MessageBuddy &buddy) const;
        int64_t getLastMessageId(const QString &aimId) const;
        std::vector<int64_t> getBubbleMessageIds(const QString &aimId, const int64_t messageId) const;
        qint64 normalizeNewMessagesId(const QString& aimid, qint64 id);
        Logic::MessageKey findFirstKeyAfter(const QString &aimId, const Logic::MessageKey &key) const;
        bool isHasPending(const QString &aimId) const;

    private:
        Data::MessageBuddy item(const InternalIndex& index);
        void requestMessages(const QString& aimId);
        int generatedDomUid();
        Ui::HistoryControlPageItem* fill(const Data::MessageBuddy& msg, QWidget* parent) const;
        QWidget* fillNew(const InternalIndex& index, QWidget* parent, qint64 newId);
        QWidget* fillItemById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId);
        void setFirstMessage(const QString& aimId, qint64 id);
        std::set<MessageInternal>::iterator previousMsg(const QString& aimId, const qint64 id);
        void processPendingMessage(InOut Data::MessageBuddies& msgs, const QString& aimId, const Ui::MessagesBuddiesOpt state);
        bool tryInsertPendingMessageToLast(std::shared_ptr<Data::MessageBuddies> msgs, const QString& aimId);
        void sendDeliveryNotifications(const Data::MessageBuddies &msgs);
        void emitUpdated(const QList<Logic::MessageKey>& list, const QString& aimId, unsigned mode);
        void emitDeleted(const QList<Logic::MessageKey>& list, const QString& aimId);

        // widgets factory
        void createFileSharingWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;
        void createImagePreviewWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;
        void createStickerWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const;

        MessageKey applyMessageModification(const QString& aimId, Data::MessageBuddy& modification);
        bool hasItemsInBetween(const QString &aimId, const InternalIndex &l, const InternalIndex &r) const;
        void updateDateItems(const QString& aimId);
        void updateMessagesMargins(const QString& aimId);

        void addDateItem(const QString &aimId, const MessageKey &key, const QDate &date);
        bool hasDate(const QString &aimId, const QDate &date) const;
        void removeDateItem(const QString &aimId, const QDate &date);
        void removeDateItems(const QString &aimId);
        void removeDateItemIfOutdated(const QString &aimId, const Data::MessageBuddy &msg);

        InternalIndexSetIter findIndexRecord(InternalIndexSet &indexRecords, const Logic::MessageKey &key) const;

    private:
        QHash<QString, std::set<MessageInternal>> Messages_;
        QHash<QString, InternalIndexSet> Indexes_;
        QHash<QString, InternalIndexSet> PendingMessages_;
        QHash<QString, qint64> LastRequested_;
        QStringList Requested_;
        QStringList FailedUploads_;
        QStringList Subscribed_;
        QList<qint64> Sequences_;

        QHash<QString, MessageKey> LastKey_;
        QHash<QString, std::map<QDate, InternalIndex>> DateItems_;

        int32_t ItemWidth_;
        int32_t DomUid_;
    };

    MessagesModel* GetMessagesModel();
    void ResetMessagesModel();
}