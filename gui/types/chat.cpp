#include "stdafx.h"
#include "chat.h"

#include "../../corelib/collection_helper.h"


namespace Data
{
	void UnserializeChatInfo(core::coll_helper* helper, ChatInfo& info)
	{
		info.AimId_ = helper->get_value_as_string("aimid");
		info.Name_ = helper->get_value_as_string("name");
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
			member.FirstName_ = value.get_value_as_string("first_name");
			member.LastName_ = value.get_value_as_string("last_name");
			member.NickName_ = value.get_value_as_string("nick_name");
			member.Friend_ = value.get_value_as_bool("friend");
			member.NoAvatar_ = value.get_value_as_bool("no_avatar");
			info.Members_.push_back(member);
		}
	}
}