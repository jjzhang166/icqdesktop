#include "stdafx.h"
#include "../utils/utils.h"
#include "BackgroundWidget.h"

namespace Ui
{
    BackgroundWidget::BackgroundWidget(QWidget* _parent, const QString& _imageName)
    : QStackedWidget(_parent), currentSize_(QSize()), tiling_(false)
    {
        if (_imageName.length() > 0)
        {
            pixmapToDraw_ = QPixmap(Utils::parse_image_name(_imageName));
        }
    };

    void BackgroundWidget::paintEvent(QPaintEvent *_e)
    {
        QWidget::paintEvent(_e);
        
        if (!pixmapToDraw_.isNull())
        {
            Utils::check_pixel_ratio(pixmapToDraw_);
            
            QSize pixmapSize = pixmapToDraw_.size();
            pixmapSize.setWidth(pixmapSize.width() / Utils::scale_bitmap(1));
            pixmapSize.setHeight(pixmapSize.height() / Utils::scale_bitmap(1));
            
            float yOffset = -(pixmapSize.height() - currentSize_.height()) / 2;
            float xOffset = -(pixmapSize.width() - currentSize_.width()) / 2;
            if (!tiling_)
            {
                float pixmapSidesRatio = 1. * pixmapSize.width() / pixmapSize.height();
                float screenRatio = 1. * currentSize_.width() / currentSize_.height();
                if ((currentSize_.width() >= pixmapSize.width() && currentSize_.height() >= pixmapSize.height()) ||
                    (currentSize_.width() < pixmapSize.width() && currentSize_.height() < pixmapSize.height())
                    )
                {
                    if (pixmapSidesRatio < screenRatio)
                    {
                        int width = currentSize_.width();
                        int height = width / pixmapSidesRatio;
                        yOffset = -(height - currentSize_.height()) / 2;
                        drawPixmap(0, yOffset, width, height, pixmapToDraw_);
                    }
                    else
                    {
                        int height = currentSize_.height();
                        int width = height * pixmapSidesRatio;
                        xOffset = -(width - currentSize_.width()) / 2;
                        drawPixmap(xOffset, 0, width, height, pixmapToDraw_);
                    }
                }
                if (currentSize_.width() > pixmapSize.width() && currentSize_.height() <= pixmapSize.height())
                {
                    float height = currentSize_.width() / pixmapSidesRatio;
                    float width = currentSize_.width();
                    yOffset = -(height - currentSize_.height())/2;
                    drawPixmap(0, yOffset, width, height, pixmapToDraw_);
                }
                else if (currentSize_.width() <= pixmapSize.width() && currentSize_.height() > pixmapSize.height())
                {
                    float height = currentSize_.height();
                    float width = currentSize_.height() * pixmapSidesRatio;
                    xOffset = -(width - currentSize_.width()) / 2;
                    drawPixmap(xOffset, 0, width, height, pixmapToDraw_);
                }
            }
            else
            {
                double ratio = Utils::scale_bitmap(1);
                xOffset = 0;
                yOffset = 0;
                for (int x = 0; x < this->rect().width(); x += pixmapToDraw_.width()/ratio)
                {
                    for (int y = 0; y < this->rect().height(); y += pixmapToDraw_.height()/ratio)
                    {
                        drawPixmap(x + xOffset, y + yOffset, pixmapSize.width(), pixmapSize.height(), pixmapToDraw_);
                    }
                }
            }
        }
    }
    
    bool BackgroundWidget::cachedPixmapParams::cacheAndCheckIfChanged(int _x, int _y, int _w, int _h)
    {
        bool changed = false;
        if (x_ != _x || y_ != _y || w_ != _w || h_ != _h)
        {
            changed = true;
        }
        x_ = _x; y_ = _y; w_ = _w; h_ = _h;
        return changed;
    }
    
    void BackgroundWidget::drawPixmap(int x, int y, int w, int h, const QPixmap& _pm)
    {
        QPainter painter(this);
        bool changed = cachedPixmapParams_.cacheAndCheckIfChanged(x, y, w, h);
        if (changed)
        {
            QPixmap scaledPixmap = _pm.scaled(w, h, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
            cachedPixmap_ = scaledPixmap;
            painter.drawPixmap(x, y, w, h, scaledPixmap);
        }
        else
        {
            painter.drawPixmap(x, y, w, h, cachedPixmap_);
        }
    }
    
    void BackgroundWidget::resizeEvent(QResizeEvent *_event)
    {
        QWidget::resizeEvent(_event);
        QSize size = _event->size();
        currentSize_ = size;
    }
    
    void BackgroundWidget::setImage(const QPixmap& _pixmap, const bool _tiling)
    {
        if (!_pixmap.isNull())
        {
            pixmapToDraw_ = _pixmap;
            tiling_ = _tiling;
            cachedPixmapParams_.invalidate();
            update();
        }
    }
}