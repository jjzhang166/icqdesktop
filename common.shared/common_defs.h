#ifndef __COMMON_DEFS_H_
#define __COMMON_DEFS_H_

namespace common
{
#ifdef _WIN32
    std::wstring get_user_profile();

    std::wstring get_product_data_path();
#endif //_WIN32
    struct core_gui_settings
    {
        core_gui_settings()
            : recents_avatars_size_(-1)
        {
        }

        core_gui_settings(int _recents_avatars_size)
            : recents_avatars_size_(_recents_avatars_size)
        {}

        int recents_avatars_size_;
    };
}

#endif // __COMMON_DEFS_H_