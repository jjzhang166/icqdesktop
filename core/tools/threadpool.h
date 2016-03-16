#pragma once

#include "semaphore.h"

namespace core
{
	namespace tools
	{
		class threadpool : boost::noncopyable
		{
			boost::thread::id		creator_thread_id_;

		public:

			typedef std::function<void()> task;

            explicit threadpool(const unsigned _count, std::function<void()> _on_thread_exit = [](){});

			virtual ~threadpool();

			bool enqueue(const task _task);

		private:
			std::vector<std::thread>			threads_;
			std::mutex							queue_mutex_;
			std::condition_variable				condition_;
			std::queue<task>					tasks_;
			std::atomic<bool>					stop_;

		};
	}
	
}