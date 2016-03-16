#include "stdafx.h"
#include "gui_coll_helper.h"

namespace Ui
{
	gui_coll_helper::gui_coll_helper(core::icollection* _collection, bool auto_release)
		: core::coll_helper(_collection, auto_release)
	{

	}

	void gui_coll_helper::set_value_as_qstring(const char* _name, const QString& _value)
	{
		auto value = _value.toUtf8();

		return core::coll_helper::set_value_as_string(_name, value.data(), value.size());
	}

}
