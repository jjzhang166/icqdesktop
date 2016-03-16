#ifndef __SCOPE_H_
#define __SCOPE_H_

#pragma once

namespace core
{
	namespace tools
	{
		class auto_scope
		{	
			std::function<void()> end_lambda_;

		public:

			auto_scope(std::function<void()> _lambda)
				: end_lambda_(_lambda)
			{
			}

			~auto_scope()
			{
				end_lambda_();
			}
		};

	}
}

#endif //__SCOPE_H_