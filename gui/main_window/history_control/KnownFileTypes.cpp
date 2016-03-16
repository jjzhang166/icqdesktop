#include "stdafx.h"

#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "KnownFileTypes.h"

namespace
{
	typedef std::map<QString, Themes::PixmapResourceId> Ext2IconMap;

	const Ext2IconMap& GetExt2IconMap();
}

namespace HistoryControl
{
	const Themes::IThemePixmapSptr& GetIconByFilename(const QString &filename)
	{
		assert(!filename.isEmpty());

		QFileInfo fi(filename);

		const auto &map = GetExt2IconMap();

		const auto iter = map.find(fi.suffix());
		if (iter == map.end())
		{
			return Themes::GetPixmap(Themes::PixmapResourceId::FileSharingFileTypeIconUnknown);
		}

		return Themes::GetPixmap(iter->second);
	}
}

namespace
{
	const Ext2IconMap& GetExt2IconMap()
	{
		static Ext2IconMap map;

		if (map.empty())
		{

		}

		return map;
	}
}