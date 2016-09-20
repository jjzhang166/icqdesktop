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
    push_back(task);
}

void main_thread::excute_core_context_priority(std::function<void()> task)
{
    push_front(task);
}

std::thread::id main_thread::get_core_thread_id() const
{
    assert(get_threads_ids().size() == 1);

    if (get_threads_ids().size() == 1)
    {
        return get_threads_ids()[0];
    }

    return std::thread::id(); // nobody
}
