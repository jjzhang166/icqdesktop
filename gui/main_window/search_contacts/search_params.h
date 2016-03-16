#pragma once

#include "../../utils/gui_coll_helper.h"

namespace Ui
{
	enum Gender
	{
		unknown		= 0,
		male		= 1,
		female		= 2
	};

	class search_params
	{
		QString							keyword_;
		Gender							gender_;
		QString							country_;
		std::pair<int32_t, int32_t>		age_;
		int32_t							count_;
		int32_t							skip_count_;
		bool							online_only_;
        
	public:

		search_params();
		virtual ~search_params();

		const QString& get_keyword() const;
		void set_keyword(const QString& _val);

		Gender get_gender() const;
		void set_gender(Gender _val);

		const QString& get_country() const;
		void set_country(const QString& _val);

		const std::pair<int32_t, int32_t>& get_age() const;
		void set_age(const std::pair<int32_t, int32_t>& _age);

		int32_t get_count() const;
		void set_count(int32_t _val);

		bool get_online_only() const;
		void set_online_only(bool _val);
        
		int32_t get_skip_count() const;
		void set_skip_count(int32_t _val);


		void serialize(Ui::gui_coll_helper _coll) const;
	};

}

