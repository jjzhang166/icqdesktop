#include "stdafx.h"
#include "search_contacts_params.h"

namespace core
{
    search_params::search_params(void)
        :	gender_(profile::gender::unknown),
        age_(-1, -1),
        skip_count_(0),
        count_(0)
    {
    }


    search_params::~search_params(void)
    {
    }

    const std::string& search_params::get_keyword() const
    {
        return keyword_;
    }

    void search_params::set_keyword(const std::string& _val)
    {
        keyword_ = _val;
    }

    profile::gender search_params::get_gender() const
    {
        return gender_;
    }

    void search_params::set_gender(profile::gender _val)
    {
        gender_ = _val;
    }

    const std::string& search_params::get_country() const
    {
        return country_;
    }

    void search_params::set_country(const std::string& _val)
    {
        country_ = _val;
    }

    const std::pair<int32_t, int32_t>& search_params::get_age() const
    {
        return age_;
    }

    void search_params::set_age(const std::pair<int32_t, int32_t>& _age)
    {
        age_ = _age;
    }

    int32_t search_params::get_skip_count() const
    {
        return skip_count_;
    }

    void search_params::set_skip_count(int32_t _val)
    {
        skip_count_ = _val;
    }

    int32_t search_params::get_count() const
    {
        return count_;
    }

    void search_params::set_count(int32_t _val)
    {
        count_ = _val;
    }

    void search_params::unserialize(core::coll_helper _coll)
    {
        keyword_ = _coll.get_value_as_string("keyword");
        country_ = _coll.get_value_as_string("country");
        std::string gen = _coll.get_value_as_string("gender");
        if (gen == "male")
            gender_ = profile::gender::male;
        else if (gen == "female")
            gender_ = profile::gender::female;
        age_.first = _coll.get_value_as_int("age_from");
        age_.second = _coll.get_value_as_int("age_to");
        count_ = _coll.get_value_as_int("count");
        skip_count_ = _coll.get_value_as_int("skip_count");
        online_only_ = _coll.get_value_as_bool("online_only");
    }

    bool search_params::get_online_only() const
    {
        return online_only_;
    }

    void search_params::set_online_only(bool _val)
    {
        online_only_ = _val;
    }
}