#include "stdafx.h"
#include "search_contacts_response.h"

using namespace core;
using namespace wim;

search_contacts_response::search_contacts_response()
{
    //
}

int32_t search_contacts_response::unserialize(const rapidjson::Value& array)
{
    data_.clear();
    for (auto node = array.Begin(), nend = array.End(); node != nend; ++node)
    {
        chunk c;
        { auto i = node->FindMember("sn"); if (i != node->MemberEnd() && i->value.IsString()) c.aimid_ = i->value.GetString(); }
        { auto i = node->FindMember("stamp"); if (i != node->MemberEnd() && i->value.IsString()) c.stamp_ = i->value.GetString(); }
        { auto i = node->FindMember("type"); if (i != node->MemberEnd() && i->value.IsString()) c.type_ = i->value.GetString(); }
        { auto i = node->FindMember("score"); if (i != node->MemberEnd() && i->value.IsInt()) c.score_ = i->value.GetInt(); }
        {
            auto a = node->FindMember("anketa");
            if (a != node->MemberEnd() && a->value.IsObject())
            {
                const auto& sub_node = a->value;
                { auto i = sub_node.FindMember("firstName"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.first_name_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("lastName"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.last_name_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("nickname"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.nick_name_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("city"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.city_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("state"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.state_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("country"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.country_ = i->value.GetString(); }
                { auto i = sub_node.FindMember("gender"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.gender_ = i->value.GetString(); }
                {
                    auto b = sub_node.FindMember("birthDate");
                    if (b != sub_node.MemberEnd() && b->value.IsObject())
                    {
                        const auto& sub_sub_node = b->value;
                        { auto i = sub_sub_node.FindMember("year"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.year_= i->value.GetInt(); }
                        { auto i = sub_sub_node.FindMember("month"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.month_= i->value.GetInt(); }
                        { auto i = sub_sub_node.FindMember("day"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.day_= i->value.GetInt(); }
                    }
                }
                { auto i = sub_node.FindMember("aboutMeCut"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.about_ = i->value.GetString(); }
            }
        }
        data_.push_back(c);
    }
	return 0;
}

void search_contacts_response::serialize(core::coll_helper root_coll)
{
    if (data_.empty())
        return;
    ifptr<iarray> array(root_coll->create_array());
    array->reserve((int)data_.size());
    for (auto c: data_)
    {
        coll_helper coll(root_coll->create_collection(), true);
        
        coll.set_value_as_string("aimid", c.aimid_);
        coll.set_value_as_string("stamp", c.stamp_);
        coll.set_value_as_string("type", c.type_);
        if (c.score_ >= 0)
        {
            coll.set_value_as_int("score", c.score_);
        }
        coll.set_value_as_string("first_name", c.first_name_);
        coll.set_value_as_string("last_name", c.last_name_);
        coll.set_value_as_string("nick_name", c.nick_name_);
        coll.set_value_as_string("city", c.city_);
        coll.set_value_as_string("state", c.state_);
        coll.set_value_as_string("country", c.country_);
        coll.set_value_as_string("gender", c.gender_);
        if (c.birthdate_.year_ >= 0 && c.birthdate_.month_ >= 0 && c.birthdate_.day_ >= 0)
        {
            coll_helper b(root_coll->create_collection(), true);
            b.set_value_as_int("year", c.birthdate_.year_);
            b.set_value_as_int("month", c.birthdate_.month_);
            b.set_value_as_int("day", c.birthdate_.day_);
            coll.set_value_as_collection("birthdate", b.get());
        }
        coll.set_value_as_string("about", c.about_);

        ifptr<ivalue> val(root_coll->create_value());
        val->set_as_collection(coll.get());
        array->push_back(val.get());
        
        root_coll.set_value_as_array("data", array.get());
    }
}
