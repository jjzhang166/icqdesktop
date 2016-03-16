#include "stdafx.h"
#include "search_params.h"

namespace Ui
{
	search_params::search_params(void)
		:	gender_(Gender::unknown),
			age_(-1, -1),
			count_(20),
			skip_count_(0),
			online_only_(false)
	{
	}


	search_params::~search_params(void)
	{
	}

	const QString& search_params::get_keyword() const
	{
		return keyword_;
	}

	void search_params::set_keyword(const QString& _val)
	{
		keyword_ = _val;
	}

	Gender search_params::get_gender() const
	{
		return gender_;
	}

	void search_params::set_gender(Gender _val)
	{
		gender_ = _val;
	}

	const QString& search_params::get_country() const
	{
		return country_;
	}

	void search_params::set_country(const QString& _val)
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
		_coll.set_value_as_int("count", count_);
		_coll.set_value_as_int("skip_count", skip_count_);
		_coll.set_value_as_bool("online_only", online_only_);
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

	bool search_params::get_online_only() const
	{
		return online_only_;
	}

	void search_params::set_online_only(bool _val)
	{
		online_only_ = _val;
	}
    
}

