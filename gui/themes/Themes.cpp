#include "stdafx.h"

#include "ThemePixmap.h"

#include "Themes.h"

using namespace Themes;

namespace
{
	auto CurrentThemeId_ = ThemeId::Invalid;
}

namespace Themes
{
	const QString& GetThemeNameById(const ThemeId id)
	{
		assert(id > ThemeId::Min);
		assert(id < ThemeId::Max);

		static std::unordered_map<int, QString> names;

		if (names.empty())
		{
			names.emplace((int)ThemeId::Standard, "standard");
		}

		return names[(int)id];

	}

	void SetCurrentThemeId(const ThemeId id)
	{
		assert(id > ThemeId::Min);
		assert(id < ThemeId::Max);

		if (CurrentThemeId_ == id)
		{
			return;
		}

		CurrentThemeId_ = id;
	}

	ThemeId GetCurrentThemeId()
	{
		assert(CurrentThemeId_ > ThemeId::Min);
		assert(CurrentThemeId_ < ThemeId::Max);

		return CurrentThemeId_;
	}
}

namespace
{

}