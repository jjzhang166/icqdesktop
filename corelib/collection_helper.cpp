#include "stdafx.h"

#include "remote_proc.h"

#include "collection_helper.h"

namespace core
{

	void coll_helper::set_value_as_proc(const char *_name, const remote_proc_t _proc)
	{
		assert(_name);
		assert(::strlen(_name) > 0);
		assert(_proc);

		//remote_proc_t::create(_callback);
	}

}