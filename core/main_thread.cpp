#include "stdafx.h"
#include "main_thread.h"

using namespace core;

main_thread::main_thread()
	:	threadpool( 1 )
{
}


main_thread::~main_thread()
{
}

void main_thread::excute_core_context(std::function<void()> task)
{
	enqueue(task);
}
