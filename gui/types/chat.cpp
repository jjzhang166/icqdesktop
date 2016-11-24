#include "stdafx.h"
#include "chat.h"

#include "../../corelib/collection_helper.h"


namespace Data
{
    const QString ChatMemberInfo::getFriendly() const
    {
        if (!NickName_.isEmpty())
            return NickName_;

        QString friendly;
        if (!FirstName_.isEmpty())
            friendly += FirstName_;

        if (!LastName_.isEmpty())
        {
            if (!friendly.isEmpty())
                friendly += " ";

            friendly += LastName_;
        }

        if (!friendly.isEmpty())
            return friendly;

        return AimId_;
    }

    ChatInfo::ChatInfo()
        : YouBlocked_(false)
        , Public_(false)
        , ApprovedJoin_(false)
        , Live_(false)
        , Controlled_(false)
        , YouPending_(false)
        , YouMember_(false)
        , AgeRestriction_(false)
        , CreateTime_(-1)
        , MembersCount_(-1)
        , FriendsCount(-1)
        , BlockedCount_(-1)
        , PendingCount_(-1)
    {
    }

    void UnserializeChatMembers(core::coll_helper* helper, QList<ChatMemberInfo>& members)
    {
        members.clear();
        core::iarray* membersArray = helper->get_value_as_array("members");
        for (int i = 0; i < membersArray->size(); ++i)
        {
            ChatMemberInfo member;
            core::coll_helper value(membersArray->get_at(i)->get_as_collection(), false);
            member.AimId_ = value.get_value_as_string("aimid");
            if (value.is_value_exist("role"))
                member.Role_ = value.get_value_as_string("role");
            member.FirstName_ = QString(value.get_value_as_string("first_name")).trimmed();
            member.LastName_ = QString(value.get_value_as_string("last_name")).trimmed();
            member.NickName_ = QString(value.get_value_as_string("nick_name")).trimmed();
            if (value.is_value_exist("friend"))
                member.Friend_ = value.get_value_as_bool("friend");
            if (value.is_value_exist("no_avatar"))
                member.NoAvatar_ = value.get_value_as_bool("no_avatar");
            members.push_back(member);
        }
    }

	void UnserializeChatInfo(core::coll_helper* helper, ChatInfo& info)
	{
		info.AimId_ = helper->get_value_as_string("aimid");
		info.Name_ = QString(helper->get_value_as_string("name")).trimmed();
        info.Location_ = helper->get_value_as_string("location");
        info.Stamp_ = helper->get_value_as_string("stamp");
		info.About_ = helper->get_value_as_string("about");
		info.YourRole_ = helper->get_value_as_string("your_role");
		info.Owner_ = helper->get_value_as_string("owner");
        info.Creator_ = helper->get_value_as_string("creator");
        info.DefaultRole_ = helper->get_value_as_string("default_role");
		info.MembersVersion_ = helper->get_value_as_string("members_version");
		info.InfoVersion_ = helper->get_value_as_string("info_version");
		info.CreateTime_ =  helper->get_value_as_int("create_time");
		info.MembersCount_ =  helper->get_value_as_int("members_count");
		info.FriendsCount =  helper->get_value_as_int("friend_count");
		info.BlockedCount_ =  helper->get_value_as_int("blocked_count");
        info.PendingCount_ =  helper->get_value_as_int("pending_count");
		info.YouBlocked_ = helper->get_value_as_bool("you_blocked");
        info.YouPending_ = helper->get_value_as_bool("you_pending");
        info.YouMember_ = helper->get_value_as_bool("you_member");
		info.Public_ = helper->get_value_as_bool("public");
		info.Live_ = helper->get_value_as_bool("live");
		info.Controlled_ = helper->get_value_as_bool("controlled");
        info.Stamp_ = helper->get_value_as_string("stamp");
        info.ApprovedJoin_ = helper->get_value_as_bool("joinModeration");
        info.AgeRestriction_ = helper->get_value_as_bool("age_restriction");
        UnserializeChatMembers(helper, info.Members_);
	}

    void UnserializeChatHome(core::coll_helper* helper, QList<ChatInfo>& chats, QString& newTag, bool& restart, bool& finished)
    {
        newTag = helper->get_value_as_string("new_tag");
        restart = helper->get_value_as_bool("need_restart");
        finished = helper->get_value_as_bool("finished");
        core::iarray* chatsArray = helper->get_value_as_array("chats");
        chats.clear();
        for (int i = 0; i < chatsArray->size(); ++i)
        {
            ChatInfo info;
            core::coll_helper value(chatsArray->get_at(i)->get_as_collection(), false);
            UnserializeChatInfo(&value, info);
            chats.append(info);
        }
    }
}