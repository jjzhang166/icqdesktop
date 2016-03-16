#ifndef __WINCE_H__
#define __WINCE_H__

#include <winsock.h>

#if _WIN32_WCE >= 0x400
#include <time.h>
#include <assert.h>
#else
#include "wince_time.h"
#define assert(x) 
#endif

#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xFFFFFFFF
#endif

#define getenv(x) 0
int abort();

#endif
