#pragma once

#include "../../corelib/collection_helper.h"
#include "contact_profile.h"

namespace core
{
	class search_params
	{
		std::string						keyword_;
		profile::gender					gender_;
		std::string						country_;
		std::pair<int32_t, int32_t>		age_;
		int32_t							count_;
		int32_t							skip_count_;
		bool							online_only_;

	public:

		search_params();
		virtual ~search_params();

		const std::string& get_keyword() const;
		void set_keyword(const std::string& _val);

		profile::gender get_gender() const;
		void set_gender(profile::gender _val);

		const std::string& get_country() const;
		void set_country(const std::string& _val);

		const std::pair<int32_t, int32_t>& get_age() const;
		void set_age(const std::pair<int32_t, int32_t>& _age);

		int32_t get_count() const;
		void set_count(int32_t _val);

		int32_t get_skip_count() const;
		void set_skip_count(int32_t _val);

		bool get_online_only() const;
		void set_online_only(bool _val);

		void unserialize(core::coll_helper _coll);
	};

}


