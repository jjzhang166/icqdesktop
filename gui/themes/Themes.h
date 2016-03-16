#pragma once

namespace Themes
{

    enum class ThemeId : int
	{
		Invalid,
		Min,

		Standard,
		Default = Standard,

		Max
	};

	const QString& GetThemeNameById(const ThemeId id);

	void SetCurrentThemeId(const ThemeId id);

	ThemeId GetCurrentThemeId();

}