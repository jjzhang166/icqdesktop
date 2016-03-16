#pragma once

namespace core
{
	class coll_helper;
}

namespace Ui
{
	class AppConfig
	{
	public:
		AppConfig(const core::coll_helper &collection);

		bool IsServerHistoryEnabled() const;

		int GetForcedDpi() const;

	private:
		bool IsServerHistoryEnabled_;

		int ForcedDpi_;
	};

	typedef std::unique_ptr<AppConfig> AppConfigUptr;

	const AppConfig& GetAppConfig();

	void SetAppConfig(AppConfigUptr &appConfig);
}