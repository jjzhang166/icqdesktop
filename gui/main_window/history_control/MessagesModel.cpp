#include "stdafx.h"

#include "MessagesModel.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../../utils/Text2DocConverter.h"

#include "MessageItem.h"
#include "MessageStyle.h"
#include "ServiceMessageItem.h"
#include "ChatEventItem.h"
#include "DeletedMessageItem.h"

#include "../../core_dispatcher.h"
#include "../../../corelib/core_face.h"
#include "../../../corelib/enumerations.h"

#include "../history_control/ChatEventInfo.h"
#include "ContentWidgets/FileSharingWidget.h"
#include "../history_control/FileSharingInfo.h"
#include "ContentWidgets/ImagePreviewWidget.h"
#include "ContentWidgets/StickerWidget.h"
#include "../history_control/VoipEventInfo.h"
#include "../history_control/VoipEventItem.h"
#include "ContentWidgets/PttAudioWidget.h"

#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"

#include "../../gui_settings.h"

namespace
{
    static const auto PRELOAD_COUNT = 30;
    static const auto MORE_COUNT = 30;

    Logic::MessageInternal internal(const Logic::MessageKey& key)
    {
        return Logic::MessageInternal(key);
    }

    QString NormalizeAimId(const QString& aimId)
    {
        int pos = aimId.indexOf("@uin.icq");
        return pos == -1 ? aimId : aimId.left(pos);
    }

    QString GetChatFriendly(const QString& aimdId, const QString& chatFriendly)
    {
        QString normalized = NormalizeAimId(aimdId);
        QString clFriendly = Logic::GetContactListModel()->getDisplayName(normalized);
        if (clFriendly == normalized)
        {
            return chatFriendly.isEmpty() ? aimdId : chatFriendly;
        }

        return clFriendly;
    }
}

namespace Logic
{
    const MessageKey MessageKey::MAX(INT64_MAX, INT64_MAX - 1, QString(), -1, 0, core::message_type::base, false, false, false, control_type::ct_message);
    const MessageKey MessageKey::MIN(2, 1, QString(), -1, 0, core::message_type::base, false, false, false, control_type::ct_message);

    MessageKey::MessageKey()
        : Id_(-1)
        , Prev_(-1)
        , Type_(core::message_type::base)
        , Outgoing_(false)
        , PendingId_(-1)
        , Time_(-1)
        , IsPreview_(false)
        , IsDeleted_(false)
        , control_type_(control_type::ct_message)
    {
    }

    MessageKey::MessageKey(
        qint64 id,
        qint64 prev,
        const QString& internalId,
        int pendingId,
        qint32 time,
        core::message_type type,
        bool outgoing,
        bool isPreview,
        bool isDeleted,
        control_type _control_type)
        : Id_(id)
        , InternalId_(internalId)
        , Type_(type)
        , Prev_(prev)
        , Outgoing_(outgoing)
        , PendingId_(pendingId)
        , Time_(time)
        , IsPreview_(isPreview)
        , IsDeleted_(isDeleted)
        , control_type_(_control_type)
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);
    }

    bool MessageKey::operator<(const MessageKey& other) const
    {
        return compare(other);
    }

    bool MessageKey::hasId() const
    {
        return (Id_ != -1);
    }

    bool MessageKey::checkInvariant() const
    {
        if (Outgoing_)
        {
            if (!hasId() && InternalId_.isEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!InternalId_.isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    bool MessageKey::isChatEvent() const
    {
        assert(Type_ >= core::message_type::min);
        assert(Type_ <= core::message_type::max);

        return (Type_ == core::message_type::chat_event);
    }

    bool MessageKey::isDeleted() const
    {
        return IsDeleted_;
    }

    bool MessageKey::isOutgoing() const
    {
        return Outgoing_;
    }

    void MessageKey::setDeleted(const bool isDeleted)
    {
        IsDeleted_ = isDeleted;
    }

    void MessageKey::setId(const int64_t id)
    {
        assert(id >= -1);

        Id_ = id;
    }

    void MessageKey::setOutgoing(const bool isOutgoing)
    {
        Outgoing_ = isOutgoing;

        if (!hasId() && Outgoing_)
        {
            assert(!InternalId_.isEmpty());
        }
    }

    QString MessageKey::toLogStringShort() const
    {
        QString logStr;
        logStr.reserve(512);

        QTextStream fmt(&logStr);

        fmt << "id=" << Id_ << ";"
            "prev=" << Prev_;

        return logStr;
    }

    bool MessageKey::isFileSharing() const
    {
        assert(Type_ >= core::message_type::min);
        assert(Type_ <= core::message_type::max);

        return (Type_ == core::message_type::file_sharing);
    }

    bool MessageKey::isDate() const
    {
        return (control_type_ == control_type::ct_date);
    }

    bool MessageKey::isSticker() const
    {
        assert(Type_ >= core::message_type::min);
        assert(Type_ <= core::message_type::max);

        return (Type_ == core::message_type::sticker);
    }

    bool MessageKey::compare(const MessageKey& rhs) const
    {
        if (!InternalId_.isEmpty() && InternalId_ == rhs.InternalId_)
        {
            return control_type_ < rhs.control_type_;
        }

        if (PendingId_ != -1 && rhs.PendingId_ == -1)
        {
            if (control_type_ != rhs.control_type_)
                return control_type_ < rhs.control_type_;

            if (control_type_ == rhs.control_type_)
                return false;

            return Time_ == rhs.Time_ ? false : Time_ < rhs.Time_;
        }

        if (PendingId_ == -1 && rhs.PendingId_ != -1)
        {
            if (control_type_ != rhs.control_type_)
                return control_type_ < rhs.control_type_;

            if (control_type_ == rhs.control_type_)
                return true;

            return Time_ == rhs.Time_ ? true : Time_ < rhs.Time_;
        }

        if ((Id_ != -1) && (rhs.Id_ != -1))
        {
            if (Id_ != rhs.Id_)
            {
                return (Id_ < rhs.Id_);
            }
        }

        if (PendingId_ != -1 && rhs.PendingId_ != -1)
        {
            if (PendingId_ != rhs.PendingId_)
            {
                return (PendingId_ < rhs.PendingId_);
            }
            else if (InternalId_ != rhs.InternalId_)
            {
                return Time_ < rhs.Time_;
            }
        }

        return (control_type_ < rhs.control_type_);
    }

    bool InternalIndex::ContainsId(const int64_t id) const
    {
        assert(id >= -1);

        if (Key_.Id_ == id)
        {
            return true;
        }

        return std::any_of(
            MsgKeys_.cbegin(),
            MsgKeys_.cend(),
            [id](const Logic::MessageKey &key)
            {
                return (key.Id_ == id);
            });
    }

    bool InternalIndex::ContainsKey(const MessageKey &needle) const
    {
        assert(!needle.isEmpty());

        return std::any_of(
            MsgKeys_.cbegin(),
            MsgKeys_.cend(),
            [&needle](const Logic::MessageKey &key)
            {
                return ((key.Id_ == needle.Id_) && (key.control_type_ == needle.control_type_));
            });
    }

    bool InternalIndex::IsBase() const
    {
        assert(Key_.Type_ > core::message_type::min);
        assert(Key_.Type_ < core::message_type::max);

        return (Key_.Type_ == core::message_type::base);
    }

    bool InternalIndex::IsChatEvent() const
    {
        assert(Key_.Type_ > core::message_type::min);
        assert(Key_.Type_ < core::message_type::max);

        return (Key_.Type_ == core::message_type::chat_event);
    }

    bool InternalIndex::IsDate() const
    {
        return (Key_.isDate());
    }

    bool InternalIndex::IsDeleted() const
    {
        return Deleted_;
    }

    bool InternalIndex::IsFileSharing() const
    {
        assert(Key_.Type_ > core::message_type::min);
        assert(Key_.Type_ < core::message_type::max);
        assert((Key_.Type_ != core::message_type::file_sharing) || FileSharing_);

        return (Key_.Type_ == core::message_type::file_sharing);
    }

    bool InternalIndex::IsOutgoing() const
    {
        return Key_.isOutgoing();
    }

    bool InternalIndex::IsPending() const
    {
        return (!Key_.hasId() && !Key_.InternalId_.isEmpty());
    }

    bool InternalIndex::IsStandalone() const
    {
        return true;
        //return (IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || IsPreview() || IsDeleted() || IsPending());
    }

    bool InternalIndex::IsSticker() const
    {
        assert(Key_.Type_ > core::message_type::min);
        assert(Key_.Type_ < core::message_type::max);
        assert((Key_.Type_ != core::message_type::sticker) || Sticker_);

        return (Key_.Type_ == core::message_type::sticker);
    }

    bool InternalIndex::IsVoipEvent() const
    {
        assert(Key_.Type_ > core::message_type::min);
        assert(Key_.Type_ < core::message_type::max);
        assert((Key_.Type_ != core::message_type::voip_event) || VoipEvent_);

        return (Key_.Type_ == core::message_type::voip_event);
    }

    bool InternalIndex::IsPreview() const
    {
        return Key_.IsPreview_;
    }

    const HistoryControl::ChatEventInfoSptr& InternalIndex::GetChatEvent() const
    {
        assert(!ChatEvent_ || IsChatEvent());

        return ChatEvent_;
    }

    const QString& InternalIndex::GetChatSender() const
    {
        return ChatSender_;
    }

    const HistoryControl::FileSharingInfoSptr& InternalIndex::GetFileSharing() const
    {
        assert(!FileSharing_ || IsFileSharing());

        return FileSharing_;
    }

    const MessageKey& InternalIndex::GetFirstMessageKey() const
    {
        assert(!MsgKeys_.empty());

        const auto &firstKey = *MsgKeys_.cbegin();
        return firstKey;
    }

    MessageKey InternalIndex::GetKeyById(const int64_t id) const
    {
        for (const auto &key : MsgKeys_)
        {
            if (key.Id_ == id)
            {
                return key;
            }
        }

        return MessageKey();
    }

    const MessageKey& InternalIndex::GetLastMessageKey() const
    {
        assert(!MsgKeys_.empty());

        const auto &lastKey = *MsgKeys_.crbegin();
        return lastKey;
    }

    const HistoryControl::StickerInfoSptr& InternalIndex::GetSticker() const
    {
        assert(!Sticker_ || IsSticker());

        return Sticker_;
    }

    const HistoryControl::VoipEventInfoSptr& InternalIndex::GetVoipEvent() const
    {
        assert(!VoipEvent_ || IsVoipEvent());

        return VoipEvent_;
    }

    void InternalIndex::SetChatEvent(const HistoryControl::ChatEventInfoSptr& info)
    {
        assert(!ChatEvent_);
        assert(!info || (Key_.Type_ == core::message_type::chat_event));

        ChatEvent_ = info;
    }

    void InternalIndex::SetChatSender(const QString& chatSender)
    {
        ChatSender_ = chatSender;
    }

    void InternalIndex::SetDeleted(const bool deleted)
    {
        Deleted_ = deleted;
        Key_.setDeleted(deleted);
    }

    void InternalIndex::SetFileSharing(const HistoryControl::FileSharingInfoSptr& info)
    {
        assert(!FileSharing_);
        assert(!info || (Key_.Type_ == core::message_type::file_sharing));

        FileSharing_ = info;
    }

    void InternalIndex::SetSticker(const HistoryControl::StickerInfoSptr& info)
    {
        assert(!Sticker_);
        assert(!info || (Key_.Type_ == core::message_type::sticker));

        Sticker_ = info;
    }

    void InternalIndex::SetVoipEvent(const HistoryControl::VoipEventInfoSptr& info)
    {
        assert(!VoipEvent_);
        assert(!info || (Key_.Type_ == core::message_type::voip_event));

        VoipEvent_ = info;
    }

    void InternalIndex::InsertMessageKey(const MessageKey& key) const
    {
        assert(
            !std::any_of(
                MsgKeys_.begin(),
                MsgKeys_.end(),
                [](const MessageKey& key)
                {
                    return (key.isFileSharing() || key.isSticker() || key.isChatEvent());
                }));

        assert(MsgKeys_.empty() || (!key.isFileSharing() && !key.isSticker() && !key.isChatEvent()));

        MsgKeys_.insert(key);
    }

    void InternalIndex::InsertMessageKeys(const MessageKeySet& keys) const
    {
        MsgKeys_.insert(keys.begin(), keys.end());
    }

    const MessageKeySet& InternalIndex::GetMessageKeys() const
    {
        return MsgKeys_;
    }

    void InternalIndex::RemoveMessageKey(MessageKey key) const
    {
        MsgKeys_.erase(key);
    }

    void InternalIndex::ApplyModification(const Data::MessageBuddy &modification)
    {
        assert(ContainsId(modification.Id_));

        EraseEventData();

        if (modification.IsBase())
        {
            Key_.Type_ = core::message_type::base;

            return;
        }

        if (modification.IsChatEvent())
        {
            Key_.Type_ = core::message_type::base;

            return;
        }
    }

    void InternalIndex::EraseEventData()
    {
        FileSharing_.reset();
        Sticker_.reset();
        ChatEvent_.reset();
        VoipEvent_.reset();
    }

    std::unique_ptr<MessagesModel> g_messages_model;

    MessagesModel::MessagesModel(QObject *parent)
        : QObject(parent)
        , ItemWidth_(0)
        , DomUid_(1)
    {
        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messageBuddies,
            this,
            &MessagesModel::messageBuddies,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeleted,
            this,
            &MessagesModel::messagesDeleted,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeletedUpTo,
            this,
            &MessagesModel::messagesDeletedUpTo,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesModified,
            this,
            &MessagesModel::messagesModified,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::dlgState,
            this,
            &MessagesModel::dlgState,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingResult,
            this,
            &MessagesModel::fileSharingUploadingResult,
            Qt::DirectConnection);
    }

    int MessagesModel::generatedDomUid()
    {
        return DomUid_++;
    }

    void MessagesModel::dlgState(Data::DlgState state)
    {
        emit readByClient(state.AimId_, state.TheirsLastRead_);
    }

    void MessagesModel::fileSharingUploadingResult(QString id, bool, QString, bool too_large_files)
    {
        if (too_large_files)
            FailedUploads_ << id;
    }

    void MessagesModel::messageBuddies(std::shared_ptr<Data::MessageBuddies> msgs, QString aimId, Ui::MessagesBuddiesOpt option, bool havePending, qint64 seq)
    {
        assert(option > Ui::MessagesBuddiesOpt::Min);
        assert(option < Ui::MessagesBuddiesOpt::Max);
        assert(msgs);

        const auto isDlgState = (option == Ui::MessagesBuddiesOpt::DlgState || option == Ui::MessagesBuddiesOpt::Init || option == Ui::MessagesBuddiesOpt::MessageStatus);
        const auto isPending = (option == Ui::MessagesBuddiesOpt::Pending);

        if (build::is_debug())
        {
            for (const auto &buddy : *msgs)
            {
                if (!buddy->IsFileSharing())
                {
                    continue;
                }

                __TRACE(
                    "fs",
                    "incoming file sharing message in model\n" <<
                    "    seq=<" << seq << ">\n" <<
                    buddy->GetFileSharing()->ToLogString());
            }
        }

        if (isDlgState && !msgs->empty())
        {
            sendDeliveryNotifications(*msgs);
        }

        if (isPending && tryInsertPendingMessageToLast(msgs, aimId))
        {
            updateMessagesMargins(aimId);

            return;
        }

        if (msgs->isEmpty())
        {
            if (Sequences_.contains(seq))
            {
                LastRequested_[aimId] = -1;
            }

            updateMessagesMargins(aimId);

            return;
        }

        qint64 modelFirst = (Messages_[aimId].empty() || Indexes_[aimId].empty()) ? -2 : Messages_[aimId].begin()->Key_.Prev_;

        if (havePending || isDlgState || isPending)
        {
            processPendingMessage(*msgs, aimId, option);
        }

        if (msgs->isEmpty())
        {
            if (Sequences_.contains(seq))
            {
                LastRequested_[aimId] = -1;
            }

            updateMessagesMargins(aimId);

            return;
        }

        if ((!Sequences_.contains(seq) && msgs->last()->Id_ < modelFirst) || !Requested_.contains(aimId))
        {
            updateMessagesMargins(aimId);

            return;
        }

        bool hole = false;

        if (msgs->size() > PRELOAD_COUNT || option == Ui::MessagesBuddiesOpt::FromServer)
        {
            Data::MessageBuddies tmpMsgs;
            for (int i = 0; i < PRELOAD_COUNT; ++i)
            {
                if (msgs->empty())
                    break;

                tmpMsgs.push_front(msgs->last());
                msgs->pop_back();
            }
            *msgs = std::move(tmpMsgs);
            auto idFirst = msgs->first()->Id_;

            if (!isDlgState)
            {
                QList<MessageKey> deletedValues;

                auto &indexRecords = Indexes_[aimId];

                auto iter = indexRecords.begin();
                while(iter != indexRecords.end())
                {
                    if (iter->IsPending())
                    {
                        continue;
                    }

                    if (iter->Key_.Id_ < idFirst)
                    {
                        deletedValues << iter->Key_;

                        for (auto id : iter->GetMessageKeys())
                        {
                            Messages_[aimId].erase(internal(iter->Key_));
                        }

                        if (iter->IsDate())
                        {
                            removeDateItem(aimId, iter->Date_);
                        }

                        if (iter->Key_.Id_ <= LastRequested_[aimId])
                        {
                            LastRequested_[aimId] = -1;
                        }

                        iter = Indexes_[aimId].erase(iter);

                        continue;
                    }

                    if (iter->IsDate() && (iter->Date_ == msgs->first()->GetDate()))
                    {
                        deletedValues << iter->Key_;

                        removeDateItem(aimId, iter->Date_);

                        iter = Indexes_[aimId].erase(iter);

                        continue;
                    }

                    ++iter;
                }

                std::set<MessageInternal>::iterator iterMsg = Messages_[aimId].begin();
                while (iterMsg != Messages_[aimId].end())
                {
                    if (iterMsg->Key_.Id_ != -1 && iterMsg->Key_.Id_ < idFirst)
                        iterMsg = Messages_[aimId].erase(iterMsg);
                    else
                        ++iterMsg;
                }

                MessageKey key;
                if (!Indexes_[aimId].empty())
                {
                    key = Indexes_[aimId].begin()->Key_;
                }
                else
                {
                    key = msgs->first()->ToKey();
                }
                if (LastKey_[aimId] < key)
                    LastKey_[aimId] = key;

                Sequences_.clear();
                LastRequested_[aimId] = -1;

                hole = !deletedValues.isEmpty();
                if (hole)
                {
                    emitDeleted(deletedValues, aimId);
                }
            }
        }

        QList<MessageKey> updatedValues;
        std::set<InternalIndex> indexes;

        for (auto msg : *msgs)
        {
            auto &messages = Messages_[aimId];

            const auto key = msg->ToKey();
            const auto messageAlreadyInserted = (std::find(messages.begin(), messages.end(), internal(key)) != messages.end());
            if (messageAlreadyInserted)
            {
                continue;
            }

            auto prevMsg = previousMsg(aimId, msg->Prev_);
            const auto havePrevMsg = (prevMsg != messages.end());

            if (havePrevMsg)
            {
                const auto &prevMsgBuddy = prevMsg->Buddy_;
                msg->SetHasAvatar(msg->IsOutgoing() ? false : msg->GetIndentWith(*prevMsgBuddy));
            }
            else
            {
                msg->SetHasAvatar(!msg->IsOutgoing());
            }

            auto merged = false;
            if (!msg->IsStandalone())
            {
                for (auto &index : indexes)
                {
                    if (msg->GetTime() >= (index.MinTime_ - 120)
                        && msg->GetTime() <= (index.MaxTime_ + 120)
                        && index.Key_.isOutgoing() == msg->IsOutgoing()
                        && index.GetChatSender() == msg->GetChatSender()
                        && !index.IsDate() && !index.IsStandalone()
                        && (index.ContainsId(msg->Id_) || index.ContainsId(msg->Prev_)))
                    {
                        index.InsertMessageKey(key);
                        index.MaxTime_ = std::max(msg->GetTime(), index.MaxTime_);
                        index.MinTime_ = std::min(msg->GetTime(), index.MinTime_);
                        merged = true;
                        break;
                    }
                }
            }

            if (!merged)
            {
                removeDateItemIfOutdated(aimId, *msg);

                const auto dateItemAlreadyExists = hasDate(aimId, msg->GetDate());
                const auto scheduleForDatesProcessing = (!dateItemAlreadyExists && !msg->IsDeleted());
                if (scheduleForDatesProcessing)
                {
                    auto dateKey = msg->ToKey();
                    dateKey.Type_ = core::message_type::undefined;
                    dateKey.control_type_ = control_type::ct_date;

                    InternalIndex newIndex;
                    newIndex.AimId_ = aimId;
                    newIndex.MaxTime_ = 0;
                    newIndex.MinTime_ = 0;
                    newIndex.Date_ = msg->GetDate();
                    newIndex.InsertMessageKey(dateKey);
                    newIndex.Key_ = dateKey;
                    newIndex.SetDeleted(msg->IsDeleted());

                    const auto isFirstMessage = !havePrevMsg;
                    const auto isDateDiffer = (havePrevMsg && (msg->GetDate() != prevMsg->Buddy_->GetDate()));
                    const auto insertDateTablet = (isFirstMessage || isDateDiffer);
                    if (insertDateTablet)
                    {
                        addDateItem(aimId, newIndex.Key_, newIndex.Date_);
                        indexes.insert(newIndex);
                        msg->SetHasAvatar(!msg->IsOutgoing());
                    }
                }

                InternalIndex newIndex;
                newIndex.AimId_ = aimId;
                newIndex.MaxTime_ = msg->GetTime();
                newIndex.MinTime_ = msg->GetTime();
                newIndex.InsertMessageKey(key);
                newIndex.Key_ = key;
                newIndex.SetChatSender(msg->GetChatSender());
                newIndex.ChatFriendly_ = msg->ChatFriendly_;
                newIndex.SetFileSharing(msg->GetFileSharing());
                newIndex.SetSticker(msg->GetSticker());
                newIndex.SetChatEvent(msg->GetChatEvent());
                newIndex.SetVoipEvent(msg->GetVoipEvent());
                newIndex.SetDeleted(msg->IsDeleted());
                indexes.insert(newIndex);
            }

            messages.emplace(MessageInternal(msg));
        }

        auto &indexesByAimId = Indexes_[aimId];

        QList<MessageKey> updatedIndexes;
        for (auto iter : indexes)
        {
            bool inserted = false;
            if (!iter.IsDate() && !iter.IsStandalone())
            {
                for (auto ind = indexesByAimId.begin(); ind != indexesByAimId.end(); ++ind)
                {
                    if (!PendingMessages_[aimId].empty() && ind->Key_.Id_ <= PendingMessages_[aimId].rbegin()->LastIndex_)
                        continue;

                    if (ind->Key_.isOutgoing() == iter.Key_.isOutgoing()
                        && ind->GetChatSender() == iter.GetChatSender()
                        && !ind->IsDate() && !ind->IsStandalone())
                    {
                        if (((iter.ContainsId(ind->Key_.Prev_) || iter.ContainsId(ind->Key_.Id_)) && ind->MinTime_ - iter.MaxTime_ <= 120)
                            || ((ind->ContainsId(iter.Key_.Prev_) || ind->ContainsId(iter.Key_.Id_)) && iter.MinTime_ - ind->MaxTime_ <= 120))
                        {
                            ind->MaxTime_ = std::max(iter.MaxTime_, ind->MaxTime_);
                            ind->MinTime_ = std::min(iter.MinTime_, ind->MinTime_);
                            ind->InsertMessageKeys(iter.GetMessageKeys());
                            inserted = true;
                            if (!updatedIndexes.contains(ind->Key_))
                                updatedIndexes << ind->Key_;
                            break;
                        }
                    }
                }
            }

            if (!inserted)
            {
                indexesByAimId.insert(iter);
                if (!updatedValues.contains(iter.Key_))
                    updatedValues << iter.Key_;
            }
        }

        updateMessagesMargins(aimId);

        if (modelFirst == -2 && Sequences_.contains(seq))
            emit ready(aimId);

        if (option != Ui::MessagesBuddiesOpt::Requested)
        {
            for (auto iter : updatedIndexes)
            {
                updatedValues.append(iter);
            }
            emitUpdated(updatedValues, aimId, hole ? HOLE : BASE);
        }
        else
        {
            if (!updatedIndexes.empty())
                emitUpdated(updatedIndexes, aimId, REQUESTED);

            if (Subscribed_.contains(aimId))
            {
                Subscribed_.removeAll(aimId);
                emit canFetchMore(aimId);
            }
        }

        const auto isFromServer = (option == Ui::MessagesBuddiesOpt::FromServer);
        const auto isRequested = (option == Ui::MessagesBuddiesOpt::Requested);
        const auto isInit = (option == Ui::MessagesBuddiesOpt::Init);
        if (isFromServer || isRequested || isInit)
        {
            if ((int32_t)indexesByAimId.size() >= preloadCount())
            {
                setFirstMessage(
                    aimId,
                    indexesByAimId.begin()->Key_.Id_
                    );
            }
        }
    }

    void MessagesModel::messagesDeleted(QString aimId, QList<int64_t> deletedIds)
    {
        assert(!aimId.isEmpty());
        assert(!deletedIds.isEmpty());

        auto &index = Indexes_[aimId];
        auto &messages = Messages_[aimId];

        QList<Logic::MessageKey> messageKeys;

        for (const auto id : deletedIds)
        {
            assert(id > 0);

            InternalIndex searchIndexRec;
            searchIndexRec.Key_.Id_ = id;

            // find and mark the message as deleted

            auto messageIter = messages.find(internal(searchIndexRec.Key_));
            if (messageIter != messages.end())
            {
                auto patchedMessageRec = *messageIter;

                patchedMessageRec.Key_.setDeleted(true);
                patchedMessageRec.Buddy_->SetDeleted(true);

                auto insertionIter = messageIter;
                ++insertionIter;

                messages.erase(messageIter);

                messages.emplace_hint(insertionIter, std::move(patchedMessageRec));
            }

            // find the deleted index record

            const auto indexRecIter = index.find(searchIndexRec);
            if (indexRecIter == index.end())
            {
                continue;
            }

            // patch the index record found

            auto patchedIndexRec = *indexRecIter;

            const auto &key = patchedIndexRec.Key_;

            patchedIndexRec.SetDeleted(true);

            auto insertionIter = indexRecIter;
            ++insertionIter;

            index.erase(indexRecIter);

            index.emplace_hint(insertionIter, std::move(patchedIndexRec));

            messageKeys.push_back(key);

            __INFO(
                "delete_history",
                "sending delete request to the history page\n"
                "    message-id=<" << key.Id_ << ">"
                );
        }

        if (!messageKeys.empty())
        {
            emitDeleted(messageKeys, aimId);
        }

        updateMessagesMargins(aimId);

        updateDateItems(aimId);
    }

    void MessagesModel::messagesDeletedUpTo(QString aimId, int64_t id)
    {
        assert(!aimId.isEmpty());
        assert(id > -1);

        auto &index = Indexes_[aimId];

        QList<Logic::MessageKey> messageKeys;

        auto iter = index.cbegin();
        for (; iter != index.cend(); ++iter)
        {
            const auto currentId = iter->Key_.Id_;
            if (currentId > id)
            {
                break;
            }

            const Logic::MessageKey key(currentId, -1, QString(), -1, 0, core::message_type::base, false, false, true, Logic::control_type::ct_message);

            messageKeys.push_back(key);
        }

        emitDeleted(messageKeys, aimId);

        index.erase(index.begin(), iter);

        updateDateItems(aimId);
    }

    void MessagesModel::messagesModified(QString aimId, std::shared_ptr<Data::MessageBuddies> modifications)
    {
        assert(!aimId.isEmpty());
        assert(modifications);
        assert(!modifications->empty());

        QList<Logic::MessageKey> messageKeys;

        for (const auto &modification : *modifications)
        {
            if (!modification->HasText() &&
                !modification->IsChatEvent())
            {
                assert(!"unexpected modification");
                continue;
            }

            const auto key = applyMessageModification(aimId, *modification);
            if (!key.hasId())
            {
                continue;
            }

            messageKeys.push_back(key);

            __INFO(
                "delete_history",
                "sending update request to the history page\n"
                "    message-id=<" << key.Id_ << ">");
        }

        emitUpdated(messageKeys, aimId, MODIFIED);
    }

    void MessagesModel::processPendingMessage(InOut Data::MessageBuddies& msgs, const QString& aimId, const Ui::MessagesBuddiesOpt state)
    {
        assert(state > Ui::MessagesBuddiesOpt::Min);
        assert(state < Ui::MessagesBuddiesOpt::Max);

        if (msgs.empty())
        {
            return;
        }

        std::set<InternalIndex> indexes;
        for (Data::MessageBuddies::iterator iter = msgs.begin(); iter != msgs.end();)
        {
            auto msg = *iter;

            MessageKey key = msg->ToKey();
            if (msg->IsPending())
            {
                msg->SetHasAvatar(!msg->IsOutgoing());

                bool inserted = false;
                for (auto &index : indexes)
                {
                    if (msg->GetTime() >= (index.MinTime_ - 120)
                        && msg->GetTime() <= (index.MaxTime_ + 120)
                        && index.Key_.isOutgoing() == msg->IsOutgoing()
                        && index.GetChatSender() == msg->GetChatSender()
                        && !index.IsStandalone())
                    {
                        index.InsertMessageKey(key);
                        index.MaxTime_ = std::max(msg->GetTime(), index.MaxTime_);
                        index.MinTime_ = std::min(msg->GetTime(), index.MinTime_);
                        inserted = true;
                        break;
                    }
                }

                if (!inserted)
                {

                    InternalIndex newIndex;
                    newIndex.AimId_ = aimId;
                    newIndex.MaxTime_ = msg->GetTime();
                    newIndex.MinTime_ = msg->GetTime();
                    newIndex.InsertMessageKey(key);
                    newIndex.Key_ = key;
                    newIndex.SetChatSender(msg->GetChatSender());
                    newIndex.ChatFriendly_ = msg->ChatFriendly_;
                    newIndex.SetFileSharing(msg->GetFileSharing());
                    newIndex.SetSticker(msg->GetSticker());
                    newIndex.SetChatEvent(msg->GetChatEvent());
                    newIndex.SetVoipEvent(msg->GetVoipEvent());
                    newIndex.SetDeleted(msg->IsDeleted());
                    if (!Indexes_[aimId].empty())
                        newIndex.LastIndex_ = Indexes_[aimId].rbegin()->Key_.Id_;
                    indexes.insert(newIndex);
                }
                MessageInternal message(msg);
                Messages_[aimId].emplace(message);
                iter = msgs.erase(iter);
            }
            else if (key.isOutgoing())
            {
                InternalIndex newIndex;
                newIndex.Key_ = key;
                std::set<InternalIndex>::iterator exist = std::find(PendingMessages_[aimId].begin(), PendingMessages_[aimId].end(), newIndex);
                if (exist != PendingMessages_[aimId].end())
                {
                    assert(key.hasId());
                    assert(!key.InternalId_.isEmpty());
                    emit messageIdFetched(aimId, key);

                    const auto &existingKeys = exist->GetMessageKeys();
                    for (auto iter : existingKeys)
                    {
                        Messages_[aimId].erase(internal(iter));
                    }

                    PendingMessages_[aimId].erase(*exist);
                }
                else
                {
                    if (!PendingMessages_[aimId].empty())
                    {
                        std::set<InternalIndex>::reverse_iterator last = PendingMessages_[aimId].rbegin();
                        while (last != PendingMessages_[aimId].rend())
                        {
                            const auto &existingKeys = last->GetMessageKeys();

                            std::set<MessageKey>::iterator existKey = std::find(existingKeys.begin(), existingKeys.end(), key);
                            if (existKey != existingKeys.end())
                            {
                                Messages_[aimId].erase(internal(*existKey));
                                last->RemoveMessageKey(*existKey);
                                if (existingKeys.empty())
                                {
                                    PendingMessages_[aimId].erase(last.base());
                                }
                                else
                                {
                                    QList<MessageKey> updatedValues;
                                    updatedValues << last->Key_;
                                    emitUpdated(updatedValues, aimId, PENDING);
                                }
                                break;
                            }
                            ++last;
                        }
                    }

                    if (!Indexes_[aimId].empty())
                    {
                        std::set<InternalIndex>::reverse_iterator last = Indexes_[aimId].rbegin();
                        while (last != Indexes_[aimId].rend())
                        {
                            if (last->Key_.isOutgoing())
                            {
                                const auto &existingKeys = last->GetMessageKeys();

                                std::set<MessageKey>::iterator existKey = std::find(existingKeys.begin(), existingKeys.end(), key);
                                if (existKey != existingKeys.end() && existKey->isPending())
                                {
                                    Messages_[aimId].erase(internal(*existKey));
                                    last->RemoveMessageKey(*existKey);
                                    QList<MessageKey> updatedValues;
                                    updatedValues << last->Key_;
                                    emitUpdated(updatedValues, aimId, PENDING);
                                    break;
                                }
                            }
                            ++last;
                        }
                    }
                }
                ++iter;
            }
            else
            {
                ++iter;
            }
        }


        QList<MessageKey> updatedValues;
        for (auto iter : indexes)
        {
            PendingMessages_[aimId].emplace(iter);
            updatedValues << iter.Key_;
        }

        if (state != Ui::MessagesBuddiesOpt::Requested)
            emitUpdated(updatedValues, aimId, BASE);
    }

    Data::MessageBuddy MessagesModel::item(const InternalIndex& index)
    {
        Data::MessageBuddy result;

        result.Id_ = index.Key_.Id_;
        result.Prev_ = index.Key_.Prev_;
        result.InternalId_ = index.Key_.InternalId_;

        auto first = true;
        for (const auto& id : index.GetMessageKeys())
        {
            const auto &messages = Messages_[index.AimId_];

            auto msg = messages.find(internal(id));
            if (msg == messages.end())
            {
                continue;
            }

            if (first)
            {
                result.SetType(index.Key_.Type_);
                result.AimId_ = index.AimId_;
                result.SetOutgoing(msg->Buddy_->IsOutgoing());
                result.Chat_ = msg->Buddy_->Chat_;
                result.SetChatSender(msg->Buddy_->GetChatSender());
                result.ChatFriendly_ = msg->Buddy_->ChatFriendly_;
                result.SetHasAvatar(msg->Buddy_->HasAvatar());
                result.SetIndentBefore(msg->Buddy_->GetIndentBefore());
            }

            result.FillFrom(*msg->Buddy_, !first);

            first = false;

            result.SetTime(index.MaxTime_);

            result.SetFileSharing(index.GetFileSharing());
            result.SetSticker(index.GetSticker());
            result.SetChatEvent(index.GetChatEvent());
            result.SetVoipEvent(index.GetVoipEvent());

            if (id.Id_ == -1)
            {
                result.Id_ = -1;
            }
        }

        if (first)
        {
            result.Id_ = -1;
            result.InternalId_.clear();
        }

        return result;
    }

    bool MessagesModel::tryInsertPendingMessageToLast(std::shared_ptr<Data::MessageBuddies> msgs, const QString& aimId)
    {
        QList<MessageKey> updatedValues;

        auto inserted = false;

        auto &pendingMessages = PendingMessages_[aimId];
        auto &indexes = Indexes_[aimId];
        auto &messages = Messages_[aimId];

        for (auto msg : *msgs)
        {
            if (!pendingMessages.empty())
            {
                auto last = pendingMessages.rbegin();

                const auto messageTimeGreaterThanMergeRangeStart = ((last->MinTime_ - 120) <= msg->GetTime());
                const auto messageTimeLesserThanMergeRangeEnd = ((last->MaxTime_ + 120) >= msg->GetTime());
                const auto messageTimeIsInMergeRage = (messageTimeGreaterThanMergeRangeStart && messageTimeLesserThanMergeRangeEnd);
                const auto mergeMessages = (messageTimeIsInMergeRage && !last->IsStandalone() && !msg->IsStandalone());

                if (mergeMessages)
                {
                    MessageInternal message(msg);
                    messages.insert(message);

                    last->MaxTime_ = std::max(last->MaxTime_, msg->GetTime());
                    last->MinTime_ = std::min(last->MinTime_, msg->GetTime());
                    last->InsertMessageKey(message.Key_);
                    inserted = true;
                    if (!updatedValues.contains(last->Key_))
                        updatedValues << last->Key_;
                }
            }

            if (!inserted && !indexes.empty())
            {
                auto &last = *indexes.rbegin();

                const auto messageTimeGreaterThanMergeRangeStart = ((last.MinTime_ - 120) <= msg->GetTime());
                const auto messageTimeLesserThanMergeRangeEnd = ((last.MaxTime_ + 120) >= msg->GetTime());
                const auto messageTimeIsInMergeRage = (messageTimeGreaterThanMergeRangeStart && messageTimeLesserThanMergeRangeEnd);
                const auto mergeMessages = (last.Key_.isOutgoing() && messageTimeIsInMergeRage && !last.IsStandalone() && !msg->IsStandalone());

                if (!PendingMessages_[aimId].empty() && last.Key_.Id_ <= PendingMessages_[aimId].rbegin()->LastIndex_)
                    continue;

                if (mergeMessages)
                {
                    MessageInternal message(msg);
                    messages.insert(message);

                    last.MaxTime_ = std::max(last.MaxTime_, msg->GetTime());
                    last.MinTime_ = std::min(last.MinTime_, msg->GetTime());

                    const auto &existingKeys = last.GetMessageKeys();
                    if (std::find(existingKeys.begin(), existingKeys.end(), message.Key_) == existingKeys.end())
                    {
                        last.InsertMessageKey(message.Key_);
                        inserted = true;
                        if (!updatedValues.contains(last.Key_))
                            updatedValues << last.Key_;
                    }
                }
            }
        }

        if (inserted)
            emitUpdated(updatedValues, aimId, BASE);

        return inserted;
    }

    void MessagesModel::sendDeliveryNotifications(const Data::MessageBuddies &msgs)
    {
        assert(!msgs.empty());

        __INFO(
            "delivery",
            "sending delivery notifications to widgets\n" <<
            "	count=<" << msgs.size() << ">");

        for (const auto &msg : msgs)
        {
            assert(msg->CheckInvariant());

            if (!msg->IsOutgoing())
            {
                // only outgoing messages can be delivered
                continue;
            }

            if (msg->InternalId_.isEmpty())
            {
                // we should not notify outgoing message items recovered from the storage
                continue;
            }

            if (msg->IsDeliveredToClient())
            {
                __TRACE(
                    "delivery",
                    "sending delivery notification\n" <<
                    "	type=<client>\n" <<
                    "	dst=<" << msg->InternalId_ << ">");

            //    emit deliveredToClient(msg->InternalId_);
                continue;
            }

            if (msg->IsDeliveredToServer())
            {
                __TRACE(
                    "delivery",
                    "sending delivery notification\n" <<
                    "	type=<server>\n" <<
                    "	dst=<" << msg->InternalId_ << ">");

                emit deliveredToServer(msg->InternalId_);
            }
        }
    }

    void MessagesModel::emitUpdated(const QList<Logic::MessageKey>& list, const QString& aimId, unsigned mode)
    {
        bool containsChatEvent = false;
        for (auto iter : list)
        {
            if (iter.isChatEvent())
            {
                containsChatEvent = true;
                break;
            }
        }

        if (containsChatEvent)
            emit chatEvent(aimId);

        if (list.size() <= moreCount())
        {
            emit updated(list, aimId, mode);
            return;
        }

        int i = 0;
        QList<Logic::MessageKey> updatedList;
        for (auto iter : list)
        {
            if (++i < moreCount())
            {
                updatedList.push_back(iter);
                continue;
            }

            updatedList.push_back(iter);

            i = 0;
            emit updated(updatedList, aimId, mode);
            updatedList.clear();
        }

        if (!updatedList.isEmpty())
            emit updated(updatedList, aimId, mode);
    }

    void MessagesModel::emitDeleted(const QList<Logic::MessageKey>& list, const QString& aimId)
    {
        emit deleted(list, aimId);
    }

    void MessagesModel::createFileSharingWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const
    {
        std::unique_ptr<HistoryControl::MessageContentWidget> item;
        if (messageBuddy.ContainsPttAudio())
        {
            item.reset(new HistoryControl::PttAudioWidget(parent, messageBuddy.AimId_, messageBuddy.IsOutgoing(), messageBuddy.GetText(), messageBuddy.GetPttDuration(), messageBuddy.Id_, messageBuddy.Prev_));
        }
        else
        {
            const auto previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);

            item.reset(
                new HistoryControl::FileSharingWidget(
                    parent,
                    messageBuddy.IsOutgoing(),
                    messageBuddy.AimId_,
                    messageBuddy.GetFileSharing(),
                    previewsEnabled));

            connect(
                item.get(),
                &HistoryControl::MessageContentWidget::removeMe,
                [messageBuddy]
                {
                    QList<MessageKey> keys;
                    keys << messageBuddy.ToKey();
                    emit GetMessagesModel()->deleted(keys, messageBuddy.AimId_);
                });
        }

        messageItem.setContentWidget(item.release());
    }

    void MessagesModel::createImagePreviewWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const
    {
        const auto uri = messageBuddy.GetFirstUriFromText();
        const auto &fullText = messageBuddy.GetText();
        const auto previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);

        auto item = new HistoryControl::ImagePreviewWidget(parent, messageBuddy.IsOutgoing(), uri.toString(), fullText, previewsEnabled, messageItem.getContact());
        item->setMaximumWidth(ItemWidth_);

        messageItem.setContentWidget(item);
    }

    void MessagesModel::createStickerWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const
    {
        auto item = new HistoryControl::StickerWidget(parent, messageBuddy.GetSticker(), messageBuddy.IsOutgoing(), messageItem.getContact());
        item->setFixedWidth(ItemWidth_);

        messageItem.setContentWidget(item);
        messageItem.setStickerText(messageBuddy.GetText());
    }

    MessageKey MessagesModel::applyMessageModification(const QString& aimId, Data::MessageBuddy& modification)
    {
        assert(modification.AimId_ == aimId);

        const auto &messages = Messages_[aimId];
        auto &index = Indexes_[aimId];

        auto key = modification.ToKey();
        assert(key.hasId());

        // find index

        const auto existingIndexIter = findIndexRecord(index, key);
        const auto isIndexMissing = (existingIndexIter == index.end());
        if (isIndexMissing)
        {
            __INFO(
                "delete_history",
                "modification patch skipped\n"
                "    reason=<no-index>\n"
                "    message-id=<" << key.Id_ << ">");

            return MessageKey();
        }

        // apply modification to the index record

        const auto &existingIndex = *existingIndexIter;
        assert(!existingIndex.IsDeleted());

        key = existingIndex.GetKeyById(key.Id_);
        assert(!key.isEmpty());

        auto modifiedIndex = existingIndex;
        modifiedIndex.ApplyModification(modification);

        auto insertionIter = existingIndexIter;
        ++insertionIter;

        index.erase(existingIndexIter);

        index.emplace_hint(insertionIter, modifiedIndex);

        // find message

        const auto existingMessageIter = messages.find(internal(key));
        const auto isMessageMissing = (existingMessageIter == messages.end());
        if (isMessageMissing)
        {
            __INFO(
                "delete_history",
                "modification patch skipped\n"
                "    reason=<no-message>\n"
                "    message-id=<" << key.Id_ << ">");

            return key;
        }

        // apply modification to the message

        auto &existingMessage = *existingMessageIter->Buddy_;
        existingMessage.ApplyModification(modification);

        return key;
    }

    bool MessagesModel::hasItemsInBetween(const QString &aimId, const InternalIndex &l, const InternalIndex &r) const
    {
        assert(!aimId.isEmpty());
        assert(l < r);

        const auto &indexRecords = Indexes_[aimId];

        const auto iterL = indexRecords.upper_bound(l);
        const auto iterR = indexRecords.lower_bound(r);

        for (auto iter = iterL; iter != iterR; ++iter)
        {
            if (!iter->IsDeleted())
            {
                return true;
            }
        }

        return false;
    }

    void MessagesModel::updateDateItems(const QString& aimId)
    {
        assert(!aimId.isEmpty());

        auto &dateRecords = DateItems_[aimId];

        QList<MessageKey> toRemove;
        toRemove.reserve(dateRecords.size());

        for(
            auto dateRecordsIter = dateRecords.cbegin();
            dateRecordsIter != dateRecords.cend();
        )
        {
            const auto &dateIndexRecord = dateRecordsIter->second;

            InternalIndex nextDateIndexRecord;

            auto nextDateRecordsIter = dateRecordsIter;
            ++nextDateRecordsIter;

            const auto isLastDateRecord = (nextDateRecordsIter == dateRecords.cend());
            if (isLastDateRecord)
            {
                nextDateIndexRecord.Key_ = MessageKey::MAX;
            }
            else
            {
                nextDateIndexRecord = nextDateRecordsIter->second;
            }

            const auto hasItemsBetweenDates = hasItemsInBetween(aimId, dateIndexRecord, nextDateIndexRecord);
            if (!hasItemsBetweenDates)
            {
                toRemove << dateIndexRecord.Key_;
                dateRecords.erase(dateRecordsIter);
            }

            dateRecordsIter = nextDateRecordsIter;
        }

        emitDeleted(toRemove, aimId);
    }

    void MessagesModel::updateMessagesMargins(const QString& aimId)
    {
        assert(!aimId.isEmpty());

        const auto &messages = Messages_[aimId];

        auto messagesIter = messages.crbegin();
        for(;;)
        {
            for (;;)
            {
                const auto isFirstElementReached = (messagesIter == messages.crend());
                if (isFirstElementReached)
                {
                    return;
                }

                if (!messagesIter->Buddy_->IsDeleted())
                {
                    break;
                }

                ++messagesIter;
            }

            auto &message = *messagesIter->Buddy_;
            const auto &messageKey = messagesIter->Key_;

            for (;;)
            {
                ++messagesIter;

                const auto isFirstElementReached = (messagesIter == messages.crend());
                if (isFirstElementReached)
                {
                    if (message.GetIndentBefore())
                    {
                        message.SetIndentBefore(false);
                        emit indentChanged(messageKey, false);
                    }

                    return;
                }

                if (!messagesIter->Buddy_->IsDeleted())
                {
                    break;
                }
            }

            const auto &prevMessage = *messagesIter->Buddy_;

            const auto oldMessageIndent = message.GetIndentBefore();
            const auto newMessageIndent = message.GetIndentWith(prevMessage);

            if (newMessageIndent == oldMessageIndent)
            {
                continue;
            }

            message.SetIndentBefore(newMessageIndent);

            emit indentChanged(messageKey, newMessageIndent);
        }
    }

    void MessagesModel::addDateItem(const QString &aimId, const MessageKey &key, const QDate &date)
    {
        assert(!aimId.isEmpty());
        assert(date.isValid());
        assert(key.hasId());
        assert(key.control_type_ == control_type::ct_date);

        auto message = std::make_shared<Data::MessageBuddy>();
        message->Id_ = key.Id_;
        message->Prev_ = key.Prev_;
        message->AimId_ = aimId;
        message->SetTime(0);
        message->SetDate(date);
        message->SetType(core::message_type::undefined);

        MessageInternal messageInternal(message);
        messageInternal.Key_ = key;
        messageInternal.Key_.control_type_ = control_type::ct_date;

        Messages_[aimId].emplace(messageInternal);

        InternalIndex dateIndex;
        dateIndex.Key_ = key;
        dateIndex.Date_ = date;
        DateItems_[aimId].emplace(date, dateIndex);

        __INFO(
            "gui_dates",
            "added gui date item\n"
            "    contact=<" << aimId << ">\n"
            "    id=<" << key.Id_ << ">\n"
            "    prev=<" << key.Prev_ << ">"
            );
    }

    bool MessagesModel::hasDate(const QString &aimId, const QDate &date) const
    {
        assert(!aimId.isEmpty());

        return (DateItems_[aimId].count(date) > 0);
    }

    void MessagesModel::removeDateItem(const QString &aimId, const QDate &date)
    {
        assert(!aimId.isEmpty());

        DateItems_[aimId].erase(date);
    }

    void MessagesModel::removeDateItems(const QString &aimId)
    {
        assert(!aimId.isEmpty());

        DateItems_.remove(aimId);
    }

    void MessagesModel::removeDateItemIfOutdated(const QString &aimId, const Data::MessageBuddy &msg)
    {
        assert(!aimId.isEmpty());
        assert(msg.HasId());

        const auto msgKey = msg.ToKey();

        const auto &date = msg.GetDate();

        auto &dateItems = DateItems_[aimId];

        auto dateItemIter = dateItems.find(date);
        if (dateItemIter == dateItems.end())
        {
            return;
        }

        auto &indexRecord = dateItemIter->second;

        const auto isOutdated = (msgKey < indexRecord.Key_);
        if (!isOutdated)
        {
            return;
        }

        auto &index = Indexes_[aimId];
        index.erase(indexRecord);

        auto &messages = Messages_[aimId];
        messages.erase(internal(indexRecord.Key_));

        QList<Logic::MessageKey> toRemove;
        toRemove << indexRecord.Key_;

        emitDeleted(toRemove, aimId);

        dateItems.erase(dateItemIter);
    }

    InternalIndexSetIter MessagesModel::findIndexRecord(InternalIndexSet &indexRecords, const Logic::MessageKey &key) const
    {
        assert(!key.isEmpty());

        return std::find_if(
            indexRecords.begin(),
            indexRecords.end(),
            [&key](const InternalIndex &index)
            {
                return index.ContainsKey(key);
            });
    }

    Logic::MessageKey MessagesModel::findFirstKeyAfter(const QString &aimId, const Logic::MessageKey &key) const
    {
        assert(!aimId.isEmpty());
        assert(!key.isEmpty());

        const auto &messages = Messages_[aimId];

        auto messageIter = messages.upper_bound(internal(key));
        if ((messageIter == messages.begin()) ||
            (messageIter == messages.end()))
        {
            return Logic::MessageKey();
        }

        return messageIter->Key_;
    }

    void MessagesModel::requestMessages(const QString& aimId)
    {
        assert(!aimId.isEmpty());

        if (!Requested_.contains(aimId))
            Requested_.append(aimId);

        qint64 lastId = Messages_[aimId].empty() ? -1 : Messages_[aimId].begin()->Buddy_->Id_;
        qint64 lastPrev = Messages_[aimId].empty() ? -1 : Messages_[aimId].begin()->Buddy_->Prev_;

        if (LastRequested_.contains(aimId) && LastRequested_[aimId] == lastId)
            return;

        if (lastId != -1 && lastPrev == -1)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        collection.set_value_as_int64("from", lastId);
        collection.set_value_as_int64("count", PRELOAD_COUNT);
        Sequences_ << Ui::GetDispatcher()->post_message_to_core("archive/messages/get", collection.get());
        LastRequested_[aimId] = lastId;
    }

    Ui::HistoryControlPageItem* MessagesModel::fill(const Data::MessageBuddy& msg, QWidget* parent) const
    {
        if (msg.IsEmpty())
            return 0;

        auto dlgState = Logic::GetRecentsModel()->getDlgState(msg.AimId_);

        qint64 lastReadId = dlgState.TheirsLastRead_;

        if (!dlgState.Outgoing_)
        {
            lastReadId = dlgState.LastMsgId_;
        }
        
        bool isLastRead = (!msg.Chat_ && msg.Id_ != -1 && msg.Id_ == lastReadId);

        const auto isServiceMessage = (!msg.IsBase() && !msg.IsFileSharing() && !msg.IsSticker() && !msg.IsChatEvent() && !msg.IsVoipEvent());
        if (isServiceMessage)
        {
            std::unique_ptr<Ui::ServiceMessageItem> serviceMessageItem(new Ui::ServiceMessageItem(parent));

            serviceMessageItem->setDate(msg.GetDate());
            serviceMessageItem->setWidth(ItemWidth_);
            serviceMessageItem->setContact(msg.AimId_);
            serviceMessageItem->updateStyle();

            return serviceMessageItem.release();
        }

        if (msg.IsChatEvent())
        {
            std::unique_ptr<Ui::ChatEventItem> item(new Ui::ChatEventItem(parent, msg.GetChatEvent()));
            item->setContact(msg.AimId_);
            item->setHasAvatar(msg.HasAvatar());
            item->setFixedWidth(ItemWidth_);
            return item.release();
        }

        if (msg.IsVoipEvent())
        {
            const auto &voipEvent = msg.GetVoipEvent();

            std::unique_ptr<Ui::VoipEventItem> item(new Ui::VoipEventItem(parent, voipEvent));
            item->setTopMargin(msg.GetIndentBefore());
            item->setFixedWidth(ItemWidth_);
            item->setHasAvatar(voipEvent->isIncomingCall());
            item->setId(msg.Id_);
            item->setLastRead(isLastRead);
            return item.release();
        }

        if (msg.IsDeleted())
        {
            std::unique_ptr<Ui::DeletedMessageItem> deletedItem(new Ui::DeletedMessageItem(parent));
            return deletedItem.release();
        }

        std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(parent));
        messageItem->setContact(msg.AimId_);
        messageItem->setId(msg.Id_, msg.AimId_);
        messageItem->setNotificationKeys(msg.GetNotificationKeys());

        const auto sender =
            (msg.Chat_ && msg.HasChatSender()) ?
            NormalizeAimId(msg.GetChatSender()) :
            msg.AimId_;

        messageItem->setSender(sender);

        if (msg.HasAvatar())
        {
            messageItem->loadAvatar(Utils::scale_bitmap(Ui::MessageStyle::getAvatarSize()));
        }
        else
        {
            messageItem->setAvatarVisible(false);
        }

        messageItem->setTopMargin(msg.GetIndentBefore());
        messageItem->setOutgoing(msg.IsOutgoing(), msg.IsDeliveredToServer(), msg.Chat_, true);
        messageItem->setLastRead(isLastRead);
        messageItem->setMchatSender(GetChatFriendly(msg.GetChatSender(), msg.ChatFriendly_));
        messageItem->setMchatSenderAimId(msg.HasChatSender() ? msg.GetChatSender(): msg.AimId_);
        messageItem->setTime(msg.GetTime());
        messageItem->setDate(msg.GetDate());

        if (msg.IsFileSharing())
        {
            createFileSharingWidget(*messageItem, messageItem.get(), msg);
        }
        else if (msg.IsSticker())
        {
            createStickerWidget(*messageItem, messageItem.get(), msg);
        }
        else if (msg.ContainsPreviewableLink())
        {
            createImagePreviewWidget(*messageItem, messageItem.get(), msg);
        }
        else
        {
            messageItem->setMessage(msg.GetText());
        }

        return messageItem.release();
    }

    QWidget* MessagesModel::fillNew(const InternalIndex& index, QWidget* parent, qint64 newId)
    {
        assert(parent);

        std::unique_ptr<QWidget> result(new QWidget(parent));
        QVBoxLayout* layout = new QVBoxLayout(result.get());
        result->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        layout->setDirection(QBoxLayout::TopToBottom);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        result->setLayout(layout);

        auto buddy = std::make_shared<Data::MessageBuddy>();
        buddy->Id_ = index.Key_.Id_;
        bool first = true;
        for (auto id : index.GetMessageKeys())
        {
            std::set<MessageInternal>::const_iterator msg = Messages_[index.AimId_].find(internal(id));
            if (msg == Messages_[index.AimId_].end())
                continue;

            auto dlgState = Logic::GetRecentsModel()->getDlgState(buddy->AimId_);

            qint64 lastReadId = dlgState.TheirsLastRead_;

            if (!dlgState.Outgoing_)
            {
                lastReadId = dlgState.LastMsgId_;
            }

            bool isLastRead = (!buddy->Chat_ && buddy->Id_ != -1 && buddy->Id_ == lastReadId);

            if (first)
            {
                buddy->SetType(index.Key_.Type_);
                buddy->AimId_ = index.AimId_;
                buddy->SetOutgoing(msg->Buddy_->IsOutgoing());
                buddy->Chat_ = msg->Buddy_->Chat_;
                buddy->SetChatSender(msg->Buddy_->GetChatSender());
                buddy->ChatFriendly_ = msg->Buddy_->ChatFriendly_;
                buddy->SetHasAvatar(msg->Buddy_->HasAvatar());
                buddy->SetIndentBefore(msg->Buddy_->GetIndentBefore());
            }

            buddy->FillFrom(*msg->Buddy_, !first);

            first = false;

            buddy->SetTime(index.MaxTime_);

            if (id.Id_ != newId)
            {
                continue;
            }

            if (index.IsChatEvent())
            {
                std::unique_ptr<Ui::ChatEventItem> item(new Ui::ChatEventItem(parent, buddy->GetChatEvent()));
                item->setContact(buddy->AimId_);
                item->setHasAvatar(buddy->HasAvatar());
                item->setFixedWidth(ItemWidth_);
                result->layout()->addWidget(item.release());
            }
            else if (index.IsVoipEvent())
            {
                std::unique_ptr<Ui::VoipEventItem> item(new Ui::VoipEventItem(parent, buddy->GetVoipEvent()));
                item->setMaximumWidth(ItemWidth_);
                item->setHasAvatar(buddy->HasAvatar());
                item->setTopMargin(buddy->GetIndentBefore());
                item->setId(buddy->Id_);
                item->setLastRead(isLastRead);
                result->layout()->addWidget(item.release());
            }
            else
            {
                std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(result.get()));
                messageItem->setId(buddy->Id_, buddy->AimId_);
                messageItem->setNotificationKeys(buddy->GetNotificationKeys());
                messageItem->setSender(buddy->Chat_ ? NormalizeAimId(buddy->GetChatSender()) : buddy->AimId_);

                if (buddy->HasAvatar())
                {
                    messageItem->loadAvatar(Utils::scale_bitmap(Ui::MessageStyle::getAvatarSize()));
                }
                else
                {
                    messageItem->setAvatarVisible(false);
                }
                messageItem->setMchatSender(GetChatFriendly(buddy->GetChatSender(), buddy->ChatFriendly_));
                messageItem->setTopMargin(buddy->GetIndentBefore());
                messageItem->setOutgoing(buddy->IsOutgoing(), buddy->IsDeliveredToServer(), buddy->Chat_);
                messageItem->setLastRead(isLastRead);
                messageItem->setContact(buddy->AimId_);
                messageItem->setTime(buddy->GetTime());
                messageItem->setDate(buddy->GetDate());

                if (!buddy->IsDeleted())
                {
                    if (buddy->IsFileSharing())
                    {
                        createFileSharingWidget(*messageItem, parent, *buddy);
                    }
                    else if (buddy->IsSticker())
                    {
                        createStickerWidget(*messageItem, parent, *buddy);
                    }
                    else if (buddy->ContainsPreviewableLink())
                    {
                        createImagePreviewWidget(*messageItem, messageItem.get(), *buddy);
                    }
                    else
                    {
                        messageItem->setMessage(buddy->GetText());
                    }
                }

                result->layout()->addWidget(messageItem.release());
                buddy->SetText("");
            }

            std::unique_ptr<Ui::ServiceMessageItem> newPlate(
                new Ui::ServiceMessageItem(result.get())
                );
            newPlate->setWidth(ItemWidth_);
            newPlate->setNew();
            newPlate->setContact(buddy->AimId_);
            newPlate->updateStyle();
            result->layout()->addWidget(newPlate.release());
        }

        if (buddy->HasText() && buddy->IsBase())
        {
            std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(result.get()));
            messageItem->setId(buddy->Id_, buddy->AimId_);
            messageItem->setNotificationKeys(buddy->GetNotificationKeys());
            messageItem->loadAvatar(Utils::scale_bitmap(Ui::MessageStyle::getAvatarSize()));
            messageItem->setTopMargin(false);
            messageItem->setOutgoing(buddy->IsOutgoing(), buddy->IsDeliveredToServer(), buddy->Chat_);
            messageItem->setMchatSender(GetChatFriendly(buddy->GetChatSender(), buddy->ChatFriendly_));
            messageItem->setTime(buddy->GetTime());
            messageItem->setDate(buddy->GetDate());
            messageItem->setMessage(buddy->GetText());
            result->layout()->addWidget(messageItem.release());
        }

        result->setFixedWidth(ItemWidth_);
        result->setProperty("New", true);
        return first ?  0 : result.release();
    }

    QWidget* MessagesModel::fillItemById(const QString& aimId, const MessageKey& key,  QWidget* parent, qint64 newId)
    {
        assert(!aimId.isEmpty());

        if (FailedUploads_.contains(key.InternalId_))
        {
            FailedUploads_.removeAll(key.InternalId_);
            return nullptr;
        }

        const auto &pendingMessages = PendingMessages_[aimId];
        auto current = pendingMessages.crbegin();
        while (current != pendingMessages.crend())
        {
            if (current->Key_ == key)
            {
                break;
            }

            ++current;
        }


        if (current != pendingMessages.crend())
        {
            // merge message widgets
            auto result = item(*current);
            return fill(result, parent);
        }

        const auto &indexes = Indexes_[aimId];

        auto haveIncoming = false;
        current = indexes.crbegin();
        while (current != indexes.crend())
        {
            haveIncoming |= !current->Key_.isOutgoing();

            if (current->Key_ == key)
            {
                break;
            }

            ++current;
        }

        if (current == indexes.crend())
        {
            return nullptr;
        }

        if (newId != -1)
        {
            const auto releaseVoodoo = ((current != indexes.rbegin() && current->ContainsId(newId)) || current->containsNotLast(newId));
            const auto createNew = (
                haveIncoming && !current->IsPending() &&
                (current->IsBase() || current->IsFileSharing() || current->IsSticker() || current->IsChatEvent() || current->IsVoipEvent())
                && releaseVoodoo);
            if (createNew)
            {
                return fillNew(*current, parent, newId);
            }
        }

        // merge message widgets
        auto result = item(*current);
        return fill(result, parent);
    }

    void MessagesModel::setFirstMessage(const QString& aimId, qint64 msgId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        collection.set_value_as_int64("message", msgId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/set_first_message", collection.get());
    }

    std::set<MessageInternal>::iterator MessagesModel::previousMsg(const QString& aimId, const qint64 id)
    {
        assert(!aimId.isEmpty());
        assert(id >= -1);

        const auto &messages = Messages_[aimId];

        const auto firstMessage = (id == -1);
        if (firstMessage)
        {
            return messages.end();
        }

        for (auto iter = messages.begin(); iter != messages.end(); ++iter)
        {
            const auto &key = iter->Key_;

            if (key.Id_ != id)
            {
                continue;
            }

            if (key.isDate() || key.isDeleted())
            {
                continue;
            }

            return iter;
        }

        return messages.end();
    }

    void MessagesModel::contactChanged(QString contact)
    {
        if (contact.isEmpty() || LastRequested_.contains(contact))
        {
            return;
        }

        requestMessages(contact);
    }

    void MessagesModel::setItemWidth(int width)
    {
        ItemWidth_ = width;
    }

    QMap<MessageKey, QWidget*> MessagesModel::tail(const QString& aimId, QWidget* parent, qint64 newId)
    {
        QMap<MessageKey, QWidget*> result;
        if (Indexes_[aimId].empty() && PendingMessages_[aimId].empty())
            return result;

        int i = 0;
        MessageKey key;

        std::set<InternalIndex>::reverse_iterator iter = PendingMessages_[aimId].rbegin();
        while (iter != PendingMessages_[aimId].rend())
        {
            assert(!iter->IsDeleted());

            key = iter->Key_;
            Data::MessageBuddy buddy = item(*iter);
            result.insert(key, fill(buddy, parent));
            if (++i == moreCount())
            {
                break;
            }

            ++iter;
        }

        if (i < moreCount())
        {
            const auto &indexesByAimid = Indexes_[aimId];

            bool haveImcoming = false;
            iter = indexesByAimid.rbegin();
            while (iter != indexesByAimid.rend())
            {
                haveImcoming |= !iter->Key_.isOutgoing();
                key = iter->Key_;

                if (newId != -1 &&
                    haveImcoming &&
                    !iter->IsPending() &&
                    (iter->IsBase() || iter->IsFileSharing() || iter->IsSticker() || iter->IsChatEvent() || iter->IsVoipEvent()) &&
                    ((iter != indexesByAimid.rbegin() && iter->ContainsId(newId)) || iter->containsNotLast(newId)))
                {
                    result.insert(key, fillNew(*iter, parent, newId));
                }
                else
                {
                    auto buddy = item(*iter);
                    result.insert(key, fill(buddy, parent));
                }

                if (++i == moreCount())
                {
                    break;
                }

                ++iter;
            }
        }

        LastKey_[aimId] = key;

        if (LastKey_[aimId].isEmpty() || LastKey_[aimId].isPending() || (LastKey_[aimId].Prev_ != -1 && Indexes_[aimId].begin()->ContainsId(LastKey_[aimId].Id_)))
            requestMessages(aimId);

        if (result.isEmpty())
            Subscribed_ << aimId;

        return result;
    }

    QMap<MessageKey, QWidget*> MessagesModel::more(const QString& aimId, QWidget* parent, qint64 newId)
    {
        if (LastKey_[aimId].isEmpty())
            return tail(aimId, parent, newId);

        QMap<MessageKey, QWidget*> result;
        if (Indexes_[aimId].empty())
            return result;

        bool haveIncoming = false;
        auto i = 0;
        MessageKey key = LastKey_[aimId];

        std::set<InternalIndex>::const_reverse_iterator iter = PendingMessages_[aimId].rbegin();
        while (iter != PendingMessages_[aimId].rend())
        {
            assert(!iter->IsDeleted());

            if (!(iter->Key_ < key))
            {
                ++iter;
                continue;
            }

            haveIncoming |= !iter->Key_.isOutgoing();
            key = iter->Key_;
            Data::MessageBuddy buddy = item(*iter);
            result.insert(key, fill(buddy, parent));

            if (++i == moreCount())
            {
                break;
            }

            ++iter;
        }

        if (i < moreCount())
        {
            iter = Indexes_[aimId].rbegin();
            while (iter != Indexes_[aimId].rend())
            {
                haveIncoming |= !iter->Key_.isOutgoing();
                if (!(iter->Key_ < key))
                {
                    ++iter;
                    continue;
                }

                key = iter->Key_;

                if (newId != -1 && haveIncoming && !iter->IsPending() &&
                    (iter->IsBase() || iter->IsFileSharing() || iter->IsSticker() || iter->IsChatEvent() || iter->IsVoipEvent()) &&
                    ((iter != Indexes_[aimId].rbegin() && iter->ContainsId(newId)) || iter->containsNotLast(newId)))
                {
                    result.insert(key, fillNew(*iter, parent, newId));
                }
                else
                {
                    Data::MessageBuddy buddy = item(*iter);
                    result.insert(key, fill(buddy, parent));
                }

                if (++i == moreCount())
                {
                    break;
                }

                ++iter;
            }
        }

        LastKey_[aimId] = key;

        if (
            LastKey_[aimId].isEmpty() ||
            LastKey_[aimId].isPending() ||
            (
                (LastKey_[aimId].Prev_ != -1) &&
                Indexes_[aimId].begin()->ContainsId(LastKey_[aimId].Id_)
            )
        )
        {
            requestMessages(aimId);
        }

        if (!result.isEmpty())
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_preload);
        }
        else
        {
            Subscribed_ << aimId;
        }

        return result;
    }

    QWidget* MessagesModel::getById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId)
    {
        return fillItemById(aimId, key, parent, newId);
    }

    int32_t MessagesModel::preloadCount() const
    {
        return PRELOAD_COUNT;
    }

    int32_t MessagesModel::moreCount() const
    {
        return MORE_COUNT;
    }

    void MessagesModel::setLastKey(const MessageKey& key, const QString& aimId)
    {
        assert(!key.isEmpty());
        assert(!aimId.isEmpty());

        __TRACE(
            "history_control",
            "setting a last key for the dialog\n"
            "	contact=<" << aimId << ">"
            "	key=<" << key.toLogStringShort() << ">");

        LastKey_[aimId] = key;

        auto &indexesByAimid = Indexes_[aimId];

        auto iter = indexesByAimid.begin();
        while(iter != indexesByAimid.end())
        {
            if (iter->Key_ < key)
            {
                for (auto id : iter->GetMessageKeys())
                {
                    Messages_[aimId].erase(internal(id));
                }

                if (iter->IsDate())
                {
                    removeDateItem(aimId, iter->Date_);
                }

                if (iter->Key_.Id_ <= LastRequested_[aimId])
                    LastRequested_[aimId] = -1;

                iter = indexesByAimid.erase(iter);
            }
            else
            {
                break;
            }
        }

        if ((int32_t)indexesByAimid.size() >= preloadCount())
        {
            setFirstMessage(aimId, indexesByAimid.begin()->Key_.Id_);
        }
    }

    void MessagesModel::removeDialog(const QString& aimId)
    {
        Messages_.remove(aimId);
        Indexes_.remove(aimId);
        LastKey_.remove(aimId);
        LastRequested_.remove(aimId);
        Requested_.removeAll(aimId);

        removeDateItems(aimId);
    }

    void MessagesModel::updateNew(const QString& aimId, qint64 newId, bool hide)
    {
        std::set<InternalIndex>::const_reverse_iterator iter = Indexes_[aimId].rbegin();
        while (iter != Indexes_[aimId].rend())
        {
            if (iter->ContainsId(newId))
            {
                QList<MessageKey> updatedValues;
                updatedValues << iter->Key_;

                bool outgoing = iter->Key_.isOutgoing();
                const auto &chatSender = iter->GetChatSender();
                bool chatEvent = iter->Key_.isChatEvent();
                if (iter != Indexes_[aimId].rbegin())
                {
                    --iter;
                    const auto &messages = Messages_[aimId];
                    auto msg = messages.find(internal(iter->Key_));
                    if (msg != messages.end() && !msg->Buddy_->IsOutgoing())
                    {
                        if (hide)
                            msg->Buddy_->SetHasAvatar(msg->Buddy_->Chat_ ? msg->Buddy_->GetChatSender() != chatSender  : outgoing);
                        else
                            msg->Buddy_->SetHasAvatar(true);

                        msg->Buddy_->SetHasAvatar(msg->Buddy_->HasAvatar() || chatEvent);
                        updatedValues << iter->Key_;
                    }
                }

                emitUpdated(updatedValues, aimId, NEW_PLATE);
                break;
            }
            ++iter;
        }
    }

    QString MessagesModel::formatRecentsText(const Data::MessageBuddy &buddy) const
    {
        if (buddy.IsServiceMessage())
        {
            return QString();
        }

        if (buddy.IsChatEvent())
        {
            std::unique_ptr<Ui::ChatEventItem> item(new Ui::ChatEventItem(buddy.GetChatEvent()));
            item->setContact(buddy.AimId_);
            return item->formatRecentsText();
        }

        if (buddy.IsVoipEvent())
        {
            std::unique_ptr<Ui::VoipEventItem> item(new Ui::VoipEventItem(buddy.GetVoipEvent()));
            return item->formatRecentsText();
        }

        QString photoStr = QT_TRANSLATE_NOOP("contact_list", "Photo");
        std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem());
        messageItem->setContact(buddy.AimId_);
        if (buddy.IsFileSharing())
        {
            if (buddy.ContainsPttAudio())
                return QT_TRANSLATE_NOOP("contact_list", "Voice message");
            else if (buddy.ContainsImage())
                return photoStr;

            auto item = new HistoryControl::FileSharingWidget(buddy.GetFileSharing(), buddy.AimId_);
            messageItem->setContentWidget(item);
        }
        else if (buddy.ContainsPreviewableLink())
        {
            return photoStr;
        }
        else if (buddy.IsSticker())
        {
            auto item = new HistoryControl::StickerWidget(buddy.AimId_);
            messageItem->setContentWidget(item);
        }
        else
        {
            messageItem->setMessage(buddy.GetText());
        }

        return messageItem->formatRecentsText();
    }

    int64_t MessagesModel::getLastMessageId(const QString &aimId) const
    {
        assert(!aimId.isEmpty());

        const auto &index = Indexes_[aimId];

        for (auto iterIndex = index.crbegin(); iterIndex != index.crend(); ++iterIndex)
        {
            const auto &keys = iterIndex->GetMessageKeys();
            assert(!keys.empty());

            for (auto iterKey = keys.crbegin(); iterKey != keys.crend(); ++iterKey)
            {
                const auto id = iterKey->Id_;
                assert(id >= -1);

                if (id > 0)
                {
                    return id;
                }
            }
        }

        return -1;
    }

    std::vector<int64_t> MessagesModel::getBubbleMessageIds(const QString &aimId, const int64_t messageId) const
    {
        assert(!aimId.isEmpty());
        assert(messageId > 0);

        const MessageKey key(messageId, -1, QString(), -1, 0, core::message_type::base, false, false, false, control_type::ct_message);

        InternalIndex keyIndex;
        keyIndex.Key_ = key;

        const auto &index = Indexes_[aimId];
        const auto indexIter = index.find(keyIndex);

        assert(indexIter != index.end());
        if (indexIter == index.end())
        {
            return std::vector<int64_t>();
        }

        const auto &indexEntry = *indexIter;

        const auto &messageKeys = indexEntry.GetMessageKeys();
        assert(!messageKeys.empty());

        std::vector<int64_t> ids;
        ids.reserve(messageKeys.size());

        for (const auto &key : messageKeys)
        {
            ids.push_back(key.Id_);
        }

        return ids;
    }

    qint64 MessagesModel::normalizeNewMessagesId(const QString& aimid, qint64 id)
    {
        if (id == -1)
            return id;

        qint64 newId = -1;
        std::set<MessageInternal>::reverse_iterator iter = Messages_[aimid].rbegin();
        while (iter != Messages_[aimid].rend() && iter->Key_.Id_ >= id)
        {
            if (iter->Key_.isOutgoing())
            {
                newId = iter->Key_.Id_;
                break;
            }

            newId = iter->Key_.Id_;
            ++iter;
        }

        return newId == -1 ? id : newId;
    }

    bool MessagesModel::isHasPending(const QString &aimId) const
    {
        auto iter = PendingMessages_.find(aimId);
        if (iter == PendingMessages_.end())
        {
            return false;
        }

        return !iter.value().empty();
    }

    MessagesModel* GetMessagesModel()
    {
        if (!g_messages_model)
            g_messages_model.reset(new MessagesModel(0));

        return g_messages_model.get();
    }

    void ResetMessagesModel()
    {
        if (g_messages_model)
            g_messages_model.reset();
    }
}