#include "stdafx.h"

#include "../utils/utils.h"

#include "DpiAwareImage.h"

namespace Ui
{

	DpiAwareImage::DpiAwareImage()
	{
	}

	DpiAwareImage::DpiAwareImage(const QImage &image)
		: Image_(image)
	{
	}

	DpiAwareImage::DpiAwareImage(const QImage &&image)
		: Image_(image)
	{
	}

	DpiAwareImage::DpiAwareImage(const QPixmap &pixmap)
		: Image_(pixmap.toImage())
	{
	}

	DpiAwareImage::DpiAwareImage(const QPixmap &&pixmap)
		: Image_(pixmap.toImage())
	{
	}

	DpiAwareImage::operator bool() const
	{
		return !isNull();
	}

	void DpiAwareImage::draw(QPainter &p, const int32_t x, const int32_t y) const
	{
		draw(p, QPoint(x, y));
	}

	void DpiAwareImage::draw(QPainter &p, const QPoint &coords) const
	{
		assert(!Image_.isNull());

#ifdef __APPLE__
        auto imageRect = Image_.rect();
        imageRect.moveTopLeft(coords);
        p.drawImage(coords, Image_);
#else
		auto imageRect = Utils::scale_bitmap(
			Image_.rect()
		);
        imageRect.moveTopLeft(coords);
        p.drawImage(imageRect, Image_);
#endif
	}

	bool DpiAwareImage::isNull() const
	{
		return Image_.isNull();
	}

	int32_t DpiAwareImage::width() const
	{
#ifdef __APPLE__
        return Image_.width() / Image_.devicePixelRatio();
#else
        return Image_.width();
#endif
	}

    int32_t DpiAwareImage::height() const
    {
#ifdef __APPLE__
        return Image_.height() / Image_.devicePixelRatio();
#else
        return Image_.height();
#endif
    }

}