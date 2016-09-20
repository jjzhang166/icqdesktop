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
        QString                         nextTag_;
		bool							onlineOnly_;
        
	public:

		search_params();
		virtual ~search_params();

		const QString& getKeyword() const;
		void setKeyword(const QString& _val);

		Gender getGender() const;
		void setGender(Gender _val);

		const QString& getCountry() const;
		void setCountry(const QString& _val);

		const std::pair<int32_t, int32_t>& getAge() const;
		void setAge(const std::pair<int32_t, int32_t>& _age);

		bool getOnlineOnly() const;
		void setOnlineOnly(bool _val);
        
        inline const QString &getNextTag() const { return nextTag_; }
        inline void setNextTag(const QString &_val) { nextTag_ = _val; }

		void serialize(Ui::gui_coll_helper _coll) const;
	};

}

