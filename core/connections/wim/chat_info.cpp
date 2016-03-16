#include "stdafx.h"
#include "chat_info.h"

using namespace core;
using namespace wim;

chat_info::chat_info()
	: create_time_(0)
	, members_count_(0)
	, friend_count_(0)
	, blocked_count_(0)
	, you_blocked_(false)
	, public_(false)
	, live_(false)
	, controlled_(false)
{
}

int32_t chat_info::unserialize(const rapidjson::Value& _node)
{
	auto iter_aimid = _node.FindMember("sn");
	if (iter_aimid != _node.MemberEnd() && iter_aimid->value.IsString())
		aimid_ =  iter_aimid->value.GetString();

	auto iter_name = _node.FindMember("name");
	if (iter_name != _node.MemberEnd() && iter_name->value.IsString())
		name_ =  iter_name->value.GetString();

	auto iter_about = _node.FindMember("about");
	if (iter_about != _node.MemberEnd() && iter_about->value.IsString())
		about_ =  iter_about->value.GetString();

	auto iter_create_time = _node.FindMember("createTime");
	if (iter_create_time != _node.MemberEnd() && iter_create_time->value.IsInt())
		create_time_ =  iter_create_time->value.GetInt();

	auto iter_members_count = _node.FindMember("membersCount");
	if (iter_members_count != _node.MemberEnd() && iter_members_count->value.IsInt())
		members_count_ =  iter_members_count->value.GetInt();

	auto iter_friends_count = _node.FindMember("friendsCount");
	if (iter_friends_count != _node.MemberEnd() && iter_friends_count->value.IsInt())
		friend_count_ =  iter_friends_count->value.GetInt();

	auto iter_yours = _node.FindMember("you");
	if (iter_yours != _node.MemberEnd())
	{
		auto iter_your_role = iter_yours->value.FindMember("role");
		if (iter_your_role != iter_yours->value.MemberEnd() && iter_your_role->value.IsString())
			your_role_ = iter_your_role->value.GetString();

		auto iter_you_blocked = iter_yours->value.FindMember("blocked");
		if (iter_you_blocked != iter_yours->value.MemberEnd() && iter_you_blocked->value.IsBool())
			you_blocked_ = iter_you_blocked->value.GetBool();
	}

	auto iter_public = _node.FindMember("public");
	if (iter_public != _node.MemberEnd() && iter_public->value.IsBool())
		public_ =  iter_public->value.GetBool();

	auto iter_live = _node.FindMember("live");
	if (iter_live != _node.MemberEnd() && iter_live->value.IsBool())
		live_ =  iter_live->value.GetBool();

	auto iter_controlled = _node.FindMember("controlled");
	if (iter_controlled != _node.MemberEnd() && iter_controlled->value.IsBool())
		controlled_ =  iter_controlled->value.GetBool();

	auto iter_blocked_count = _node.FindMember("blockedCount");
	if (iter_blocked_count != _node.MemberEnd() && iter_blocked_count->value.IsInt())
		blocked_count_ =  iter_blocked_count->value.GetInt();

	auto iter_members_version = _node.FindMember("membersVersion");
	if (iter_members_version != _node.MemberEnd() && iter_members_version->value.IsString())
		members_version_ =  iter_members_version->value.GetString();

	auto iter_info_version = _node.FindMember("infoVersion");
	if (iter_info_version != _node.MemberEnd() && iter_info_version->value.IsString())
		info_version_ =  iter_info_version->value.GetString();

	auto iter_members = _node.FindMember("members");
	if (iter_members != _node.MemberEnd() && iter_members->value.IsArray())
	{
		for (auto iter = iter_members->value.Begin(); iter != iter_members->value.End(); iter++)
		{
			chat_member_info member_info;
			auto iter_aimid = iter->FindMember("sn");
			if (iter_aimid != iter->MemberEnd() && iter_aimid->value.IsString())
				member_info.aimid_ =  iter_aimid->value.GetString();

			auto iter_role = iter->FindMember("role");
			if (iter_role != iter->MemberEnd() && iter_role->value.IsString())
				member_info.role_ = iter_role->value.GetString();

			auto iter_friend = iter->FindMember("friend");
			if (iter_friend != iter->MemberEnd() && iter_friend->value.IsBool())
				member_info.friend_ = iter_friend->value.GetBool();

			auto iter_no_avatar = iter->FindMember("noAvatar");
			if (iter_no_avatar != iter->MemberEnd() && iter_no_avatar->value.IsBool())
				member_info.no_avatar_ = iter_no_avatar->value.GetBool();

			auto iter_anketa = iter->FindMember("anketa");
			if (iter_anketa != iter->MemberEnd())
			{
				auto iter_first_name = iter_anketa->value.FindMember("firstName");
				if (iter_first_name != iter_anketa->value.MemberEnd() && iter_first_name->value.IsString())
					member_info.first_name_ = iter_first_name->value.GetString();

				auto iter_last_name = iter_anketa->value.FindMember("lastName");
				if (iter_last_name != iter_anketa->value.MemberEnd() && iter_last_name->value.IsString())
					member_info.last_name_ = iter_last_name->value.GetString();

				auto iter_nickname = iter_anketa->value.FindMember("nickname");
				if (iter_nickname != iter_anketa->value.MemberEnd() && iter_nickname->value.IsString())
					member_info.nick_name_ = iter_nickname->value.GetString();
			}

			members_.push_back(member_info);
		}
	}
	
	auto iter_owner = _node.FindMember("owner");
	if (iter_owner != _node.MemberEnd())
	{
		auto iter_owner_aimid = iter_yours->value.FindMember("sn");
		if (iter_owner_aimid != iter_owner->value.MemberEnd() && iter_owner_aimid->value.IsString())
			owner_ = iter_owner_aimid->value.GetString();
	}
		
	return 0;
}

void chat_info::serialize(core::coll_helper _coll)
{
	_coll.set_value_as_string("aimid", aimid_);
	_coll.set_value_as_string("name", name_);
	_coll.set_value_as_string("about", about_);
	_coll.set_value_as_string("your_role", your_role_);
	_coll.set_value_as_string("owner", owner_);
	_coll.set_value_as_string("members_version", members_version_);
	_coll.set_value_as_string("info_version", info_version_);
	_coll.set_value_as_int("create_time", create_time_);
	_coll.set_value_as_int("members_count", members_count_);
	_coll.set_value_as_int("friend_count", friend_count_);
	_coll.set_value_as_int("blocked_count", blocked_count_);
	_coll.set_value_as_bool("you_blocked", you_blocked_);
	_coll.set_value_as_bool("public", public_);
	_coll.set_value_as_bool("live", live_);
	_coll.set_value_as_bool("controlled", controlled_);

	ifptr<iarray> members_array(_coll->create_array());

	if (!members_.empty())
	{
		members_array->reserve((int)members_.size());
		for (const auto member : members_)
		{
			coll_helper _coll_message(_coll->create_collection(), true);
			_coll_message.set_value_as_string("aimid", member.aimid_);
			_coll_message.set_value_as_string("role", member.role_);
			_coll_message.set_value_as_string("first_name", member.first_name_);
			_coll_message.set_value_as_string("last_name", member.last_name_);
			_coll_message.set_value_as_string("nick_name", member.nick_name_);
			_coll_message.set_value_as_bool("friend", member.friend_);
			_coll_message.set_value_as_bool("no_avatar", member.no_avatar_);
			ifptr<ivalue> val(_coll->create_value());
			val->set_as_collection(_coll_message.get());
			members_array->push_back(val.get());
		}
	}

	_coll.set_value_as_array("members", members_array.get());
}