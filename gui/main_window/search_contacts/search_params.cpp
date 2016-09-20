#include "stdafx.h"
#include "search_params.h"

namespace Ui
{
	search_params::search_params(void)
		:	gender_(Gender::unknown),
			age_(-1, -1),
			onlineOnly_(false)
	{
	}

	search_params::~search_params(void)
	{
	}

	const QString& search_params::getKeyword() const
	{
		return keyword_;
	}

	void search_params::setKeyword(const QString& _val)
	{
		keyword_ = _val;
	}

	Gender search_params::getGender() const
	{
		return gender_;
	}

	void search_params::setGender(Gender _val)
	{
		gender_ = _val;
	}

	const QString& search_params::getCountry() const
	{
		return country_;
	}

	void search_params::setCountry(const QString& _val)
	{
		country_ = _val;
	}

	const std::pair<int32_t, int32_t>& search_params::getAge() const
	{
		return age_;
	}

	void search_params::setAge(const std::pair<int32_t, int32_t>& _age)
	{
		age_ = _age;
	}

	void search_params::serialize(Ui::gui_coll_helper _coll) const
	{
		_coll.set_value_as_qstring("keyword", keyword_);
		std::string gender;
		if (gender_ == Gender::male)
			gender = "male";
		else if (gender_ == Gender::female)
			gender = "female";
		_coll.set_value_as_string("gender", gender);
		_coll.set_value_as_qstring("country", country_);
		_coll.set_value_as_int("age_from", age_.first);
		_coll.set_value_as_int("age_to", age_.second);
		_coll.set_value_as_qstring("next_tag", nextTag_);
		_coll.set_value_as_bool("online_only", onlineOnly_);
	}

	bool search_params::getOnlineOnly() const
	{
		return onlineOnly_;
	}

	void search_params::setOnlineOnly(bool _val)
	{
        onlineOnly_ = _val;
	}
    
}

