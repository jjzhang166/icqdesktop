#include "stdafx.h"

#include "MessagesModel.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../../utils/Text2DocConverter.h"

#include "MessageItem.h"
#include "ServiceMessageItem.h"
#include "ChatEventItem.h"

#include "../../core_dispatcher.h"
#include "../../../corelib/core_face.h"
#include "../../../corelib/enumerations.h"

#include "../history_control/FileSharingWidget.h"
#include "../history_control/FileSharingInfo.h"
#include "../history_control/ImagePreviewWidget.h"
#include "../history_control/StickerWidget.h"
#include "../history_control/VoipEventInfo.h"
#include "../history_control/VoipEventItem.h"
#include "../history_control/PttAudioWidget.h"

#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"

#include "../../gui_settings.h"

namespace
{
	static int PRELOAD_COUNT = 30;
	static int MORE_COUNT = 30;

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
    const MessageKey MessageKey::MAX(INT64_MAX, INT64_MAX - 1, QString(), -1, 0, core::message_type::base, false, false, control_type::ct_message);
    const MessageKey MessageKey::MIN(2, 1, QString(), -1, 0, core::message_type::base, false, false, control_type::ct_message);

    MessageKey::MessageKey()
        : Id_(-1)
        , Prev_(-1)
        , Type_(core::message_type::base)
        , Outgoing_(false)
        , PendingId_(-1)
        , Time_(-1)
        , IsPreview_(false)
        , control_type_(control_type::ct_message)
    {
    }

	MessageKey::MessageKey(qint64 id, qint64 prev, const QString& internalId, int pendingId, qint32 time, core::message_type type, bool outgoing, bool isPreview, control_type _control_type)
		: Id_(id)
		, InternalId_(internalId)
		, Type_(type)
		, Prev_(prev)
		, Outgoing_(outgoing)
		, PendingId_(pendingId)
        , Time_(time)
        , IsPreview_(isPreview)
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

	bool MessageKey::isOutgoing() const
	{
		return Outgoing_;
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

	bool InternalIndex::IsFileSharing() const
	{
		assert(Key_.Type_ > core::message_type::min);
		assert(Key_.Type_ < core::message_type::max);
		assert((Key_.Type_ != core::message_type::file_sharing) || FileSharing_);

		return (Key_.Type_ == core::message_type::file_sharing);
	}

	bool InternalIndex::IsPending() const
	{
		return !Key_.hasId() && !Key_.InternalId_.isEmpty();
	}

	bool InternalIndex::IsStandalone() const
	{
		return IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || IsPreview();
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

	const HistoryControl::FileSharingInfoSptr& InternalIndex::GetFileSharing() const
	{
		assert(!FileSharing_ || IsFileSharing());

		return FileSharing_;
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
				}
			)
		);

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

    std::unique_ptr<MessagesModel> g_messages_model;

	MessagesModel::MessagesModel(QObject *parent)
		: QObject(parent)
		, ItemWidth_(0)
		, DomUid_(1)
	{
		connect(
			Ui::GetDispatcher(),
			SIGNAL(messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64)),
			this,
			SLOT(messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64)),
			Qt::QueuedConnection);

		connect(
			Ui::GetDispatcher(),
			SIGNAL(dlgState(Data::DlgState)),
			this,
			SLOT(dlgState(Data::DlgState)),
			Qt::QueuedConnection);
	}

	int MessagesModel::generatedDomUid()
	{
		return DomUid_++;
	}

	void MessagesModel::dlgState(Data::DlgState state)
	{
		if (state.LastMsgId_ == -1)
		{
			return;
		}

		auto &indexes = Indexes_[state.AimId_];

		for (auto &ind : indexes)
		{
			if (!ind.Key_.isOutgoing())
			{
				// incoming messages should not be marked as delivered
				continue;
			}

            auto msgs = ind.GetMessageKeys();
            if (msgs.empty())
                continue;

            MessageInternal msg = *(msgs.rbegin());
			const auto idRead = (msg.Key_.Id_ <= state.TheirsLastDelivered_);
            auto iter = Messages_[state.AimId_].find(msg);
            if (iter == Messages_[state.AimId_].end())
                continue;

			const auto markAsRead = (idRead && !iter->Buddy_->IsDeliveredToClient());
			if (markAsRead)
			{
				iter->Buddy_->MarkAsDeliveredToClient();

                if (ind.Key_.InternalId_.isEmpty())
                    emit deliveredToClient(ind.Key_.Id_);
                else
                    emit deliveredToClient(ind.Key_.InternalId_);
			}
		}
	}

	void MessagesModel::messageBuddies(std::shared_ptr<Data::MessageBuddies> msgs, QString aimId, Ui::MessagesBuddiesOpt option, bool havePending, qint64 seq)
	{
		assert(option > Ui::MessagesBuddiesOpt::Min);
		assert(option < Ui::MessagesBuddiesOpt::Max);
		assert(msgs);

		const auto isDlgState = (option == Ui::MessagesBuddiesOpt::DlgState);
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
					"	seq=<" << seq << ">\n" <<
					buddy->GetFileSharing()->ToLogString());
			}
		}

		if (isDlgState && !msgs->empty())
		{
			sendDeliveryNotifications(*msgs);
		}

		if (isPending && tryInsertPendingMessageToLast(msgs, aimId))
		{
			return;
		}

		if (msgs->isEmpty())
		{
            if (Sequences_.contains(seq))
			    LastRequested_[aimId] = -1;
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
                LastRequested_[aimId] = -1;
			return;
		}

		if ((!Sequences_.contains(seq) && msgs->last()->Id_ < modelFirst) || !Requested_.contains(aimId))
		{
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
			msgs->swap(tmpMsgs);
			qint64 idFirst = msgs->first()->Id_;

			if (!isDlgState)
			{
				QList<MessageKey> deletedValues;
				std::set<InternalIndex>::iterator iter = Indexes_[aimId].begin();
				while(iter != Indexes_[aimId].end())
				{
					if (!iter->IsPending())
                    {
                        if (iter->Key_.Id_ < idFirst)
                        {
                            for (auto id : iter->GetMessageKeys())
                            {
                                Messages_[aimId].erase(internal(iter->Key_));
                            }
                            deletedValues << iter->Key_;
                            if (iter->IsDate())
                            {
                                Dates_[aimId].removeAll(iter->Date_);
                            }

                            if (iter->Key_.Id_ <= LastRequested_[aimId])
                                LastRequested_[aimId] = -1;

                            iter = Indexes_[aimId].erase(iter);
                        }
                        else if (iter->IsDate() && iter->Date_ == msgs->first()->GetDate())
                        {
                            deletedValues << iter->Key_;
                            iter = Indexes_[aimId].erase(iter);
                            Dates_[aimId].removeAll(iter->Date_);
                        }
                        else
                        {
                            ++iter;
                        }
					}
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
		auto lastDate = msgs->last()->GetDate();
		for (auto iter : TmpDates_[aimId])
		{
			if (lastDate < iter.Date_)
			{
				Dates_[aimId] << iter.Date_;
				insertDateMessage(iter.Key_, iter.AimId_, iter.Date_);
				indexes.insert(iter);

				std::set<MessageInternal>::iterator prevMsg = previousMsg(iter.AimId_, iter.Key_.Prev_);
				if (prevMsg != Messages_[aimId].end())
				{
					prevMsg->Buddy_->SetIndentBefore(false);
					prevMsg->Buddy_->SetHasAvatar(!prevMsg->Buddy_->IsOutgoing() || prevMsg->Buddy_->IsChatEvent());
					if (!updatedValues.contains(iter.Key_))
						updatedValues << iter.Key_;
				}
			}
		}
		TmpDates_[aimId].clear();

		for (auto msg : *msgs)
		{
            const auto key = msg->ToKey();
            if (std::find(Messages_[aimId].begin(), Messages_[aimId].end(), internal(key)) != Messages_[aimId].end())
                continue;

			auto prevMsg = previousMsg(aimId, msg->Prev_);
			const auto havePrevMsg = (prevMsg != Messages_[aimId].end());

			if (havePrevMsg)
			{
                const auto &prevMsgBuddy = prevMsg->Buddy_;

				if (msg->Chat_)
				{
					msg->SetIndentBefore(msg->ChatSender_ != prevMsgBuddy->ChatSender_  || prevMsgBuddy->IsChatEvent());
					msg->SetHasAvatar(msg->IsOutgoing() ? false : msg->GetIndentBefore());
				}
				else
				{
                    auto indentBefore = false;

                    indentBefore = (indentBefore ||
                        (msg->IsOutgoingVoip() != prevMsgBuddy->IsOutgoingVoip())
                    );

                    indentBefore = (indentBefore ||
                        prevMsgBuddy->IsChatEvent()
                    );

                    msg->SetIndentBefore(indentBefore);

					msg->SetHasAvatar(msg->IsOutgoing() ? false : msg->GetIndentBefore());
				}
			}
			else
			{
				msg->SetHasAvatar(!msg->IsOutgoing());
				msg->SetIndentBefore(true);
			}

			bool inserted = false;
			if (!msg->IsStandalone())
			{
				for (auto &index : indexes)
				{
					if (msg->GetTime() >= (index.MinTime_ - 120)
						&& msg->GetTime() <= (index.MaxTime_ + 120)
						&& index.Key_.isOutgoing() == msg->IsOutgoing()
						&& index.ChatSender_ == msg->ChatSender_
						&& !index.IsDate() && !index.IsStandalone()
						&& (index.contains(msg->Id_) || index.contains(msg->Prev_)))
					{
						index.InsertMessageKey(key);
                        index.MaxTime_ = std::max(msg->GetTime(), index.MaxTime_);
                        index.MinTime_ = std::min(msg->GetTime(), index.MinTime_);
						inserted = true;
						break;
					}
				}
			}

			if (!inserted)
			{
				const auto inDates = Dates_[aimId].contains(msg->GetDate());
				if (!inDates)
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

					if ((!havePrevMsg && msg->Prev_ == -1) || (havePrevMsg && msg->GetDate() != prevMsg->Buddy_->GetDate()))
					{
						Dates_[aimId] << msg->GetDate();
						insertDateMessage(newIndex.Key_, newIndex.AimId_, newIndex.Date_);
						indexes.insert(newIndex);
						msg->SetIndentBefore(false);
						msg->SetHasAvatar(!msg->IsOutgoing());
					}
					else
					{
						bool found = false;
						for (auto tmpdatesIter : TmpDates_[aimId])
						{
							if (msg->GetDate() == tmpdatesIter.Date_)
							{
								found = true;
								break;
							}
						}

						if (!found)
							TmpDates_[aimId] << newIndex;
					}
				}

				InternalIndex newIndex;
				newIndex.AimId_ = aimId;
				newIndex.MaxTime_ = msg->GetTime();
				newIndex.MinTime_ = msg->GetTime();
				newIndex.InsertMessageKey(key);
				newIndex.Key_ = key;
				newIndex.ChatSender_ = msg->ChatSender_;
				newIndex.ChatFriendly_ = msg->ChatFriendly_;
				newIndex.SetFileSharing(msg->GetFileSharing());
				newIndex.SetSticker(msg->GetSticker());
				newIndex.SetChatEvent(msg->GetChatEvent());
                newIndex.SetVoipEvent(msg->GetVoipEvent());
				indexes.insert(newIndex);
			}

			Messages_[aimId].emplace(MessageInternal(msg));
		}


        QList<MessageKey> updatedIndexes;
		for (auto iter : indexes)
		{
			bool inserted = false;
			if (!iter.IsDate() && !iter.IsStandalone())
			{
				for (auto ind = Indexes_[aimId].begin(); ind != Indexes_[aimId].end(); ++ind)
				{
					if (!PendingMessages_[aimId].empty() && ind->Key_.Id_ <= PendingMessages_[aimId].rbegin()->LastIndex_)
						continue;

					if (ind->Key_.isOutgoing() == iter.Key_.isOutgoing()
						&& ind->ChatSender_ == iter.ChatSender_
						&& !ind->IsDate() && !ind->IsStandalone())
					{
						if (((iter.contains(ind->Key_.Prev_) || iter.contains(ind->Key_.Id_)) && ind->MinTime_ - iter.MaxTime_ <= 120)
							|| ((ind->contains(iter.Key_.Prev_) || ind->contains(iter.Key_.Id_)) && iter.MinTime_ - ind->MaxTime_ <= 120))
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
				Indexes_[aimId].insert(iter);
				if (!updatedValues.contains(iter.Key_))
					updatedValues << iter.Key_;
			}
		}

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
		if (isFromServer)
        {
            const auto &index = Indexes_[aimId];

            if (index.size() >= preloadCount())
            {
			    setFirstMessage(
                    aimId,
                    index.begin()->Key_.Id_
                );
            }
        }
	}

	void MessagesModel::processPendingMessage(Data::MessageBuddies& msgs, const QString& aimId, const Ui::MessagesBuddiesOpt state)
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
				if (PendingMessages_[aimId].empty() && (!Messages_[aimId].empty() && !Messages_[aimId].rbegin()->Key_.isOutgoing()))
					msg->SetIndentBefore(true);

                bool inserted = false;
                for (auto &index : indexes)
                {
                    if (msg->GetTime() >= (index.MinTime_ - 120)
                        && msg->GetTime() <= (index.MaxTime_ + 120)
                        && index.Key_.isOutgoing() == msg->IsOutgoing()
                        && index.ChatSender_ == msg->ChatSender_
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
				    newIndex.ChatSender_ = msg->ChatSender_;
				    newIndex.ChatFriendly_ = msg->ChatFriendly_;
				    newIndex.SetFileSharing(msg->GetFileSharing());
				    newIndex.SetSticker(msg->GetSticker());
				    newIndex.SetChatEvent(msg->GetChatEvent());
                    newIndex.SetVoipEvent(msg->GetVoipEvent());
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
						std::set<InternalIndex>::iterator pending = PendingMessages_[aimId].begin();
						while (pending != PendingMessages_[aimId].end())
						{
							const auto &existingKeys = pending->GetMessageKeys();

							std::set<MessageKey>::iterator existKey = std::find(existingKeys.begin(), existingKeys.end(), key);
							if (existKey != existingKeys.end())
							{
								Messages_[aimId].erase(internal(*existKey));
								pending->RemoveMessageKey(*existKey);
                                if (existingKeys.empty())
                                {
                                    PendingMessages_[aimId].erase(pending);
                                }
                                else
                                {
                                    QList<MessageKey> updatedValues;
                                    updatedValues << pending->Key_;
                                    emitUpdated(updatedValues, aimId, PENDING);
                                }
								break;
							}
							++pending;
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
				result.Type_ = index.Key_.Type_;
				result.AimId_ = index.AimId_;
				result.SetOutgoing(msg->Buddy_->IsOutgoing());
				result.Chat_ = msg->Buddy_->Chat_;
				result.ChatSender_ = msg->Buddy_->ChatSender_;
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

	void MessagesModel::insertDateMessage(const MessageKey& key, const QString& aimId, const QDate& date)
	{
		MessageInternal message(std::make_shared<Data::MessageBuddy>());
		message.Buddy_->Id_ = key.Id_;
		message.Buddy_->Prev_ = key.Prev_;
		message.Buddy_->AimId_ = aimId;
		message.Buddy_->SetTime(0);
		message.Buddy_->SetDate(date);
		message.Key_ = key;
        message.Key_.control_type_ = control_type::ct_date;
		Messages_[aimId].emplace(message);
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

				emit deliveredToClient(msg->InternalId_);
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
        if (list.size() <= MORE_COUNT)
        {
            emit updated(list, aimId, mode);
            return;
        }

        int i = 0;
        QList<Logic::MessageKey> updatedList;
        for (auto iter : list)
        {
            if (++i < MORE_COUNT)
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
                    previewsEnabled
                )
            );

            connect(
                item.get(),
                &HistoryControl::MessageContentWidget::removeMe,
                [messageBuddy]
            {
                QList<MessageKey> keys;
                keys << messageBuddy.ToKey();
                emit GetMessagesModel()->deleted(keys, messageBuddy.AimId_);
            }
            );
        }

		messageItem.setContentWidget(item.release(), true);
	}

    void MessagesModel::createImagePreviewWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const
    {
        const auto uri = messageBuddy.GetFirstUriFromText();
        const auto &fullText = messageBuddy.GetText();
        const auto previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);

        auto item = new HistoryControl::ImagePreviewWidget(parent, messageBuddy.IsOutgoing(), uri.toString(), fullText, previewsEnabled, messageItem.getContact());
        item->setMaximumWidth(ItemWidth_);

        messageItem.setContentWidget(item, true);
    }

	void MessagesModel::createStickerWidget(Ui::MessageItem &messageItem, QWidget* parent, const Data::MessageBuddy& messageBuddy) const
	{
		auto item = new HistoryControl::StickerWidget(parent, messageBuddy.GetSticker(), messageBuddy.IsOutgoing(), messageItem.getContact());
        item->setFixedWidth(ItemWidth_);

		messageItem.setContentWidget(item, false);
		messageItem.setStickerText(messageBuddy.GetText());
	}

	void MessagesModel::requestMessages(const QString& aimId)
	{
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
            return item.release();
        }

		std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(parent));
        messageItem->setContact(msg.AimId_);
        messageItem->setId(msg.Id_, msg.AimId_);
		messageItem->setNotificationKeys(msg.GetNotificationKeys());
        const auto senderNick = GetChatFriendly(msg.ChatSender_, msg.ChatFriendly_);
		if (msg.HasAvatar())
		{
            const auto sender =
                (msg.Chat_ && !msg.ChatSender_.isEmpty()) ?
                    NormalizeAimId(msg.ChatSender_) :
                    msg.AimId_;

			messageItem->loadAvatar(
                sender,
                senderNick,
				Utils::scale_bitmap(Utils::scale_value(32))
			);
		}
		else
		{
			messageItem->setAvatarVisible(false);
		}

		messageItem->setTopMargin(msg.GetIndentBefore());
		messageItem->setOutgoing(msg.IsOutgoing(), msg.IsPending(), msg.IsDeliveredToServer(), msg.IsDeliveredToClient(), msg.Chat_);
        messageItem->setMchatSenderAimId(msg.ChatSender_.length() ? msg.ChatSender_ : msg.AimId_);
		messageItem->setMchatSender(senderNick);
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

			if (first)
			{
				buddy->Type_ = index.Key_.Type_;
				buddy->AimId_ = index.AimId_;
				buddy->SetOutgoing(msg->Buddy_->IsOutgoing());
				buddy->Chat_ = msg->Buddy_->Chat_;
				buddy->ChatSender_ = msg->Buddy_->ChatSender_;
				buddy->ChatFriendly_ = msg->Buddy_->ChatFriendly_;
				buddy->SetHasAvatar(msg->Buddy_->HasAvatar());
				buddy->SetIndentBefore(msg->Buddy_->GetIndentBefore());
			}

			buddy->FillFrom(*msg->Buddy_, !first);

			first = false;

			buddy->SetTime(index.MaxTime_);

			if (id.Id_ == newId)
			{
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
                    result->layout()->addWidget(item.release());
                }
                else
                {
				    std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(result.get()));
				    messageItem->setId(buddy->Id_, buddy->AimId_);
				    messageItem->setNotificationKeys(buddy->GetNotificationKeys());

				    if (buddy->HasAvatar())
                    {
					    messageItem->loadAvatar(
                            buddy->Chat_ ?
                                NormalizeAimId(buddy->ChatSender_) :
                                buddy->AimId_, QString(),
                            Utils::scale_bitmap(Utils::scale_value(32))
                        );
                    }
				    else
                    {
					    messageItem->setAvatarVisible(false);
                    }

				    messageItem->setTopMargin(buddy->GetIndentBefore());
				    messageItem->setOutgoing(buddy->IsOutgoing(), buddy->IsPending(), buddy->IsDeliveredToServer(), buddy->IsDeliveredToClient(), buddy->Chat_);
                    messageItem->setContact(buddy->AimId_);
				    messageItem->setMchatSender(GetChatFriendly(buddy->ChatSender_, buddy->ChatFriendly_));
				    messageItem->setTime(buddy->GetTime());
				    messageItem->setDate(buddy->GetDate());

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
		}

		if (buddy->HasText() && buddy->IsBase())
		{
			std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(result.get()));
			messageItem->setId(buddy->Id_, buddy->AimId_);
			messageItem->setNotificationKeys(buddy->GetNotificationKeys());
			messageItem->loadAvatar(
                buddy->Chat_ ?
                    NormalizeAimId(buddy->ChatSender_) :
                    buddy->AimId_,
                QString(),
                Utils::scale_bitmap(Utils::scale_value(32))
            );
			messageItem->setTopMargin(false);
			messageItem->setOutgoing(buddy->IsOutgoing(), buddy->IsPending(), buddy->IsDeliveredToServer(), buddy->IsDeliveredToClient(), buddy->Chat_);
			messageItem->setMchatSender(GetChatFriendly(buddy->ChatSender_, buddy->ChatFriendly_));
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

		if (current != indexes.crend())
		{
			if (newId != -1)
			{
				const auto releaseVoodoo = ((current != indexes.rbegin() && current->contains(newId)) || current->containsNotLast(newId));
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

		return nullptr;
	}

	void MessagesModel::setFirstMessage(const QString& aimId, qint64 msgId)
	{
		Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", aimId);
		collection.set_value_as_int64("message", msgId);
		Ui::GetDispatcher()->post_message_to_core("dialogs/set_first_message", collection.get());
	}

	std::set<MessageInternal>::iterator MessagesModel::previousMsg(const QString& aimId, qint64 id)
	{
		std::set<MessageInternal>::iterator iter = Messages_[aimId].begin();
		for (; iter != Messages_[aimId].end(); ++iter)
		{
			if (iter->Key_.Id_ == id && !iter->Key_.isDate())
				break;
		}

		return iter;
	}

	void MessagesModel::contactChanged(QString contact)
	{
		if (!LastRequested_.contains(contact))
		{
			requestMessages(contact);
		}
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
			key = iter->Key_;
			Data::MessageBuddy buddy = item(*iter);
			result.insert(key, fill(buddy, parent));
			if (++i == MORE_COUNT)
				break;

			++iter;
		}

		if (i < MORE_COUNT)
		{
			bool haveImcoming = false;
			iter = Indexes_[aimId].rbegin();
			while (iter != Indexes_[aimId].rend())
			{
				haveImcoming |= !iter->Key_.isOutgoing();
				key = iter->Key_;
				if (newId != -1 &&
					haveImcoming &&
					!iter->IsPending() &&
					(iter->IsBase() || iter->IsFileSharing() || iter->IsSticker() || iter->IsChatEvent() || iter->IsVoipEvent()) &&
					((iter != Indexes_[aimId].rbegin() && iter->contains(newId)) || iter->containsNotLast(newId)))
				{
					result.insert(key, fillNew(*iter, parent, newId));
				}
				else
				{
					Data::MessageBuddy buddy = item(*iter);
					result.insert(key, fill(buddy, parent));
				}
				if (++i == MORE_COUNT)
					break;
				++iter;
			}
		}

		LastKey_[aimId] = key;

		if (LastKey_[aimId].isEmpty() || LastKey_[aimId].isPending() || (LastKey_[aimId].Prev_ != -1 && Indexes_[aimId].begin()->contains(LastKey_[aimId].Id_)))
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
		int i = 0;
		MessageKey key = LastKey_[aimId];

		std::set<InternalIndex>::const_reverse_iterator iter = PendingMessages_[aimId].rbegin();
		while (iter != PendingMessages_[aimId].rend())
		{
			if (!(iter->Key_ < key))
			{
				++iter;
				continue;
			}

			haveIncoming |= !iter->Key_.isOutgoing();
			key = iter->Key_;
			Data::MessageBuddy buddy = item(*iter);
			result.insert(key, fill(buddy, parent));

			if (++i == MORE_COUNT)
				break;
			++iter;
		}

		if (i < MORE_COUNT)
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
					((iter != Indexes_[aimId].rbegin() && iter->contains(newId)) || iter->containsNotLast(newId)))
				{
					result.insert(key, fillNew(*iter, parent, newId));
				}
				else
				{
                    Data::MessageBuddy buddy = item(*iter);
					result.insert(key, fill(buddy, parent));
				}
				if (++i == MORE_COUNT)
					break;
				++iter;
			}
		}

		LastKey_[aimId] = key;

		if (LastKey_[aimId].isEmpty() || LastKey_[aimId].isPending() || (LastKey_[aimId].Prev_ != -1 &&
			Indexes_[aimId].begin()->contains(LastKey_[aimId].Id_)))
			requestMessages(aimId);

        if (!result.isEmpty())
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_preload);
        else
            Subscribed_ << aimId;

		return result;
	}

	QWidget* MessagesModel::getById(const QString& aimId, const MessageKey& key, QWidget* parent, qint64 newId)
	{
		return fillItemById(aimId, key, parent, newId);
	}

	unsigned MessagesModel::preloadCount() const
	{
		return PRELOAD_COUNT;
	}

    unsigned MessagesModel::moreCount() const
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

		auto iter = Indexes_[aimId].begin();
		while(iter != Indexes_[aimId].end())
		{
			if (iter->Key_ < key)
			{
				for (auto id : iter->GetMessageKeys())
				{
					Messages_[aimId].erase(internal(id));
				}

				if (iter->IsDate())
				{
					Dates_[aimId].removeAll(iter->Date_);
				}

                if (iter->Key_.Id_ <= LastRequested_[aimId])
                    LastRequested_[aimId] = -1;

				iter = Indexes_[aimId].erase(iter);
			}
			else
			{
				break;
			}
		}

        const auto &index = Indexes_[aimId];
        if (index.size() > preloadCount())
        {
		    setFirstMessage(aimId, index.begin()->Key_.Id_);
        }
	}

	void MessagesModel::removeDialog(const QString& aimId)
	{
		Messages_.remove(aimId);
		Indexes_.remove(aimId);
		LastKey_.remove(aimId);
		Dates_.remove(aimId);
		LastRequested_.remove(aimId);
		Requested_.removeAll(aimId);
	}

	void MessagesModel::updateNew(const QString& aimId, qint64 newId, bool hide)
	{
		std::set<InternalIndex>::const_reverse_iterator iter = Indexes_[aimId].rbegin();
		while (iter != Indexes_[aimId].rend())
		{
			if (iter->contains(newId))
			{
				QList<MessageKey> updatedValues;
				updatedValues << iter->Key_;

				bool outgoing = iter->Key_.isOutgoing();
				QString chatSender = iter->ChatSender_;
                bool chatEvent = iter->Key_.isChatEvent();
				if (iter != Indexes_[aimId].rbegin())
				{
					--iter;
					const auto &messages = Messages_[aimId];
					auto msg = messages.find(internal(iter->Key_));
					if (msg != messages.end() && !msg->Buddy_->IsOutgoing())
					{
						if (hide)
							msg->Buddy_->SetHasAvatar(msg->Buddy_->Chat_ ? msg->Buddy_->ChatSender_ != chatSender  : outgoing);
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
            messageItem->setContentWidget(item, false);
        }
        else if (buddy.ContainsPreviewableLink())
        {
            return photoStr;
        }
        else if (buddy.IsSticker())
        {
            auto item = new HistoryControl::StickerWidget(buddy.AimId_);
            messageItem->setContentWidget(item, false);
        }
        else
        {
            messageItem->setMessage(buddy.GetText());
        }

		return messageItem->formatRecentsText();
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