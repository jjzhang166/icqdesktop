#include "stdafx.h"

#include "message.h"

#include "../utils/gui_coll_helper.h"
#include "../../corelib/enumerations.h"
#include "../utils/log/log.h"

#include "../main_window/history_control/FileSharingInfo.h"
#include "../main_window/history_control/StickerInfo.h"
#include "../main_window/history_control/ChatEventInfo.h"
#include "../main_window/history_control/VoipEventInfo.h"
#include "../main_window/history_control/MessagesModel.h"

#include "../cache/avatars/AvatarStorage.h"

namespace
{
    bool isSupportedImagePreviewExts(const QStringRef &ext);

	void unserializeMessages(
		core::iarray* msgArray,
		const QString &aimId,
		const QString &myAimid,
		const qint64 theirs_last_delivered,
		const qint64 theirs_last_read,
		Out Data::MessageBuddies &messages);

	Data::MessageBuddySptr unserializeMessage(
		core::coll_helper& msg,
		const QString &aimId,
		const QString &myAimid,
		const qint64 theirs_last_delivered,
		const qint64 theirs_last_read);

    bool containsImagePreviewUri(const QString &text, Out QStringRef &uri);

    bool containsPttAudio(const QString& text, Out int& duration);

    bool containsImage(const QString& text);

    int decodeSymbols(const QString& str)
    {
        int result = 0;

        for (QChar ch : str)
        {
            char c = ch.toLatin1();
            int value = 0;
            if (c >= '0' && c <= '9')
                value = c - 48;
            else if(c >= 'a' && c <= 'z')
                value = c - 87;
            else
                value = c - 29;

            result += value;
        }

        return result;
    }
}

namespace Data
{
	MessageBuddy::MessageBuddy()
		: Id_(-1)
		, LastId_(-1)
		, Prev_(-1)
		, Time_(0)
        , PendingId_(-1)
		, Type_(core::message_type::base)
		, Outgoing_(false)
		, Chat_(false)
		, Unread_(false)
		, DeliveredToClient_(false)
		, HasAvatar_(false)
		, IndentBefore_(false)
		, Filled_(false)
        , Deleted_(false)
	{

	}

    void MessageBuddy::ApplyModification(const MessageBuddy &modification)
    {
        assert(modification.Id_ == Id_);

        EraseEventData();

        if (modification.IsBase())
        {
            SetText(modification.GetText());

            Type_ = core::message_type::base;

            return;
        }

        if (modification.IsChatEvent())
        {
            const auto &chatEventInfo = *modification.GetChatEvent();
            const auto &eventText = chatEventInfo.formatEventText();
            assert(!eventText.isEmpty());

            SetText(eventText);

            Type_ = core::message_type::base;

            return;
        }

        assert(!"unexpected modification type");
    }

    bool MessageBuddy::IsEmpty() const
    {
        return (Id_ == -1) && InternalId_.isEmpty();
    }

    bool MessageBuddy::CheckInvariant() const
	{
		if (Outgoing_)
		{
			if (!HasId() && InternalId_.isEmpty())
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

    bool MessageBuddy::ContainsPreviewableLink() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        QStringRef uri;
        return containsImagePreviewUri(Text_, Out uri);
    }

    bool MessageBuddy::ContainsPttAudio() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        auto pttDuration = 0;

        if (containsPttAudio(Text_, Out pttDuration))
        {
            return true;
        }

        if (!FileSharing_ || FileSharing_->IsOutgoing())
        {
            return false;
        }

        return containsPttAudio(FileSharing_->GetUri(), Out pttDuration);
    }

    bool MessageBuddy::ContainsImage() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        QString uri;
        if (FileSharing_.get())
            uri = FileSharing_->GetUri();

        return containsImage(Text_) || containsImage(uri);
    }

    bool MessageBuddy::GetIndentWith(const MessageBuddy &buddy)
    {
        if (buddy.IsServiceMessage())
        {
            return false;
        }

        return (
            (ChatSender_ != buddy.ChatSender_) ||
            (IsOutgoingVoip() != buddy.IsOutgoingVoip()) ||
            buddy.IsChatEvent()
        );
    }

	bool MessageBuddy::IsBase() const
	{
		assert(Type_ > core::message_type::min);
		assert(Type_ < core::message_type::max);

		return (Type_ == core::message_type::base);
	}

	bool MessageBuddy::IsChatEvent() const
	{
		assert(Type_ > core::message_type::min);
		assert(Type_ < core::message_type::max);

		return (Type_ == core::message_type::chat_event);
	}

    bool MessageBuddy::IsDeleted() const
    {
        return Deleted_;
    }

	bool MessageBuddy::IsDeliveredToClient() const
	{
		return DeliveredToClient_;
	}

	bool MessageBuddy::IsDeliveredToServer() const
	{
		return HasId();
	}

	bool MessageBuddy::IsFileSharing() const
	{
		assert(Type_ > core::message_type::min);
		assert(Type_ < core::message_type::max);

		return (Type_ == core::message_type::file_sharing);
	}

	bool MessageBuddy::IsOutgoing() const
	{
		return Outgoing_;
	}

    bool MessageBuddy::IsOutgoingVoip() const
    {
        if (!VoipEvent_)
        {
            return IsOutgoing();
        }

        return !VoipEvent_->isIncomingCall();
    }

	bool MessageBuddy::IsSticker() const
	{
		return (Type_ == core::message_type::sticker);
	}

	bool MessageBuddy::IsPending() const
	{
		return Id_ == -1 && !InternalId_.isEmpty();
	}

    bool MessageBuddy::IsServiceMessage() const
    {
        return (!IsBase() && !IsFileSharing() && !IsSticker() && !IsChatEvent() && !IsVoipEvent());
    }

	bool MessageBuddy::IsStandalone() const
	{
		return (IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || ContainsPreviewableLink() || IsDeleted());
	}

    bool MessageBuddy::IsVoipEvent() const
    {
        return (VoipEvent_ != nullptr);
    }

	const HistoryControl::ChatEventInfoSptr& MessageBuddy::GetChatEvent() const
	{
		return ChatEvent_;
	}

    const QString& MessageBuddy::GetChatSender() const
    {
        return ChatSender_;
    }

	const QDate& MessageBuddy::GetDate() const
	{
        assert(Date_.isValid());

		return Date_;
	}

	const HistoryControl::FileSharingInfoSptr& MessageBuddy::GetFileSharing() const
	{
		return FileSharing_;
	}

    QStringRef MessageBuddy::GetFirstUriFromText() const
    {
        QStringRef result;
        containsImagePreviewUri(GetText(), Out result);
        return result;
    }

    int MessageBuddy::GetPttDuration() const
    {
        int duration;
        containsPttAudio(GetText(), duration);
        return duration;
    }

    bool MessageBuddy::GetIndentBefore() const
    {
        return IndentBefore_;
    }

	const HistoryControl::StickerInfoSptr& MessageBuddy::GetSticker() const
	{
		return Sticker_;
	}

	const QString& MessageBuddy::GetText() const
	{
		return Text_;
	}

	const qint32 MessageBuddy::GetTime() const
	{
		return Time_;
	}

	qint64 MessageBuddy::GetLastId() const
	{
		assert(LastId_ >= -1);

		return LastId_;
	}

	const QStringList& MessageBuddy::GetNotificationKeys() const
	{
		return NotificationKeys_;
	}

    core::message_type MessageBuddy::GetType() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return Type_;
    }

    const HistoryControl::VoipEventInfoSptr& MessageBuddy::GetVoipEvent() const
    {
        return VoipEvent_;
    }

    bool MessageBuddy::HasAvatar() const
    {
        return HasAvatar_;
    }

    bool MessageBuddy::HasChatSender() const
    {
        return !ChatSender_.isEmpty();
    }

	bool MessageBuddy::HasId() const
	{
		return (Id_ != -1);
	}

	bool MessageBuddy::HasText() const
	{
		return !Text_.isEmpty();
	}

	void MessageBuddy::FillFrom(const MessageBuddy &buddy, const bool merge)
	{
		if (merge)
		{
            assert(!IsStandalone());
            assert(!buddy.IsStandalone());

			if (HasText())
			{
				Text_ += "\n";
			}

			Text_ += buddy.GetText();
		}
		else
		{
			Text_ = buddy.GetText();
		}

		SetLastId(buddy.Id_);
		Unread_ = buddy.Unread_;
		DeliveredToClient_ = buddy.DeliveredToClient_;
		Date_ = buddy.Date_;
        Deleted_ = buddy.Deleted_;

		SetFileSharing(buddy.GetFileSharing());
		SetSticker(buddy.GetSticker());
		SetChatEvent(buddy.GetChatEvent());
        SetVoipEvent(buddy.GetVoipEvent());

		NotificationKeys_.append(buddy.NotificationKeys_);
	}

    void MessageBuddy::EraseEventData()
    {
        FileSharing_.reset();
        Sticker_.reset();
        ChatEvent_.reset();
        VoipEvent_.reset();
    }

	void MessageBuddy::SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent)
	{
		assert(!chatEvent || (!Sticker_ && !FileSharing_ && !VoipEvent_));

		ChatEvent_ = chatEvent;
	}

    void MessageBuddy::SetChatSender(const QString& chatSender)
    {
        ChatSender_ = chatSender;
    }

	void MessageBuddy::SetDate(const QDate &date)
	{
        assert(date.isValid());

		Date_ = date;
	}

    void MessageBuddy::SetDeleted(const bool isDeleted)
    {
        Deleted_ = isDeleted;
    }

	void MessageBuddy::SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing)
	{
		assert(!fileSharing || (!Sticker_ && !ChatEvent_ && !VoipEvent_));

		FileSharing_ = fileSharing;
	}

    void MessageBuddy::SetHasAvatar(const bool hasAvatar)
    {
        HasAvatar_ = hasAvatar;
    }

    void MessageBuddy::SetIndentBefore(const bool indentBefore)
    {
        IndentBefore_ = indentBefore;
    }

	void MessageBuddy::SetLastId(const qint64 lastId)
	{
		assert(lastId >= -1);

		LastId_ = lastId;
	}

	void MessageBuddy::SetText(const QString &text)
	{
		Text_ = text;
	}

	void MessageBuddy::SetTime(const qint32 time)
	{
		Time_ = time;
	}

    void MessageBuddy::SetType(const core::message_type type)
    {
        assert(type > core::message_type::min);
        assert(type < core::message_type::max);

        Type_ = type;
    }

    void MessageBuddy::SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voip)
    {
        assert(!voip || (!Sticker_ && !ChatEvent_ && !FileSharing_));

        VoipEvent_ = voip;
    }

	Logic::MessageKey MessageBuddy::ToKey() const
	{
		return Logic::MessageKey(Id_, Prev_, InternalId_, PendingId_, Time_, Type_, IsOutgoing(), ContainsPreviewableLink(), IsDeleted(), Logic::control_type::ct_message);
	}

	void MessageBuddy::SetNotificationKeys(const QStringList &keys)
	{
		assert(NotificationKeys_.empty());
		NotificationKeys_ = keys;
	}

	void MessageBuddy::SetOutgoing(const bool isOutgoing)
	{
		Outgoing_ = isOutgoing;

		if (!HasId() && Outgoing_)
		{
			assert(!InternalId_.isEmpty());
		}
	}

	void MessageBuddy::SetSticker(const HistoryControl::StickerInfoSptr &sticker)
	{
		assert(!sticker || (!FileSharing_ && !ChatEvent_));

		Sticker_ = sticker;
	}

	const QString& DlgState::GetText() const
	{
		return Text_;
	}

    bool DlgState::HasLastMsgId() const
    {
        assert(LastMsgId_ >= -1);

        return (LastMsgId_ > 0);
    }

    bool DlgState::HasText() const
    {
        return !Text_.isEmpty();
    }

	void DlgState::SetText(const QString &text)
	{
		Text_ = text;
	}

	void UnserializeMessageBuddies(
        core::coll_helper* helper,
        const QString &myAimid,
        Out QString &aimId,
        Out bool &havePending,
        Out MessageBuddies& messages,
        Out MessageBuddies& modifications)
	{
		assert(!myAimid.isEmpty());

		Out havePending = false;

		aimId = helper->get_value_as_string("contact");
		if (helper->is_value_exist("result") && !helper->get_value_as_bool("result"))
        {
			return;
        }

		const auto theirs_last_delivered = helper->get_value_as_int64("theirs_last_delivered", -1);
		const auto theirs_last_read = helper->get_value_as_int64("theirs_last_read", -1);

		core::iarray* msgArray = helper->get_value_as_array("messages");
		unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);

		if (helper->is_value_exist("pending_messages"))
		{
			Out havePending = true;
			msgArray = helper->get_value_as_array("pending_messages");
			unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);
		}

        if (helper->is_value_exist("modified"))
        {
            auto modificationsArray = helper->get_value_as_array("modified");
            unserializeMessages(modificationsArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out modifications);
        }
	}

	void SerializeDlgState(core::coll_helper* helper, const DlgState& state)
	{
		helper->set_value_as_string("contact", state.AimId_.toStdString());
		helper->set_value_as_int64("unreads", state.UnreadCount_);
		helper->set_value_as_int64("last_msg_id", state.LastMsgId_);
		helper->set_value_as_int64("yours_last_read", state.YoursLastRead_);
		helper->set_value_as_int64("theirs_last_read", state.TheirsLastRead_);
		helper->set_value_as_int64("theirs_last_delivered", state.TheirsLastDelivered_);
		helper->set_value_as_string("last_message_friendly", state.LastMessageFriendly_.toStdString());
	}

	void UnserializeDlgState(core::coll_helper* helper, const QString &myAimId, Out DlgState& state)
	{
        state.AimId_ = helper->get<QString>("contact");
		state.UnreadCount_ = helper->get<int64_t>("unreads");
		state.LastMsgId_ = helper->get<int64_t>("last_msg_id");
		state.YoursLastRead_ = helper->get<int64_t>("yours_last_read");
		state.TheirsLastRead_ = helper->get<int64_t>("theirs_last_read");
		state.TheirsLastDelivered_ = helper->get<int64_t>("theirs_last_delivered");
		state.Visible_ = helper->get<bool>("visible");
		state.LastMessageFriendly_ = helper->get<QString>("last_message_friendly");
        state.Friendly_ = helper->get<QString>("friendly");
        state.Chat_ = helper->get<bool>("is_chat");
        state.Official_ = helper->get<bool>("official");
        if (helper->is_value_exist("favorite_time"))
        {
            state.FavoriteTime_ = helper->get<int64_t>("favorite_time");
        }

        if (helper->is_value_exist("message"))
        {
		    core::coll_helper value(helper->get_value_as_collection("message"), false);

		    const auto messageBuddy = unserializeMessage(value, state.AimId_, myAimId, state.TheirsLastDelivered_, state.TheirsLastRead_);

            const auto serializeMessage = helper->get<bool>("serialize_message");
            if (serializeMessage)
            {
		        auto text = Logic::GetMessagesModel()->formatRecentsText(*messageBuddy);
	            state.SetText(std::move(text));
            }

            state.senderNick_ = messageBuddy->ChatFriendly_;
		    state.Time_ = value.get<int32_t>("time");
		    state.Outgoing_ = value.get<bool>("outgoing");

            if (messageBuddy->GetChatEvent() && messageBuddy->GetChatEvent()->eventType() == core::chat_event_type::avatar_modified)
            {
                static int32_t previousUpdateTime = 0;
                if ((state.Time_ - previousUpdateTime) > 1)
                {
                    previousUpdateTime = state.Time_;
                    Logic::GetAvatarStorage()->UpdateAvatar(state.AimId_);
                }
            }
        }
	}
}

namespace
{
    bool isSupportedImagePreviewExts(const QStringRef &ext)
    {
        const QString extensions[] = { "bmp", "gif", "jpg", "jpeg", "png", "tif", "tiff" };

        for (const auto &knownExt : extensions)
        {
            if (ext == knownExt)
            {
                return true;
            }
        }

        return false;
    }

	void unserializeMessages(
		core::iarray* msgArray,
		const QString &aimId,
		const QString &myAimid,
		const qint64 theirs_last_delivered,
		const qint64 theirs_last_read,
		Out Data::MessageBuddies &messages)
	{
		assert(!aimId.isEmpty());
		assert(!myAimid.isEmpty());
		assert(msgArray);

		__TRACE(
			"delivery",
			"unserializing messages collection\n" <<
			"	size=<" << msgArray->size() << ">\n" <<
			"	last_delivered=<" << theirs_last_delivered << ">");

		for (auto i = 0; i < msgArray->size(); ++i)
		{
			core::coll_helper value(
				msgArray->get_at(i)->get_as_collection(),
				false
			);

			auto message = unserializeMessage(
				value, aimId, myAimid, theirs_last_delivered, theirs_last_read
			);

            const auto isInvisibleVoipEvent = (message->IsVoipEvent() && !message->GetVoipEvent()->isVisible());
            const auto skipMessage = isInvisibleVoipEvent;
            if (!skipMessage)
            {
                messages << std::move(message);
            }
		}
	}

	Data::MessageBuddySptr unserializeMessage(
		core::coll_helper &msgColl,
		const QString &aimId,
		const QString &myAimid,
		const qint64 theirs_last_delivered,
		const qint64 theirs_last_read)
	{
		auto message = std::make_shared<Data::MessageBuddy>();

		message->Id_ = msgColl.get_value_as_int64("id");
		message->InternalId_ = msgColl.get_value_as_string("internal_id");
		message->Prev_ = msgColl.get_value_as_int64("prev_id");
		message->AimId_ = aimId;
		message->SetOutgoing(msgColl.get<bool>("outgoing"));
        message->SetDeleted(msgColl.get<bool>("deleted"));

		if (message->IsOutgoing())
		{
			if ((message->Id_ != -1))
			{
				message->Unread_ = (message->Id_ > theirs_last_read);

				if (message->Id_ <= theirs_last_delivered)
				{

				}
				else if (!message->InternalId_.isEmpty())
				{
					message->SetNotificationKeys(QStringList(message->InternalId_));
				}
			}
			else if (!message->InternalId_.isEmpty())
			{
				message->SetNotificationKeys(QStringList(message->InternalId_));
			}
		}

		if (message->Id_ == -1 && !message->InternalId_.isEmpty())
		{
            int pendingPos = message->InternalId_.lastIndexOf("-");
            QString pendingId = message->InternalId_.right(message->InternalId_.length() - pendingPos - 1);
			message->PendingId_ = QVariant(pendingId).toInt();
		}

        const auto timestamp = msgColl.get<int32_t>("time");

		message->SetTime(timestamp);
        if (msgColl->is_value_exist("text"))
		    message->SetText(msgColl.get_value_as_string("text"));
		message->SetDate(QDateTime::fromTime_t(message->GetTime()).date());

		__TRACE(
			"delivery",
			"unserialized message\n" <<
			"	id=					<" << message->Id_ << ">\n" <<
			"	last_delivered=		<"<< theirs_last_delivered << ">\n" <<
			"	outgoing=<" << logutils::yn(message->IsOutgoing()) << ">\n" <<
			"	notification_key=<" << message->InternalId_ << ">\n" <<
			"	delivered_to_client=<" << logutils::yn(message->IsDeliveredToClient()) << ">\n" <<
			"	delivered_to_server=<" << logutils::yn(message->Id_ != -1) << ">");

		if (msgColl.is_value_exist("chat"))
		{
			core::coll_helper chat(msgColl.get_value_as_collection("chat"), false);
			if (!chat->empty())
			{
				message->Chat_ = true;
				message->SetChatSender(chat.get_value_as_string("sender"));
				message->ChatFriendly_ = chat.get_value_as_string("friendly");
			}
		}

		if (msgColl.is_value_exist("file_sharing"))
		{
			core::coll_helper file_sharing(msgColl.get_value_as_collection("file_sharing"), false);

			message->SetType(core::message_type::file_sharing);
			message->SetFileSharing(std::make_shared<HistoryControl::FileSharingInfo>(file_sharing));
		}

		if (msgColl.is_value_exist("sticker"))
		{
			core::coll_helper sticker(msgColl.get_value_as_collection("sticker"), false);

			message->SetType(core::message_type::sticker);
			message->SetSticker(HistoryControl::StickerInfo::Make(sticker));
		}

        if (msgColl.is_value_exist("voip"))
        {
            core::coll_helper voip(msgColl.get_value_as_collection("voip"), false);

            message->SetType(core::message_type::voip_event);
            message->SetVoipEvent(
                HistoryControl::VoipEventInfo::Make(voip, timestamp)
            );
        }

		if (msgColl.is_value_exist("chat_event"))
		{
            assert(!message->IsChatEvent());

			core::coll_helper chat_event(msgColl.get_value_as_collection("chat_event"), false);

			message->SetType(core::message_type::chat_event);

			message->SetChatEvent(
				HistoryControl::ChatEventInfo::Make(
					chat_event,
					message->IsOutgoing(),
					myAimid
			    )
            );
		}

		return message;
	}

    bool containsImagePreviewUri(const QString &text, Out QStringRef &uri)
    {
        assert(uri == QStringRef());

        Out uri = QStringRef();

        static const QRegExp space("\\s|\\n|\\r");

        const auto parts = text.splitRef(space, QString::SkipEmptyParts);

        for (const auto &part : parts)
        {
            const auto isUri = (
                part.startsWith("http://") ||
                part.startsWith("www.") ||
                part.startsWith("https://")
            );
            if (!isUri)
            {
                continue;
            }

            const auto lastDotIndex = part.lastIndexOf(".");
            if (lastDotIndex < 0)
            {
                continue;
            }

            const auto ext = part.mid(lastDotIndex + 1);
            if (!isSupportedImagePreviewExts(ext))
            {
                continue;
            }

            Out uri = part;
            return true;
        }

        return false;
    }

    bool containsPttAudio(const QString& text, Out int& duration)
    {
        Out duration = 0;

        const auto parts = text.splitRef(QChar(' '), QString::SkipEmptyParts);
        for (const auto &part : parts)
        {
            if (!part.startsWith("www.") &&
                !part.startsWith("http://") &&
                !part.startsWith("https://"))
            {
                continue;
            }

            auto index = part.lastIndexOf('/');
            if (index < 0)
            {
                continue;
            }

            ++index;
            if (part.at(index) != 'I' && part.at(index) != 'J')
            {
                continue;
            }

            ++index;
            QString durationStr = part.mid(index, 4).toString();

            duration = decodeSymbols(durationStr);

            return true;
        }

        return false;
    }

    bool containsImage(const QString& text)
    {
        const auto parts = text.splitRef(QChar(' '), QString::SkipEmptyParts);
        for (const auto &part : parts)
        {
            if (!part.startsWith("www.") &&
                !part.startsWith("http://") &&
                !part.startsWith("https://"))
            {
                continue;
            }

            auto index = part.lastIndexOf('/');
            if (index < 0)
            {
                continue;
            }

            ++index;
            return part.at(index) >= '0' && part.at(index) <= '7';
        }

        return false;
    }
}