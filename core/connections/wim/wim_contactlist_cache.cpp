#include "stdafx.h"

#include "wim_contactlist_cache.h"
#include "wim_im.h"
#include "../../core.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../../tools/system.h"

using namespace core;
using namespace wim;

namespace
{
    bool Contains(const std::string& first, const std::string& second)
    {
        return tools::system::to_upper(first).find(second.c_str()) != std::string::npos;
    }
}

void cl_presence::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("state", state_);
    cl.set_value_as_string("userType", usertype_);
    cl.set_value_as_string("statusMsg", status_msg_);
    cl.set_value_as_string("otherNumber", other_number_);
    cl.set_value_as_string("friendly", friendly_);
    cl.set_value_as_string("abContactName", ab_contact_name_);
    cl.set_value_as_bool("is_chat", is_chat_);
    cl.set_value_as_bool("mute", muted_);
    cl.set_value_as_bool("official", official_);
    cl.set_value_as_int("lastseen", lastseen_);
}


void cl_presence::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("state",  state_, _a);
    _node.AddMember("userType",  usertype_, _a);
    _node.AddMember("statusMsg",  status_msg_, _a);
    _node.AddMember("otherNumber",  other_number_, _a);
    _node.AddMember("friendly",  friendly_, _a);
    _node.AddMember("abContactName",  ab_contact_name_, _a);
    _node.AddMember("lastseen",  lastseen_, _a);
    _node.AddMember("mute",  muted_, _a);
    _node.AddMember("official", official_ ? 1 : 0, _a);

    if (!capabilities_.empty())
    {
        rapidjson::Value node_capabilities(rapidjson::Type::kArrayType);
        for (auto iter_cap = capabilities_.begin(); iter_cap != capabilities_.end(); iter_cap++)
        {
            rapidjson::Value capa;
            capa.SetString(*iter_cap, _a);

            node_capabilities.PushBack(capa, _a);
        }

        _node.AddMember("capabilities", node_capabilities, _a);
    }
}

void cl_presence::unserialize(const rapidjson::Value& _node)
{
    auto iter_state = _node.FindMember("state");
    auto iter_user_type = _node.FindMember("userType");
    auto iter_capabilities = _node.FindMember("capabilities");
    auto iter_statusMsg = _node.FindMember("statusMsg");
    auto iter_otherNumber = _node.FindMember("otherNumber");
    auto iter_friendly = _node.FindMember("friendly");
    auto iter_ab_contact_name = _node.FindMember("abContactName");
    auto iter_last_seen = _node.FindMember("lastseen");
    auto iter_mute = _node.FindMember("mute");
    auto iter_official = _node.FindMember("official");

    if (iter_state != _node.MemberEnd() && iter_state->value.IsString())
        state_ = iter_state->value.GetString();

    if (iter_user_type != _node.MemberEnd() && iter_user_type->value.IsString())
        usertype_ = iter_user_type->value.GetString();

    if (iter_statusMsg != _node.MemberEnd() && iter_statusMsg->value.IsString())
        status_msg_ = iter_statusMsg->value.GetString();

    if (iter_otherNumber != _node.MemberEnd() && iter_otherNumber->value.IsString())
        other_number_ = iter_otherNumber->value.GetString();

    if (iter_friendly != _node.MemberEnd() && iter_friendly->value.IsString())
        friendly_ = iter_friendly->value.GetString();

    if (iter_ab_contact_name != _node.MemberEnd() && iter_ab_contact_name->value.IsString())
        ab_contact_name_ = iter_ab_contact_name->value.GetString();

    if (iter_last_seen != _node.MemberEnd() && iter_last_seen->value.IsUint())
        lastseen_ = iter_last_seen->value.GetUint();

    if (iter_capabilities != _node.MemberEnd() && iter_capabilities->value.IsArray())
    {
        for (auto iter = iter_capabilities->value.Begin(); iter != iter_capabilities->value.End(); ++iter)
            capabilities_.insert(iter->GetString());
    }

    if (iter_mute != _node.MemberEnd())
    {
        if (iter_mute->value.IsUint())
            muted_ = iter_mute->value != 0;
        else if (iter_mute->value.IsBool())
            muted_ = iter_mute->value.GetBool();
    }

    if (iter_official != _node.MemberEnd())
    {
        if (iter_official->value.IsUint())
            official_ = iter_official->value.GetUint() == 1;
    }
}


void contactlist::update_presence(const std::string& _aimid, std::shared_ptr<cl_presence> _presence)
{
    auto iter_contact = contacts_index_.find(_aimid);
    if (iter_contact == contacts_index_.end())
        return;

    iter_contact->second->presence_->state_ = _presence->state_;
    iter_contact->second->presence_->usertype_ = _presence->usertype_;
    iter_contact->second->presence_->status_msg_ = _presence->status_msg_;
    iter_contact->second->presence_->other_number_ = _presence->other_number_;
    iter_contact->second->presence_->capabilities_ = _presence->capabilities_;
    iter_contact->second->presence_->friendly_ = _presence->friendly_;
    iter_contact->second->presence_->lastseen_ = _presence->lastseen_;
    iter_contact->second->presence_->is_chat_ = _presence->is_chat_;
    iter_contact->second->presence_->muted_ = _presence->muted_;
    iter_contact->second->presence_->official_ = _presence->official_;
    set_changed(true);
}

void contactlist::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    rapidjson::Value node_groups(rapidjson::Type::kArrayType);

    for (auto iter_group = groups_.begin(); iter_group != groups_.end(); iter_group++)
    {
        auto group = (*iter_group);

        rapidjson::Value node_group(rapidjson::Type::kObjectType);

        node_group.AddMember("name",  group->name_, _a);
        node_group.AddMember("id", group->id_, _a);

        rapidjson::Value node_buddies(rapidjson::Type::kArrayType);

        for (auto iter_buddy = group->buddies_.begin(); iter_buddy != group->buddies_.end(); iter_buddy++)
        {
            auto buddy = (*iter_buddy);

            rapidjson::Value node_buddy(rapidjson::Type::kObjectType);

            node_buddy.AddMember("aimId",  buddy->aimid_, _a);

            buddy->presence_->serialize(node_buddy, _a);

            node_buddies.PushBack(node_buddy, _a);
        }

        node_group.AddMember("buddies", node_buddies, _a);
        node_groups.PushBack(node_group, _a);
    }

    _node.AddMember("groups", node_groups, _a);
}

void core::wim::contactlist::serialize(icollection* _coll, const std::string& type)
{
    coll_helper cl(_coll, false);

    ifptr<iarray> groups_array(_coll->create_array());
    groups_array->reserve((int32_t)groups_.size());

    for (auto iter_group = groups_.begin(); iter_group != groups_.end(); iter_group++)
    {
        auto group = (*iter_group);

        coll_helper group_coll(_coll->create_collection(), true);
        group_coll.set_value_as_string("group_name", group->name_);
        group_coll.set_value_as_int("group_id", group->id_);
        group_coll.set_value_as_bool("added", group->added_);
        group_coll.set_value_as_bool("removed", group->removed_);

        ifptr<iarray> contacts_array(_coll->create_array());
        contacts_array->reserve((int32_t)group->buddies_.size());

        for (auto iter_buddy = group->buddies_.begin(); iter_buddy != group->buddies_.end(); iter_buddy++)
        {
            auto buddy = (*iter_buddy);

            coll_helper contact_coll(_coll->create_collection(), true);
            contact_coll.set_value_as_string("aimId", buddy->aimid_);

            buddy->presence_->serialize(contact_coll.get());

            ifptr<ivalue> val_contact(_coll->create_value());
            val_contact->set_as_collection(contact_coll.get());
            contacts_array->push_back(val_contact.get());
        }

        ifptr<ivalue> val_group(_coll->create_value());
        val_group->set_as_collection(group_coll.get());
        groups_array->push_back(val_group.get());

        group_coll.set_value_as_array("contacts", contacts_array.get());
    }

    if (!type.empty())
        cl.set_value_as_string("type", type);

    cl.set_value_as_array("groups", groups_array.get());
}

bool core::wim::contactlist::search(std::vector<std::string> search_patterns)
{
    for (std::vector<std::string>::iterator iter = search_patterns.begin(); iter != search_patterns.end(); ++iter)
        *iter = tools::system::to_upper(*iter);
    std::map< std::string, std::shared_ptr<cl_buddy> > casche;
    if (last_search_patterns_.empty() || search_patterns[0].find(last_search_patterns_[0].c_str()) == std::string::npos)
        casche = contacts_index_;
    else
        casche = search_cache_;

    std::map< std::string, std::shared_ptr<cl_buddy> > result;
    std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter = casche.begin();
    while (g_core->is_valid_search() && iter != casche.end())
    {
        for (auto pattern : search_patterns)
        {
            if ((Contains(iter->second->aimid_, pattern) || 
                Contains(iter->second->presence_->friendly_, pattern) ||
                Contains(iter->second->presence_->ab_contact_name_, pattern)) && iter->second->presence_->usertype_ != "sms")
                result.insert(std::make_pair(iter->first, iter->second));
        }
        ++iter;
    }

    if (g_core->end_search() == 0)
    {
        last_search_patterns_ = search_patterns;
        search_cache_.swap(result);
        return true;
    }

    return false;
}

void core::wim::contactlist::serialize_search(icollection* _coll)
{
    coll_helper cl(_coll, false);

    ifptr<iarray> contacts_array(_coll->create_array());
    contacts_array->reserve((int32_t)search_cache_.size());

    for (auto iter_buddy = search_cache_.begin(); iter_buddy != search_cache_.end(); iter_buddy++)
    {
        auto buddy = (*iter_buddy);

        coll_helper contact_coll(_coll->create_collection(), true);
        contact_coll.set_value_as_string("aimId", iter_buddy->first);
        ifptr<ivalue> val_contact(_coll->create_value());
        val_contact->set_as_collection(contact_coll.get());
        contacts_array->push_back(val_contact.get());
    }
    cl.set_value_as_array("contacts", contacts_array.get());
}

std::string contactlist::get_contact_friendly_name(const std::string& contact_login) {
    auto it = contacts_index_.find(contact_login);
    if (contacts_index_.end() != it && !!it->second->presence_) {
        return it->second->presence_->friendly_;
    }
    return "";
}

int32_t contactlist::unserialize(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    const std::string chat_domain = "@chat.agent";

    auto iter_groups = _node.FindMember("groups");
    if (iter_groups == _node.MemberEnd() || !iter_groups->value.IsArray())
        return 0;

    for (auto iter_grp = iter_groups->value.Begin(); iter_grp != iter_groups->value.End(); iter_grp++)
    {
        auto group = std::make_shared<core::wim::cl_group>();

        auto iter_group_name = iter_grp->FindMember("name");
        if (iter_group_name != iter_grp->MemberEnd() && iter_group_name->value.IsString())
        {
            group->name_ = iter_group_name->value.GetString();
        }
        else
        {
            assert(false);
        }

        auto iter_group_id = iter_grp->FindMember("id");
        if (iter_group_id == iter_grp->MemberEnd() || !iter_group_id->value.IsUint())
        {
            assert(false);
            continue;
        }

        group->id_ = iter_group_id->value.GetUint();
        groups_.push_back(group);

        auto iter_buddies = iter_grp->FindMember("buddies");
        if (iter_buddies != iter_grp->MemberEnd() && iter_buddies->value.IsArray())
        {
            for (auto iter_bd = iter_buddies->value.Begin(); iter_bd != iter_buddies->value.End(); iter_bd++)
            {
                auto buddy = std::make_shared<wim::cl_buddy>();

                buddy->id_ = (uint32_t)++buddy_id;

                auto iter_aimid = iter_bd->FindMember("aimId");
                if (iter_aimid == iter_bd->MemberEnd() || !iter_aimid->value.IsString())
                {
                    assert(false);
                    continue;
                }

                buddy->aimid_ = iter_aimid->value.GetString();

                buddy->presence_->unserialize(*iter_bd);

                if (buddy->aimid_.length() > chat_domain.length() && buddy->aimid_.substr(buddy->aimid_.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                group->buddies_.push_back(buddy);

                contacts_index_[buddy->aimid_] = buddy;
            }
        }
    }

    return 0;
}

int32_t contactlist::unserialize_from_diff(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    const std::string chat_domain = "@chat.agent";

    for (auto iter_grp = _node.Begin(); iter_grp != _node.End(); iter_grp++)
    {
        auto group = std::make_shared<core::wim::cl_group>();

        auto iter_group_name = iter_grp->FindMember("name");
        if (iter_group_name != iter_grp->MemberEnd() && iter_group_name->value.IsString())
        {
            group->name_ = iter_group_name->value.GetString();
        }
        else
        {
            assert(false);
        }

        auto iter_group_id = iter_grp->FindMember("id");
        if (iter_group_id == iter_grp->MemberEnd() || !iter_group_id->value.IsUint())
        {
            assert(false);
            continue;
        }

        group->id_ = iter_group_id->value.GetUint();

        auto iter_group_added = iter_grp->FindMember("added");
        if (iter_group_added != iter_grp->MemberEnd())
        {
            group->added_ = true;
        }

        auto iter_group_removed = iter_grp->FindMember("removed");
        if (iter_group_removed != iter_grp->MemberEnd())
        {
            group->removed_ = true;
        }

        groups_.push_back(group);

        auto iter_buddies = iter_grp->FindMember("buddies");
        if (iter_buddies != iter_grp->MemberEnd() && iter_buddies->value.IsArray())
        {
            for (auto iter_bd = iter_buddies->value.Begin(); iter_bd != iter_buddies->value.End(); iter_bd++)
            {
                auto buddy = std::make_shared<wim::cl_buddy>();

                buddy->id_ = (uint32_t)++buddy_id;

                auto iter_aimid = iter_bd->FindMember("aimId");
                if (iter_aimid == iter_bd->MemberEnd() || !iter_aimid->value.IsString())
                {
                    assert(false);
                    continue;
                }

                buddy->aimid_ = iter_aimid->value.GetString();

                buddy->presence_->unserialize(*iter_bd);

                if (buddy->aimid_.length() > chat_domain.length() && buddy->aimid_.substr(buddy->aimid_.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                group->buddies_.push_back(buddy);

                contacts_index_[buddy->aimid_] = buddy;
            }
        }
    }

    return 0;
}

void contactlist::merge_from_diff(const std::string& _type, std::shared_ptr<contactlist> _diff, std::shared_ptr<std::list<std::string>> removedContacts)
{
    if (_type == "created")
    {
        for (auto diff_group_iter = _diff->groups_.begin(); diff_group_iter != _diff->groups_.end(); ++diff_group_iter)
        {
            if ((*diff_group_iter)->added_)
            {
                auto group = std::make_shared<cl_group>();
                group->id_ = (*diff_group_iter)->id_;
                group->name_ = (*diff_group_iter)->name_;
                groups_.push_back(group);
            }


            for (auto group_iter = groups_.begin(); group_iter != groups_.end(); ++group_iter)
            {
                if ((*group_iter)->id_ != (*diff_group_iter)->id_)
                    continue;

                for (auto diff_buddy_iter = (*diff_group_iter)->buddies_.begin(); diff_buddy_iter != (*diff_group_iter)->buddies_.end(); ++diff_buddy_iter)
                {
                    (*group_iter)->buddies_.push_back((*diff_buddy_iter));
                    contacts_index_[(*diff_buddy_iter)->aimid_] = (*diff_buddy_iter);
                }
            }
        }
    }
    else if (_type == "updated")
    {
        for (auto diff_group_iter = _diff->groups_.begin(); diff_group_iter != _diff->groups_.end(); ++diff_group_iter)
        {
            for (auto diff_buddy_iter = (*diff_group_iter)->buddies_.begin(); diff_buddy_iter != (*diff_group_iter)->buddies_.end(); ++diff_buddy_iter)
            {
                update_presence((*diff_buddy_iter)->aimid_, (*diff_buddy_iter)->presence_);
            }
        }
    }
    else if (_type == "deleted")
    {
        for (auto diff_group_iter = _diff->groups_.begin(); diff_group_iter != _diff->groups_.end(); ++diff_group_iter)
        {
            for (auto group_iter = groups_.begin(); group_iter != groups_.end();)
            {
                if ((*group_iter)->id_ != (*diff_group_iter)->id_)
                {
                    ++group_iter;
                    continue;
                }

                for (auto diff_buddy_iter = (*diff_group_iter)->buddies_.begin(); diff_buddy_iter != (*diff_group_iter)->buddies_.end(); ++diff_buddy_iter)
                {
                    std::string aimId = (*diff_buddy_iter)->aimid_;
                    (*group_iter)->buddies_.erase(std::remove_if((*group_iter)->buddies_.begin(), (*group_iter)->buddies_.end(), [aimId](std::shared_ptr<cl_buddy> value){ return value->aimid_ == aimId; }), (*group_iter)->buddies_.end());
                    contacts_index_.erase(aimId);
                    removedContacts->push_back(aimId);
                }

                if ((*diff_group_iter)->removed_)
                    group_iter = groups_.erase(group_iter);
                else
                    ++group_iter;
            }
        }
    }
    else
    {
        return;
    }
    changed_ = true;
}

int contactlist::GetPhoneContactCounts() const
{
    return std::count_if (contacts_index_.begin(), contacts_index_.end(), [](std::pair<std::string, std::shared_ptr<cl_buddy>> contact)
    {return contact.second.get()->aimid_.find("+") != contact.second.get()->aimid_.npos;});
}

int contactlist::GetGroupChatContactCounts() const
{
    return std::count_if (contacts_index_.begin(), contacts_index_.end(), [](std::pair<std::string, std::shared_ptr<cl_buddy>> contact)
    {return contact.second.get()->aimid_.find("@chat.agent") != contact.second.get()->aimid_.npos;});
}
