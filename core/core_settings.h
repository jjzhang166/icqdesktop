#ifndef __CORE_SETTINGS_H_
#define __CORE_SETTINGS_H_

#pragma once

#include "tools/settings.h"

namespace core
{
	enum core_settings_values
	{
		csv_device_id	= 1
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
	};

}

#endif //__CORE_SETTINGS_H_