#include "../../utils/gui_coll_helper.h"

#pragma once

namespace Logic
{
	class address
	{
		QString		city_;
		QString		state_;
		QString		country_;

	public:
        address(const QString& _city, const QString& _state, const QString& _country): city_(_city), state_(_state), country_(_country)
        {
        }
        address() {}
        ~address() {}

		bool unserialize(Ui::gui_coll_helper _coll);

		const QString& get_city() const { return city_; }
		const QString& get_state() const { return state_; }
		const QString& get_country() const { return country_; }
	};

	class job
	{
		QString		industry_;
		QString		subindustry_;

	public:

		bool unserialize(Ui::gui_coll_helper _coll);
	};

	class email
	{
		QString		addr_;
		bool		hide_;
		bool		primary_;

	public:
		email() : hide_(false), primary_(false) {}

		bool unserialize(Ui::gui_coll_helper _coll);
	};

	typedef std::list<email>	email_list;

	class phone
	{
		QString		phone_;
		QString		type_;

	public:

		bool unserialize(Ui::gui_coll_helper _coll);
        
        const QString& get_phone() const { return phone_; }
        const QString& get_type() const { return type_; }
	};

	typedef std::list<phone>	phone_list;

	class interest
	{
		QString		code_;

	public:

		bool unserialize(Ui::gui_coll_helper _coll);
	};

	typedef std::list<interest>	interest_list;

	class contact_profile
	{
		QString			aimid_;
		QString			first_name_;
		QString			last_name_;
		QString			friendly_;
		QString			displayid_;
		QString			relationship_;
		QString			about_;
		int64_t			birthdate_;
		QString			gender_;
		address			home_address_;
		address			origin_address_;
		QString			privatekey_;
		email_list		emails_;
		phone_list		phones_;
		interest_list	interests_;
		int32_t			children_;
		QString			religion_;
		QString			sex_orientation_;
		bool			smoking_;

	public:

		contact_profile();
		virtual ~contact_profile();

		const QString& get_aimid() const;
		const QString& get_first_name() const;
		const QString& get_last_name() const;
		const QString& get_friendly() const;
		const QString& get_displayid() const;
		const QString& get_relationship() const;
		const QString& get_about() const;
		int64_t get_birthdate() const;
		const QString& get_gender() const;
		const address& get_home_address() const;
		const address& get_origin_address() const;
		const QString& get_privatekey() const;
		const email_list& get_emails() const;
		const phone_list& get_phones() const;
		const interest_list& get_interests() const;
		int32_t get_children() const;
		const QString& get_religion() const;
		const QString& get_sex_orientation() const;
		bool get_smoking() const;

		bool unserialize(Ui::gui_coll_helper _coll);
        bool unserialize2(Ui::gui_coll_helper _coll);

		QString get_contact_name() const;
	};

	typedef std::list<std::shared_ptr<contact_profile>>	profiles_list;
	typedef std::shared_ptr<contact_profile> profile_ptr;
}


