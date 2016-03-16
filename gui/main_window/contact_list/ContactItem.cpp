#include "stdafx.h"
#include "ContactItem.h"
#include "contact_profile.h"

namespace Logic
{
	ContactItem::ContactItem(Data::Contact *contact)
		: contact_(contact)
		, visible_(true)
	{
	}

	bool ContactItem::operator== (const ContactItem& other) const
	{
		return contact_->AimId_ == other.Get()->AimId_;
	}

	bool ContactItem::is_visible() const
	{
		return visible_;
	}

	void ContactItem::set_visible(bool visible)
	{
		visible_ = visible;
	}

	bool ContactItem::is_not_auth() const
	{
		return contact_->NotAuth_;
	}

	void ContactItem::reset_not_auth()
	{
		contact_->NotAuth_ = false;
	}

	Data::Contact* ContactItem::Get() const
	{
		return contact_.get();
	}

	bool ContactItem::is_group() const
	{
		return contact_->GetType() == Data::GROUP;
	}

	bool ContactItem::is_online() const
	{
		return contact_->HaveLastSeen_ && !contact_->LastSeen_.isValid();
	}

	bool ContactItem::is_phone() const
	{
		return contact_->UserType_ == "sms" || contact_->UserType_ == "phone";
	}

	bool ContactItem::recently() const
	{
		return contact_->LastSeen_.isValid() && contact_->LastSeen_.daysTo(QDateTime::currentDateTime()) <= 180;
	}

	bool ContactItem::is_chat() const
	{
		return contact_->Is_chat_;
	}

	bool ContactItem::is_muted() const
	{
		return contact_->Muted_;
	}

	bool ContactItem::is_checked() const
	{
		return contact_->IsChecked_;
	}

	void ContactItem::set_checked(bool _is_checked)
	{
		contact_->IsChecked_ = _is_checked;
	}

	void ContactItem::set_input_text(const QString& _input_text)
	{
		input_text_ = _input_text;
	}

	QString ContactItem::get_input_text() const
	{
		return input_text_;
	}

	void ContactItem::set_contact_profile(profile_ptr _profile)
	{
		profile_ = _profile;
	}

	profile_ptr ContactItem::get_contact_profile() const
	{
		return profile_;
	}

	QString ContactItem::get_aimid() const
	{
		return contact_->AimId_;
	}
}