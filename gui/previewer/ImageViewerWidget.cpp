#include "stdafx.h"

#include "../utils/utils.h"

#include "Bicubic.h"

#include "ImageViewerWidget.h"

namespace Previewer
{
    class AbstractViewer
    {
    public:
        explicit AbstractViewer(QLabel* _label)
            : label_(_label)
        {
        }

        virtual ~AbstractViewer()
        {
        }

        QRect rect() const
        {
            const auto widgetRect = label_->contentsRect();
            auto resultRect = imageRect();
            resultRect.moveCenter(widgetRect.center());
            return resultRect;
        }

        QSize getPrefferedImageSize(const QSize& imageSize) const
        {
            const auto widgetSize = label_->size();
            const auto size = Utils::scale_value(imageSize);

            bool needToDownscale = (widgetSize.width() < size.width() || widgetSize.height() < size.height());
            if (!needToDownscale)
            {
                return Utils::scale_bitmap(size);
            }

            const auto kx = size.width() / static_cast<double>(widgetSize.width());
            const auto ky = size.height() / static_cast<double>(widgetSize.height());
            const auto k = std::max(kx, ky);
            const auto scaledSize = QSize(size.width() / k, size.height() / k);
            return Utils::scale_bitmap(scaledSize);
        }

    private:
        virtual QRect imageRect() const = 0;

    protected:
        QLabel* label_;
    };

    class GifViewer
        : public AbstractViewer
    {
    public:
        GifViewer(const QString& _fileName, QLabel* _label)
            : AbstractViewer(_label)
        {
            gif_.reset(new QMovie(_fileName));
            gif_->start();

            const auto frameSize = gif_->frameRect().size();
            const auto prefferedSize = getPrefferedImageSize(frameSize);

            if (prefferedSize != frameSize)
                gif_->setScaledSize(prefferedSize);

            label_->setMovie(gif_.get());
        }

    private:
        QRect imageRect() const override
        {
            return gif_->frameRect();
        }

    private:
        std::unique_ptr<QMovie> gif_;
    };

    class JpegPngViewer
        : public AbstractViewer
    {
    public:
        JpegPngViewer(const QPixmap& _image, QLabel* _label)
            : AbstractViewer(_label)
            , originalImage_(_image)
        {
            const auto imageSize = originalImage_.size();
            const auto prefferedSize = getPrefferedImageSize(imageSize);

            if (prefferedSize != imageSize)
            {
                auto scaledImage = scaleBicubic(originalImage_.toImage(), prefferedSize);
                auto scaledPixmap = QPixmap::fromImage(scaledImage);
                Utils::check_pixel_ratio(scaledPixmap);
                label_->setPixmap(scaledPixmap);
                imageRect_ = scaledImage.rect();
            }
            else
            {
                label_->setPixmap(originalImage_);
                imageRect_ = originalImage_.rect();
            }
        }

    private:
        QRect imageRect() const override
        {
            return imageRect_;
        }

    private:
        QPixmap originalImage_;
        QRect imageRect_;
    };
}

Previewer::ImageViewerWidget::ImageViewerWidget(QWidget* _parent)
    : QLabel(_parent)
{
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Previewer::ImageViewerWidget::~ImageViewerWidget()
{
}

void Previewer::ImageViewerWidget::showImage(const QPixmap& _preview, const QString& _fileName)
{
    assert(!_fileName.isEmpty());

    auto reader = std::unique_ptr<QImageReader>(new QImageReader(_fileName));

    QString type = reader->format();

    if (type.isEmpty()) // extension does not match the content
    {
        reader.reset(new QImageReader(_fileName));
        reader->setDecideFormatFromContent(true);
        type = reader->format();
    }

    clear();

    if (type == "gif")
    {
        viewer_.reset(new GifViewer(_fileName, this));
    }
    else if (type == "jpeg" || type == "png" || type == "bmp")
    {
        if (_preview.isNull())
        {
            QPixmap pixmap;
            Utils::loadPixmap(_fileName, pixmap);
            viewer_.reset(new JpegPngViewer(pixmap, this));
        }
        else
            viewer_.reset(new JpegPngViewer(_preview, this));
    }
    else
    {
        assert(!"unknown format");
    }
}

void Previewer::ImageViewerWidget::reset()
{
    viewer_.reset();
    clear();
}

void Previewer::ImageViewerWidget::mousePressEvent(QMouseEvent* _event)
{
    if (!viewer_)
        return;

    if (_event->button() != Qt::LeftButton)
    {
        _event->ignore();
        return;
    }

    const auto pos = _event->pos();
    const auto rect = viewer_->rect();
    if (rect.contains(pos))
    {
        emit imageClicked();
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}
