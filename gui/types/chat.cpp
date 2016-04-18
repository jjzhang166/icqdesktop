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

        return AimdId_;
    }

    ChatInfo::ChatInfo()
        : YouBlocked_(false)
        , Public_(false)
        , Live_(false)
        , Controlled_(false)
        , CreateTime_(-1)
        , MembersCount_(-1)
        , FriendsCount(-1)
        , BlockedCount_(-1)
    {
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
		info.MembersVersion_ = helper->get_value_as_string("members_version");
		info.InfoVersion_ = helper->get_value_as_string("info_version");
		info.CreateTime_ =  helper->get_value_as_int("create_time");
		info.MembersCount_ =  helper->get_value_as_int("members_count");
		info.FriendsCount =  helper->get_value_as_int("friend_count");
		info.BlockedCount_ =  helper->get_value_as_int("blocked_count");
		info.YouBlocked_ = helper->get_value_as_bool("you_blocked");
		info.Public_ = helper->get_value_as_bool("public");
		info.Live_ = helper->get_value_as_bool("live");
		info.Controlled_ = helper->get_value_as_bool("controlled");

		core::iarray* membersArray = helper->get_value_as_array("members");
		for (int i = 0; i < membersArray->size(); ++i)
		{
			ChatMemberInfo member;
			core::coll_helper value(membersArray->get_at(i)->get_as_collection(), false);
			member.AimdId_ = value.get_value_as_string("aimid");
			member.Role_ = value.get_value_as_string("role");
			member.FirstName_ = QString(value.get_value_as_string("first_name")).trimmed();
			member.LastName_ = QString(value.get_value_as_string("last_name")).trimmed();
			member.NickName_ = QString(value.get_value_as_string("nick_name")).trimmed();
			member.Friend_ = value.get_value_as_bool("friend");
			member.NoAvatar_ = value.get_value_as_bool("no_avatar");
			info.Members_.push_back(member);
		}
	}
}