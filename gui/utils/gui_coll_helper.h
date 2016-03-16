#pragma once

#include "../../corelib/collection_helper.h"

namespace Ui
{
	class gui_coll_helper : public core::coll_helper
	{
	public:

		gui_coll_helper(core::icollection* _collection, bool _auto_release);

		void set_value_as_qstring(const char* _name, const QString& _value);
	};
}
