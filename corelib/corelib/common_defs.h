#ifndef __COMMON_DEFS_H_
#define __COMMON_DEFS_H_

#ifdef _WIN32
namespace common
{
    std::wstring get_user_profile();

    std::wstring get_product_data_path();
}
#endif // _WIN32

namespace common
{
    unsigned int get_limit_search_results();
}
#endif // __COMMON_DEFS_H_