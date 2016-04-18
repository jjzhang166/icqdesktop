#ifndef __CORE_SETTINGS_H_
#define __CORE_SETTINGS_H_

#pragma once

#include "tools/settings.h"
#include "proxy_settings.h"

namespace core
{
    enum core_settings_values
    {
        min = 0,

        csv_device_id = 1,
        core_settings_proxy = 2,
        
        max
    };

	class core_settings : public core::tools::settings
	{
		std::wstring	file_name_;

	public:
		core_settings(const boost::filesystem::wpath& _path);
		virtual ~core_settings();
        
		bool save();
		bool load();

		void init_default();

        void set_user_proxy_settings(const proxy_settings& _user_proxy_settings);
        proxy_settings get_user_proxy_settings();
	};

}

#endif //__CORE_SETTINGS_H_