#pragma once

namespace Themes
{
	enum class PixmapResourceId;

	enum class ThemeId;

	class IThemePixmap
	{
	public:
		virtual ~IThemePixmap() = 0;

		virtual void Draw(QPainter &p, const int32_t x, const int32_t y) const = 0;

		virtual void Draw(QPainter &p, const int32_t x, const int32_t y, const int32_t w, const int32_t h) const = 0;

		virtual void Draw(QPainter &p, const QRect &r) const = 0;

		virtual int32_t GetWidth() const = 0;

		virtual int32_t GetHeight() const = 0;

		virtual QRect GetRect() const = 0;

        virtual QSize GetSize() const = 0;

	};

	typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;

	const IThemePixmapSptr& GetPixmap(const PixmapResourceId id);
}