#ifndef __MAINTHREAD_H__
#define __MAINTHREAD_H__

#pragma once

#include "tools/threadpool.h"

namespace core
{
	class main_thread : protected core::tools::threadpool
	{
	public:

		main_thread();
		virtual ~main_thread();

		void excute_core_context(std::function<void()> task);
	};
}


#endif //__MAINTHREAD_H__