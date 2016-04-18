#include "stdafx.h"
#include "contact_profile.h"

namespace core
{
    namespace profile
    {
        bool address::unserialize(const rapidjson::Value& _node)
        {
            auto iter_city = _node.FindMember("city");
            if (iter_city != _node.MemberEnd() && iter_city->value.IsString())
                city_ = iter_city->value.GetString();

            auto iter_state = _node.FindMember("state");
            if (iter_state != _node.MemberEnd() && iter_state->value.IsString())
                state_ = iter_state->value.GetString();

            auto iter_country = _node.FindMember("country");
            if (iter_country != _node.MemberEnd() && iter_country->value.IsString())
                country_ = iter_country->value.GetString();

            return true;
        }

        void address::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("city", city_);
            _coll.set_value_as_string("state", state_);
            _coll.set_value_as_string("country", country_);
        }

        bool phone::unserialize(const rapidjson::Value &_node)
        {
            auto iter_phone = _node.FindMember("number");
            if (iter_phone != _node.MemberEnd() && iter_phone->value.IsString())
                phone_ = iter_phone->value.GetString();

            auto iter_type = _node.FindMember("type");
            if (iter_type != _node.MemberEnd() && iter_type->value.IsString())
                type_ = iter_type->value.GetString();

            return true;
        }

        void phone::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("phone", phone_);
            _coll.set_value_as_string("type", type_);
        }

        bool info::unserialize(const rapidjson::Value& _root)
        {
            auto iter_phones = _root.FindMember("abPhones");
            if (iter_phones != _root.MemberEnd() && iter_phones->value.IsArray())
            {
                for (auto iter_phone = iter_phones->value.Begin(), iter_phone_end = iter_phones->value.End(); iter_phone != iter_phone_end; ++iter_phone)
                {
                    phone p;
                    if (p.unserialize(*iter_phone))
                        phones_.push_back(p);
                }
            }

            auto iter_profile = _root.FindMember("profile");
            if (iter_profile == _root.MemberEnd() || !iter_profile->value.IsObject())
                return false;

            const rapidjson::Value& _node = iter_profile->value;

            auto iter_first_name = _node.FindMember("firstName");
            if (iter_first_name != _node.MemberEnd() && iter_first_name->value.IsString())
                first_name_ = iter_first_name->value.GetString();

            auto iter_last_name = _node.FindMember("lastName");
            if (iter_last_name != _node.MemberEnd() && iter_last_name->value.IsString())
                last_name_ = iter_last_name->value.GetString();

            auto iter_friendly_name = _node.FindMember("friendlyName");
            if (iter_friendly_name != _node.MemberEnd() && iter_friendly_name->value.IsString())
                friendly_ = iter_friendly_name->value.GetString();

            auto iter_displayid = _node.FindMember("displayId");
            if (iter_displayid != _node.MemberEnd() && iter_displayid->value.IsString())
                displayid_ = iter_displayid->value.GetString();

            auto iter_gender = _node.FindMember("gender");
            if (iter_gender != _node.MemberEnd() && iter_gender->value.IsString())
            {
                std::string sex = iter_gender->value.GetString();
                if (sex == "male")
                    gender_ = gender::male;
                else if (sex == "female")
                    gender_ = gender::female;
                else
                    gender_ = gender::unknown;
            }

            auto iter_relationship = _node.FindMember("relationshipStatus");
            if (iter_relationship != _node.MemberEnd() && iter_relationship->value.IsString())
                relationship_ = iter_relationship->value.GetString();

            auto iter_birthdate = _node.FindMember("birthDate");
            if (iter_birthdate != _node.MemberEnd() && iter_birthdate->value.IsInt64())
                birthdate_ = iter_birthdate->value.GetInt64();

            auto iter_home_address = _node.FindMember("homeAddress");
            if (iter_home_address != _node.MemberEnd() && iter_home_address->value.IsArray())
            {
                if (iter_home_address->value.Size() > 0)
                    home_address_.unserialize(*iter_home_address->value.Begin());
            }

            auto iter_about = _node.FindMember("aboutMe");
            if (iter_about != _node.MemberEnd() && iter_about->value.IsString())
                about_ = iter_about->value.GetString();

            return true;
        }

        void info::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("aimid", aimid_);
            _coll.set_value_as_string("firstname", first_name_);
            _coll.set_value_as_string("lastname", last_name_);
            _coll.set_value_as_string("friendly", friendly_);
            _coll.set_value_as_string("displayid", displayid_);
            _coll.set_value_as_string("relationship", relationship_);
            _coll.set_value_as_string("about", about_);
            _coll.set_value_as_int64("birthdate", birthdate_);
            std::string sgender;
            if (gender_ == gender::male)
                sgender = "male";
            else if (gender_ == gender::female)
                sgender = "female";
            _coll.set_value_as_string("gender", sgender);

            coll_helper coll_address(_coll->create_collection(), true);
            home_address_.serialize(coll_address);
            _coll.set_value_as_collection("homeaddress", coll_address.get());

            ifptr<iarray> phones(_coll->create_array());
            phones->reserve((int)phones_.size());
            for (auto phone: phones_)
            {
                coll_helper coll_phone(_coll->create_collection(), true);
                phone.serialize(coll_phone);

                ifptr<ivalue> val(_coll->create_value());
                val->set_as_collection(coll_phone.get());

                phones->push_back(val.get());
            }
            _coll.set_value_as_array("phones", phones.get());
        }
    }

}

