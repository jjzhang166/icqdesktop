#include "stdafx.h"
#include "../common.shared/crash_handler.h"

#ifndef _WIN32
extern "C"
#endif
    void init_crash_handlers()
    {
        core::dump::crash_handler ch;
        ch.set_process_exception_handlers();
        ch.set_thread_exception_handlers();
    }