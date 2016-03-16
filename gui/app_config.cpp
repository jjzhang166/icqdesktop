#include "stdafx.h"

#include "../corelib/collection_helper.h"

#include "app_config.h"

namespace Ui
{

	namespace
	{
		AppConfigUptr AppConfig_;
	}

	AppConfig::AppConfig(const core::coll_helper &collection)
	{
		IsServerHistoryEnabled_ = collection.get_value_as_bool("history.is_server_history_enabled");
		ForcedDpi_ = collection.get_value_as_int("gui.forced_dpi", 0);
	}

	bool AppConfig::IsServerHistoryEnabled() const
	{
		return IsServerHistoryEnabled_;
	}

	int AppConfig::GetForcedDpi() const
	{
		assert(ForcedDpi_ >= 0);
		return ForcedDpi_;
	}

	const AppConfig& GetAppConfig()
	{
		assert(AppConfig_);

		return *AppConfig_;
	}

	void SetAppConfig(AppConfigUptr &appConfig)
	{
		assert(!AppConfig_);
		assert(appConfig);

		AppConfig_ = std::move(appConfig);
	}

}