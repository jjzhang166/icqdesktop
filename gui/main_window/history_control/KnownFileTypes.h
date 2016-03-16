#pragma once

namespace Themes
{
	class IThemePixmap;
}

namespace HistoryControl
{
	const Themes::IThemePixmapSptr& GetIconByFilename(const QString &filename);
}