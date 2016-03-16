// corelib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#ifdef __linux__
#include "corelib.h"
#else
#include "core_instance.h"
#endif //__linux

#ifdef __APPLE__
extern "C"
#endif
bool get_core_instance(core::icore_interface** _core)
{
	*_core = new core::core_instance();

    return true;
}

