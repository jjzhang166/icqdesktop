#pragma once

namespace Ui
{

	class DpiAwareImage
	{
	public:
		DpiAwareImage();

		explicit DpiAwareImage(const QImage& _image);

		explicit DpiAwareImage(const QImage&& _image);

		explicit DpiAwareImage(const QPixmap& _pixmap);

		explicit DpiAwareImage(const QPixmap&& _pixmap);

		operator bool() const;

		void draw(QPainter& _p, const int32_t _x, const int32_t _y) const;

		void draw(QPainter& _p, const QPoint& _coords) const;

		int32_t height() const;

		bool isNull() const;

		int32_t width() const;

	private:
		QImage Image_;

	};

	static_assert(std::is_move_constructible<DpiAwareImage>::value, "DpiAwareImage must be move constructible");
	static_assert(std::is_move_assignable<DpiAwareImage>::value, "DpiAwareImage must be move assignable");

}