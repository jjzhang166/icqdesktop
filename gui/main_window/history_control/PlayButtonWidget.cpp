#include "stdafx.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "PlayButtonWidget.h"

namespace Ui
{
    PlayButtonWidget::PlayButtonWidget()
        : Progress_(0)
        , ResourcesInitialized_(false)
    {
        initResources();
    }

    PlayButtonWidget::~PlayButtonWidget()
    {

    }

    void PlayButtonWidget::setProgress(const double progress)
    {
        assert(progress >= 0);
        assert(progress <= (1.0 + DBL_EPSILON));

        const auto isProgressChanged = (std::abs(Progress_ - progress) > DBL_EPSILON);

        Progress_ = progress;

        if (isProgressChanged)
        {
            update();
        }
    }

    QSize PlayButtonWidget::sizeHint() const
    {
        assert(StartButtonImage_);
        return StartButtonImage_->GetSize();
    }

    void PlayButtonWidget::mouseMoveEvent(QMouseEvent *event)
    {
        event;
    }

    void PlayButtonWidget::mouseReleaseEvent(QMouseEvent *event)
    {
        event;
    }

    void PlayButtonWidget::paintEvent(QPaintEvent *event)
    {
        event;
    }

    void PlayButtonWidget::initResources()
    {
        if (ResourcesInitialized_)
        {
            return;
        }

        ResourcesInitialized_ = true;

        using namespace Themes;

        StartButtonImage_ = GetPixmap(PixmapResourceId::FileSharingMediaPlay);
        StartButtonImageHover_ = GetPixmap(PixmapResourceId::FileSharingMediaPlayHover);
        StartButtonImageActive_ = GetPixmap(PixmapResourceId::FileSharingMediaPlayActive);

        StopButtonImage_ = GetPixmap(PixmapResourceId::FileSharingMediaCancel);
        StopButtonImageHover_ = GetPixmap(PixmapResourceId::FileSharingMediaCancel);
        StopButtonImageActive_ = GetPixmap(PixmapResourceId::FileSharingMediaCancel);
    }
}