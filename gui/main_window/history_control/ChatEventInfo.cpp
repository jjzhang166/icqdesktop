#include "stdafx.h"

#include "../../../corelib/collection_helper.h"
#include "../../collection_helper_ext.h"
#include "../../../corelib/enumerations.h"

#include "../../my_info.h"

#include "../../cache/emoji/Emoji.h"

#include "ChatEventInfo.h"

using namespace core;

namespace
{
    void cleanupFriendlyName(QString &name);
}

namespace HistoryControl
{

	ChatEventInfoSptr ChatEventInfo::Make(const core::coll_helper &info, const bool isOutgoing, const QString &myAimid)
	{
		assert(!myAimid.isEmpty());

		const auto type = info.get_value_as_enum<chat_event_type>("type");

		ChatEventInfoSptr eventInfo(new ChatEventInfo(
			type, isOutgoing, myAimid
		));

        const auto isGeneric = (type == core::chat_event_type::generic);
        if (isGeneric)
        {
            assert(!info.is_value_exist("sender_friendly"));

            eventInfo->setGenericText(
                info.get<QString>("generic")
            );

            return eventInfo;
        }

		const auto isBuddyReg = (type == core::chat_event_type::buddy_reg);
		const auto isBuddyFound = (type == core::chat_event_type::buddy_found);
		const auto isBirthday = (type == core::chat_event_type::birthday);
		if (isBuddyReg || isBuddyFound || isBirthday)
		{
			assert(!info.is_value_exist("sender_friendly"));
			return eventInfo;
		}

		eventInfo->setSenderFriendly	(
			info.get<QString>("sender_friendly")
		);

		const auto isAddedToBuddyList = (type == core::chat_event_type::added_to_buddy_list);
        const auto isAvatarModified = (type == core::chat_event_type::avatar_modified);
		if (isAddedToBuddyList || isAvatarModified)
		{
			return eventInfo;
		}

		const auto isChatNameModified = (type == core::chat_event_type::chat_name_modified);
		if (isChatNameModified)
		{
			const auto newChatName = info.get<QString>("chat/new_name");
			assert(!newChatName.isEmpty());

			eventInfo->setNewName(newChatName);

			return eventInfo;
		}

        const auto isChatDescriptionModified = (type == core::chat_event_type::chat_description_modified);
        if (isChatDescriptionModified)
        {
            const auto newDescription = info.get<QString>("chat/new_description");

            eventInfo->setNewDescription(newDescription);

            return eventInfo;
        }

		const auto isMchatAddMembers = (type == core::chat_event_type::mchat_add_members);
		const auto isMchatInvite = (type == core::chat_event_type::mchat_invite);
		const auto isMchatLeave = (type == core::chat_event_type::mchat_leave);
		const auto isMchatDelMembers = (type == core::chat_event_type::mchat_del_members);
		const auto isMchatKicked = (type == core::chat_event_type::mchat_kicked);
		const auto hasMchatMembers = (isMchatAddMembers || isMchatInvite || isMchatLeave || isMchatDelMembers || isMchatKicked);
		if (hasMchatMembers)
		{
			const auto membersArray = info.get_value_as_array("mchat/members");
			assert(membersArray);

			eventInfo->setMchatMembers(*membersArray);

			return eventInfo;
		}

		assert(!"unexpected event type");
		return eventInfo;
	}

	ChatEventInfo::ChatEventInfo(const chat_event_type type, const bool isOutgoing, const QString &myAimid)
		: Type_(type)
		, IsOutgoing_(isOutgoing)
		, MyAimid_(myAimid)
	{
		assert(Type_ > chat_event_type::min);
		assert(Type_ < chat_event_type::max);
		assert(!MyAimid_.isEmpty());
	}

	QString ChatEventInfo::formatEventTextInternal() const
	{
		switch (Type_)
		{
			case chat_event_type::added_to_buddy_list:
				return formatAddedToBuddyListText();

            case chat_event_type::avatar_modified:
                return formatAvatarModifiedText();

			case chat_event_type::birthday:
				return formatBirthdayText();

			case chat_event_type::buddy_reg:
				return formatBuddyReg();

			case chat_event_type::buddy_found:
				return formatBuddyFound();

			case chat_event_type::chat_name_modified:
				return formatChatNameModifiedText();

            case chat_event_type::generic:
                return formatGenericText();

			case chat_event_type::mchat_add_members:
				return formatMchatAddMembersText();

			case chat_event_type::mchat_invite:
				return formatMchatInviteText();

			case chat_event_type::mchat_leave:
				return formatMchatLeaveText();

			case chat_event_type::mchat_del_members:
				return formatMchatDelMembersText();

            case chat_event_type::chat_description_modified:
                return formatChatDescriptionModified();

			case chat_event_type::mchat_kicked:
				return formatMchatKickedText();

            default:
                break;
		}

		assert(!"unexpected chat event type");
		return QString();
	}

	QString ChatEventInfo::formatAddedToBuddyListText() const
	{
		assert(Type_ == chat_event_type::added_to_buddy_list);
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);

		if (IsOutgoing_)
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You added ");
			result += SenderFriendly_;
			result += QT_TRANSLATE_NOOP("chat_event", " to contacts");
		}
		else
		{
			result += SenderFriendly_;
			result += QT_TRANSLATE_NOOP("chat_event", " added you to contacts");
		}

		return result;
	}

    QString ChatEventInfo::formatAvatarModifiedText() const
    {
        assert(Type_ == chat_event_type::avatar_modified);

        QString result;
        result.reserve(512);

        if (IsOutgoing_)
        {
            result += QT_TRANSLATE_NOOP("chat_event", "You changed picture of chat");
        }
        else
        {
            result += SenderFriendly_;
            result += QT_TRANSLATE_NOOP("chat_event", " changed picture of chat");
        }

        return result;
    }

	QString ChatEventInfo::formatBirthdayText() const
	{
		assert(Type_ == chat_event_type::birthday);

		return QT_TRANSLATE_NOOP("chat_event", "Has a birthday today!");
	}

	QString ChatEventInfo::formatBuddyFound() const
	{
		assert(Type_ == chat_event_type::buddy_found);

		return QT_TRANSLATE_NOOP("chat_event", "Your friend is now available for chat and calls. You can say hi now!");
	}

	QString ChatEventInfo::formatBuddyReg() const
	{
		assert(Type_ == chat_event_type::buddy_reg);

		return QT_TRANSLATE_NOOP("chat_event", "Your friend is now available for chat and calls. You can say hi now!");
	}

	QString ChatEventInfo::formatChatNameModifiedText() const
	{
		assert(Type_ == chat_event_type::chat_name_modified);
		assert(!SenderFriendly_.isEmpty());
		assert(!Chat_.NewName_.isEmpty());

		QString result;
		result.reserve(512);

		if (IsOutgoing_)
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You changed theme to ");
			result +=  "\"";
			result += Chat_.NewName_;
			result +=  "\"";
		}
		else
		{
			result += SenderFriendly_;
			result += QT_TRANSLATE_NOOP("chat_event", " changed theme to ");
			result +=  "\"";
			result += Chat_.NewName_;
			result +=  "\"";
		}

		return result;
	}

    QString ChatEventInfo::formatGenericText() const
    {
        assert(Type_ == chat_event_type::generic);
        assert(!Generic_.isEmpty());

        return Generic_;
    }

	QString ChatEventInfo::formatMchatAddMembersText() const
	{
		assert(Type_ == chat_event_type::mchat_add_members);
		assert(!Mchat_.MembersFriendly_.isEmpty());
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);

		if (IsOutgoing_)
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You invited ");
			result += formatMchatMembersList(false);

			return result;
		}
		const auto joinedSomeone = (SenderFriendly_ == Mchat_.MembersFriendly_.first());
		if (joinedSomeone)
		{
			result += SenderFriendly_;
			result += QT_TRANSLATE_NOOP("chat_event", " joined the chat");

			return result;
		}
		result += SenderFriendly_;
        result += QT_TRANSLATE_NOOP("chat_event", " invited ");
        result += formatMchatMembersList(false);

		return result;
	}

    QString ChatEventInfo::formatChatDescriptionModified() const
    {
        assert(Type_ == chat_event_type::chat_description_modified);

        QString result;
        result.reserve(512);

        if (IsOutgoing_)
        {
            result += QT_TRANSLATE_NOOP("chat_event", "You changed description to \"");
            result += Chat_.NewDescription_;
        }
        else
        {
            result += SenderFriendly_;
            result += QT_TRANSLATE_NOOP("chat_event", " changed description to \"");
            result += Chat_.NewDescription_;
        }

        result += "\"";

        return result;
    }

	QString ChatEventInfo::formatMchatInviteText() const
	{
		assert(Type_ == chat_event_type::mchat_invite);
		assert(!Mchat_.MembersFriendly_.isEmpty());
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);
		const auto joinedMyselfOnly = isMyAimid(SenderFriendly_);
		if ((IsOutgoing_) && (joinedMyselfOnly))
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You joined the chat");
			return result;
		}
		result += SenderFriendly_;
		result += QT_TRANSLATE_NOOP("chat_event", " invited ");
		result += formatMchatMembersList(false);

		return result;
	}

	QString ChatEventInfo::formatMchatDelMembersText() const
	{
		assert(Type_ == chat_event_type::mchat_del_members);
		assert(!Mchat_.MembersFriendly_.isEmpty());
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);

		if (IsOutgoing_)
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You removed ");
			result += formatMchatMembersList(false);

			return result;
		}
		result += SenderFriendly_;
		result += QT_TRANSLATE_NOOP("chat_event", " removed ");
		result += formatMchatMembersList(false);

		return result;
	}

	QString ChatEventInfo::formatMchatKickedText() const
	{
		assert(Type_ == chat_event_type::mchat_kicked);
		assert(!Mchat_.MembersFriendly_.isEmpty());
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);

		if (IsOutgoing_)
		{
			result += QT_TRANSLATE_NOOP("chat_event", "You removed ");
			result += formatMchatMembersList(false);

			return result;
		}

		result += SenderFriendly_;
		result += QT_TRANSLATE_NOOP("chat_event", " removed ");
		result += formatMchatMembersList(false);

		return result;
	}

	QString ChatEventInfo::formatMchatLeaveText() const
	{
		assert(Type_ == chat_event_type::mchat_leave);
		assert(!Mchat_.MembersFriendly_.isEmpty());
		assert(!SenderFriendly_.isEmpty());

		QString result;
		result.reserve(512);

		result += formatMchatMembersList(true);

		if (hasMultipleMembers())
		{
			result += QT_TRANSLATE_NOOP3("chat_event", " left the chat", "many");
		}
		else
		{
			result += QT_TRANSLATE_NOOP3("chat_event", " left the chat", "one");
		}

		return result;
	}

	QString ChatEventInfo::formatMchatMembersList(const bool activeVoice) const
	{
		assert(!MyAimid_.isEmpty());

		QString result;
		result.reserve(512);

		const auto &friendlyMembers = Mchat_.MembersFriendly_;

		if (friendlyMembers.isEmpty())
		{
			return result;
		}

		const auto you =
            activeVoice ?
                QT_TRANSLATE_NOOP3("chat_event", "You", "active_voice") :
                QT_TRANSLATE_NOOP3("chat_event", "you", "passive_voice");

		const auto format =
			[this, you](const QString &name) -> const QString&
			{
				return (isMyAimid(name) ? you : name);
			};


		const auto &first = friendlyMembers.first();

		result += format(first);

		if (friendlyMembers.size() == 1)
		{
			return result;
		}

		const auto middle = friendlyMembers.mid(1, friendlyMembers.size() - 2);
		for (const auto &member : middle)
		{
			result += ", ";
			result += format(member);
		}

		result += QT_TRANSLATE_NOOP("chat_event", " and ");
		result += format(friendlyMembers.last());

		return result;
	}

	const QString& ChatEventInfo::formatEventText() const
	{
		if (FormattedEventText_.isEmpty())
		{
			FormattedEventText_ = formatEventTextInternal();
		}

		assert(!FormattedEventText_.isEmpty());
		return FormattedEventText_;
	}

	QImage ChatEventInfo::loadEventIcon(const int32_t sizePx) const
	{
		assert(sizePx > 0);

		if (Type_ != chat_event_type::birthday)
		{
			return QImage();
		}

		const auto birthdayEmojiId = 0x1f381;
		const auto emojiSize = Emoji::GetFirstLesserOrEqualSizeAvailable(sizePx);
		return Emoji::GetEmoji(birthdayEmojiId, 0, emojiSize);
	}

	bool ChatEventInfo::isMyAimid(const QString &aimId) const
	{
		assert(!aimId.isEmpty());
		assert(!MyAimid_.isEmpty());

		return (MyAimid_ == aimId);
	}

	bool ChatEventInfo::hasMultipleMembers() const
	{
		assert(!Mchat_.MembersFriendly_.isEmpty());

		return (Mchat_.MembersFriendly_.size() > 1);
	}

    void ChatEventInfo::setGenericText(const QString &text)
    {
        assert(Generic_.isEmpty());
        assert(!text.isEmpty());

        Generic_ = text;
    }

    void ChatEventInfo::setNewDescription(const QString &newDescription)
    {
        assert(Chat_.NewDescription_.isEmpty());

        Chat_.NewDescription_ = newDescription;
    }

	void ChatEventInfo::setNewName(const QString &newName)
	{
		assert(Chat_.NewName_.isEmpty());
		assert(!newName.isEmpty());

		Chat_.NewName_ = newName;
	}

	void ChatEventInfo::setSenderFriendly(const QString &friendly)
	{
		assert(SenderFriendly_.isEmpty());
		assert(!friendly.isEmpty());

		SenderFriendly_ = friendly;
        cleanupFriendlyName(SenderFriendly_);
	}

	void ChatEventInfo::setMchatMembers(const core::iarray &members)
	{
		auto &membersFriendly = Mchat_.MembersFriendly_;

		assert(membersFriendly.isEmpty());

		for (auto index = 0; index < members.size(); ++index)
		{
			auto member = members.get_at(index);
			assert(member);

            QString memberFriendly = member->get_as_string();
            cleanupFriendlyName(memberFriendly);

			membersFriendly << memberFriendly;
		}

		membersFriendly.removeDuplicates();

		assert(membersFriendly.size() == members.size());
	}

}

namespace
{
    void cleanupFriendlyName(QString &name)
    {
        assert(!name.isEmpty());

        name.remove("@uin.icq", Qt::CaseInsensitive);
    }
}