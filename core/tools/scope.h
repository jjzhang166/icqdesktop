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

        class auto_scope_bool
        {
            std::function<void(const bool)> end_lambda_;

            bool success_;

        public:

            void set_success()
            {
                success_ = true;
            }

            auto_scope_bool(std::function<void(const bool)> _lambda)
                :   end_lambda_(_lambda),
                    success_(false)
            {
            }

            ~auto_scope_bool()
            {
                end_lambda_(success_);
            }
        };
    }
}

#endif //__SCOPE_H_