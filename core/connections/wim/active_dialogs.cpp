#include "stdafx.h"
#include "active_dialogs.h"

#include "../../../corelib/collection_helper.h"

using namespace core;
using namespace wim;

active_dialog::active_dialog()
{

}

active_dialog::active_dialog(const std::string& _aimid)
	:	aimid_(_aimid)
{
}


void active_dialog::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
	_node.AddMember("aimId",  get_aimid(), _a);
}

int32_t active_dialog::unserialize(const rapidjson::Value& _node)
{
	auto iter_aimid = _node.FindMember("aimId");
	
	if (iter_aimid == _node.MemberEnd() || !iter_aimid->value.IsString())
		return -1;

	aimid_ = iter_aimid->value.GetString();
	
	return 0;
}

void active_dialog::serialize(icollection* _coll)
{
	coll_helper cl(_coll, false);
	cl.set_value_as_string("aimId", aimid_);
}





active_dialogs::active_dialogs()
	:	changed_(false)
{
}


active_dialogs::~active_dialogs()
{
}

void active_dialogs::update(active_dialog& _dialog)
{
	changed_ = true;

	for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
	{
		if (iter->get_aimid() == _dialog.get_aimid())
		{
			dialogs_.erase(iter);

			break;
		}
	}
	
	dialogs_.push_front(_dialog);
}

void active_dialogs::remove(const std::string& _aimid)
{
	for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
	{
		if (iter->get_aimid() == _aimid)
		{
			dialogs_.erase(iter);

			changed_ = true;

			break;
		}
	}
}

bool active_dialogs::contains(const std::string& _aimId)
{
    for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
    {
        if (iter->get_aimid() == _aimId)
        {
            return true;
        }
    }

    return false;
}

void active_dialogs::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
	rapidjson::Value node_dialogs(rapidjson::Type::kArrayType);

	for (auto iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
	{
		rapidjson::Value node_dialog(rapidjson::Type::kObjectType);

		iter->serialize(node_dialog, _a);

		node_dialogs.PushBack(node_dialog, _a);
	}

	_node.AddMember("dialogs", node_dialogs, _a);
}


int32_t active_dialogs::unserialize(const rapidjson::Value& _node)
{
	auto iter_dialogs = _node.FindMember("dialogs");
	if (iter_dialogs == _node.MemberEnd() || !iter_dialogs->value.IsArray())
		return -1;

	for (auto iter = iter_dialogs->value.Begin(); iter != iter_dialogs->value.End(); iter++)
	{
		active_dialog dlg;
		if (dlg.unserialize(*iter) != 0)
			return -1;

		dialogs_.push_back(dlg);
	}
		
	return 0;
}

void active_dialogs::serialize(icollection* _coll)
{
	coll_helper cl(_coll, false);

	ifptr<iarray> dialogs_array(_coll->create_array());
	dialogs_array->reserve((uint32_t)dialogs_.size());

	for (auto iter = dialogs_.begin(); iter != dialogs_.end(); iter++)
	{
		coll_helper dlg_coll(_coll->create_collection(), true);
		iter->serialize(dlg_coll.get());

		ifptr<ivalue> val_dlg(_coll->create_value());
		val_dlg->set_as_collection(dlg_coll.get());
		dialogs_array->push_back(val_dlg.get());
	}

	cl.set_value_as_array("dialogs", dialogs_array.get());
}

size_t active_dialogs::size() const
{
    return dialogs_.size();
}

void active_dialogs::enumerate(std::function<void(const active_dialog&)> _callback)
{
	for (auto iter = dialogs_.rbegin(); iter != dialogs_.rend(); iter++)
		_callback(*iter);
}
