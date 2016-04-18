#ifndef __ASYNC_TASK_H_
#define __ASYNC_TASK_H_

#pragma once

#include "tools/threadpool.h"

namespace core
{
    class ithread_callback
    {
    public:
        ithread_callback() {}
        virtual ~ithread_callback() {}
        virtual void on_thread_shutdown() = 0;
    };

    class async_task
    {
    public:
        async_task();
        virtual ~async_task();

        virtual int32_t execute() = 0;
    };

    struct async_task_handlers
    {
        std::function<void(int32_t)>	on_result_;

        async_task_handlers()
        {
            on_result_ = [](int32_t){};
        }
    };


    class auto_callback : private boost::noncopyable
    {
        std::function<void(int32_t)> callback_;

    public:

        auto_callback(std::function<void(int32_t)> _call_back)
            :   callback_(_call_back)
        {

        }

        ~auto_callback()
        {
            assert(!callback_); // callback must be nullptr
            if (callback_)
                callback_(-1);
        }

        void callback(int32_t _error)
        {
            callback_(_error);
            callback_ = nullptr;
        }
    };

    typedef std::shared_ptr<auto_callback> auto_callback_sptr;

    class async_executer : core::tools::threadpool
    {
    public:
        async_executer(unsigned long _count = 1);
        virtual ~async_executer();

        virtual std::shared_ptr<async_task_handlers> run_async_task(std::shared_ptr<async_task> task);
        virtual std::shared_ptr<async_task_handlers> run_async_function(std::function<int32_t()> func);
    };

}


#endif //__ASYNC_TASK_H_