#include "stdafx.h"
#include "my_info.h"
#include "core_dispatcher.h"

namespace Ui
{
	my_info::my_info()
		: flags_(0)
	{

	}

	void my_info::unserialize(core::coll_helper* _collection)
	{
		aimId_ = _collection->get_value_as_string("aimId");
		displayId_ = _collection->get_value_as_string("displayId");
		friendlyName_ = _collection->get_value_as_string("friendly");
		state_ = _collection->get_value_as_string("state");
		userType_ = _collection->get_value_as_string("userType");
		phoneNumber_ = _collection->get_value_as_string("attachedPhoneNumber");
		flags_ = _collection->get_value_as_uint("globalFlags");

		emit received();
	}

	my_info* MyInfo()
	{
		static std::unique_ptr<my_info> info(new my_info());
		return info.get();
	}

}
