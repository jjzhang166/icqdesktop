#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "ImagePreviewBlock.h"
#include "Style.h"

#include "ImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ImagePreviewBlockLayout::ImagePreviewBlockLayout()
{
}

ImagePreviewBlockLayout::~ImagePreviewBlockLayout()
{
}

QSize ImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth);

    const auto &block = *blockWidget<ImagePreviewBlock>();

    auto previewSize = (
        block.hasPreview() ?
            Utils::scale_value(block.getPreviewSize()) :
            Style::Preview::getImagePlaceholderSize());

    const auto maxSizeWidth = std::min(maxWidth, Style::Preview::getImageWidthMax());

    const QSize maxSize(
        maxSizeWidth,
        Style::Preview::getImageHeightMax());

    previewSize = limitSize(previewSize, maxSize);

    return previewSize;
}

void ImagePreviewBlockLayout::setActionButtonGeometry(const QRect &previewRect, ImagePreviewBlock &block)
{
    if (!block.hasActionButton())
    {
        return;
    }

    const auto actionButtonSize = block.getActionButtonSize();

    const auto minSizeToShowActionButton = (actionButtonSize * 2);

    const auto actionButtonVisible = (
        (previewRect.width() > minSizeToShowActionButton.width()) ||
        (previewRect.height() > minSizeToShowActionButton.height()));
    if (!actionButtonVisible)
    {
        block.hideActionButton();
        return;
    }

    QRect actionButtonRect(QPoint(0, 0), actionButtonSize);

    auto actionButtonCenter = PreviewRect_.center();

    const auto actionButtonCenteringOffsetY = (actionButtonRect.center().y() - block.getActionButtonLogicalCenter().y());
    actionButtonCenter.ry() += actionButtonCenteringOffsetY;

    actionButtonRect.moveCenter(actionButtonCenter);

    block.showActionButton(actionButtonRect);
}

QSize ImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &blockLtr)
{
    assert(blockLtr.width() > 0);

    auto &block = *blockWidget<ImagePreviewBlock>();

    setPreviewGeometry(blockLtr, block);

    setActionButtonGeometry(PreviewRect_, block);

    return PreviewRect_.size();
}

void ImagePreviewBlockLayout::setPreviewGeometry(const QRect &blockLtr, ImagePreviewBlock &block)
{
    assert(blockLtr.width() > 0);

    auto previewSize = (
        block.hasPreview() ?
            Utils::scale_value(block.getPreviewSize()) :
            Style::Preview::getImagePlaceholderSize());

    auto maxSizeWidth = std::min(blockLtr.width(), Style::Preview::getImageWidthMax());
    if (block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, block.getMaxPreviewWidth());

    const QSize maxSize(
        maxSizeWidth,
        Style::Preview::getImageHeightMax());

    previewSize = limitSize(previewSize, maxSize);

    PreviewRect_ = QRect(
        QPoint(0, 0),
        previewSize);
}

const QRect& ImagePreviewBlockLayout::getPreviewRect() const
{
    return PreviewRect_;
}

UI_COMPLEX_MESSAGE_NS_END