#pragma once

namespace Ui
{

	class DpiAwareImage
	{
	public:
		DpiAwareImage();

		explicit DpiAwareImage(const QImage &image);

		explicit DpiAwareImage(const QImage &&image);

		explicit DpiAwareImage(const QPixmap &pixmap);

		explicit DpiAwareImage(const QPixmap &&pixmap);

		operator bool() const;

		void draw(QPainter &p, const int32_t x, const int32_t y) const;

		void draw(QPainter &p, const QPoint &coords) const;

		int32_t height() const;

		bool isNull() const;

		int32_t width() const;

	private:
		QImage Image_;

	};

	static_assert(std::is_move_constructible<DpiAwareImage>::value, "DpiAwareImage must be move constructible");
	static_assert(std::is_move_assignable<DpiAwareImage>::value, "DpiAwareImage must be move assignable");

}