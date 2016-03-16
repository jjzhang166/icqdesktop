#include "stdafx.h"
#include "favorites.h"

#include "../../../corelib/collection_helper.h"

using namespace core;
using namespace wim;

favorite::favorite()
    : time_(0)
{
}

favorite::favorite(const std::string& _aimid, const int64_t _time)
	:	aimid_(_aimid)
    ,   time_(_time)
{
}


void favorite::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
	_node.AddMember("aimId",  get_aimid(), _a);
    _node.AddMember("time", time_, _a);
}

int32_t favorite::unserialize(const rapidjson::Value& _node)
{
	auto iter_aimid = _node.FindMember("aimId");
	
	if (iter_aimid == _node.MemberEnd() || !iter_aimid->value.IsString())
		return -1;

	aimid_ = iter_aimid->value.GetString();

    auto iter_time = _node.FindMember("time");
    if (iter_time == _node.MemberEnd() || !iter_aimid->value.IsInt64())
    {
        time_ = 0;
        return 0;
    }
    
    time_ = iter_time->value.GetInt64();
	
	return 0;
}


favorites::favorites()
	:	changed_(false)
{
}


favorites::~favorites()
{
}

void favorites::update(favorite& _contact)
{
	changed_ = true;

	for (auto iter = contacts_.begin(); iter != contacts_.end(); ++iter)
	{
		if (iter->get_aimid() == _contact.get_aimid())
		{
			contacts_.erase(iter);

			break;
		}
	}
	
	contacts_.push_back(_contact);
    index_[_contact.get_aimid()] = _contact.get_time();
}

void favorites::remove(const std::string& _aimid)
{
	for (auto iter = contacts_.begin(); iter != contacts_.end(); ++iter)
	{
		if (iter->get_aimid() == _aimid)
		{
			contacts_.erase(iter);

			changed_ = true;

			break;
		}
	}

    auto iter = index_.find(_aimid);
    if (iter != index_.end())
        index_.erase(iter);
}

bool favorites::contains(const std::string& _aimid) const
{
    return index_.find(_aimid) != index_.end();
}

int64_t favorites::get_time(const std::string& _aimid) const
{
    auto iter = index_.find(_aimid);
    if (iter != index_.end())
        return iter->second;

    return -1;
}

void favorites::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
	rapidjson::Value node_contacts(rapidjson::Type::kArrayType);

	for (auto iter = contacts_.begin(); iter != contacts_.end(); ++iter)
	{
		rapidjson::Value node_contact(rapidjson::Type::kObjectType);

		iter->serialize(node_contact, _a);

		node_contacts.PushBack(node_contact, _a);
	}

	_node.AddMember("favorites", node_contacts, _a);
}


int32_t favorites::unserialize(const rapidjson::Value& _node)
{
	auto iter_contacts = _node.FindMember("favorites");
	if (iter_contacts == _node.MemberEnd() || !iter_contacts->value.IsArray())
		return -1;

	for (auto iter = iter_contacts->value.Begin(); iter != iter_contacts->value.End(); iter++)
	{
		favorite dlg;
		if (dlg.unserialize(*iter) != 0)
			return -1;

		contacts_.push_back(dlg);
        index_.insert(std::make_pair<std::string, int64_t>(dlg.get_aimid(), dlg.get_time()));
	}
		
	return 0;
}

size_t favorites::size() const
{
    return contacts_.size();
}
