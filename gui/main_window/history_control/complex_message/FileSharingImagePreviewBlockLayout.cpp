#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "FileSharingBlock.h"
#include "Style.h"

#include "FileSharingImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int32_t getAuthorAvatarBottomMargin();

    int32_t getAuthorAvatarLeft(const bool isStandalone);

    QSize getAuthorAvatarSizeInLayout();

    int32_t getAuthorNickBaseline();

    int32_t getAuthorNickLeftMargin();
}

FileSharingImagePreviewBlockLayout::FileSharingImagePreviewBlockLayout()
{

}

FileSharingImagePreviewBlockLayout::~FileSharingImagePreviewBlockLayout()
{

}

QSize FileSharingImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    auto &block = *blockWidget<FileSharingBlock>();

    auto previewSize = (
        block.isFailedSnap() ?
            block.getFailedSnapSizeMax() :
            block.getOriginalPreviewSize());

    const auto maxSizeWidth = std::min(maxWidth, Style::getImageWidthMax());
    const QSize maxSize(maxSizeWidth, Style::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = Ui::MessageStyle::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QSize blockSize(previewSize);

    if (block.isAuthorVisible())
    {
        AuthorAvatarRect_ = evaluateAuthorAvatarRect(block.isStandalone());

        auto authorHeight = getAuthorAvatarRect().bottom();
        authorHeight += getAuthorAvatarBottomMargin();

        blockSize.rheight() += authorHeight;
    }

    return blockSize;
}

QRect FileSharingImagePreviewBlockLayout::getAuthorAvatarRect() const
{
    return AuthorAvatarRect_;
}

int32_t FileSharingImagePreviewBlockLayout::getAuthorAvatarSize() const
{
    return Utils::scale_value(24);
}

QRect FileSharingImagePreviewBlockLayout::getAuthorNickRect() const
{
    return AuthorNickRect_;
}

const QRect& FileSharingImagePreviewBlockLayout::getContentRect() const
{
    return PreviewRect_;
}

QFont FileSharingImagePreviewBlockLayout::getFilenameFont() const
{
    return QFont();
}

const QRect& FileSharingImagePreviewBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QFont FileSharingImagePreviewBlockLayout::getFileSizeFont() const
{
    return QFont();
}

QRect FileSharingImagePreviewBlockLayout::getFileSizeRect() const
{
    return QRect();
}

QFont FileSharingImagePreviewBlockLayout::getShowInDirLinkFont() const
{
    return QFont();
}

QRect FileSharingImagePreviewBlockLayout::getShowInDirLinkRect() const
{
    return QRect();
}

const IItemBlockLayout::IBoxModel& FileSharingImagePreviewBlockLayout::getBlockBoxModel() const
{
    static const QMargins margins(
        Utils::scale_value(16),
        Utils::scale_value(12),
        Utils::scale_value(8),
        Utils::scale_value(16));

    static const BoxModel boxModel(
        true,
        margins);

    return boxModel;
}

QSize FileSharingImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<FileSharingBlock>();

    if (block.isAuthorVisible())
    {
        AuthorAvatarRect_ = evaluateAuthorAvatarRect(block.isStandalone());

        AuthorNickRect_ = evaluateAuthorNickRect(block.isStandalone(), AuthorAvatarRect_, geometry.width());

        block.setAuthorNickGeometry(AuthorNickRect_);
    }

    PreviewRect_ = evaluatePreviewRect(block, geometry.width());

    setCtrlButtonGeometry(block, PreviewRect_);

    auto blockSize = PreviewRect_.size();

    if (block.isAuthorVisible())
    {
        blockSize.rheight() += AuthorAvatarRect_.bottom();
        blockSize.rheight() += getAuthorAvatarBottomMargin();
    }

    return blockSize;
}

QRect FileSharingImagePreviewBlockLayout::evaluateAuthorAvatarRect(const bool isStandalone) const
{
    const auto top = (isStandalone ? getAuthorAvatarLeft(isStandalone) : 0);

    const QPoint leftTop(getAuthorAvatarLeft(isStandalone), top);

    QRect result(leftTop, getAuthorAvatarSizeInLayout());

    return result;
}

QRect FileSharingImagePreviewBlockLayout::evaluateAuthorNickRect(const bool isStandalone, const QRect &authorAvatarRect, const int32_t blockWidth) const
{
    assert(authorAvatarRect.isValid());
    assert(blockWidth > 0);

    auto nickLeft = authorAvatarRect.right();
    nickLeft += getAuthorNickLeftMargin();

    const auto nickRight = blockWidth;

    const auto nickTop = (isStandalone ? Utils::scale_value(12) : 0);

    QRect result(
        nickLeft, nickTop,
        0, getAuthorNickBaseline());
    result.setRight(nickRight);

    return result;
}

QRect FileSharingImagePreviewBlockLayout::evaluatePreviewRect(const FileSharingBlock &block, const int32_t blockWidth) const
{
    assert(blockWidth > 0);

    auto previewSize = (
        block.isFailedSnap() ?
            block.getFailedSnapSizeMax() :
            block.getOriginalPreviewSize());

    auto maxSizeWidth = std::min(blockWidth, Style::getImageWidthMax());
    if (block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, block.getMaxPreviewWidth());
    const QSize maxSize(maxSizeWidth, Style::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = MessageStyle::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QPoint leftTop;

    if (block.isAuthorVisible())
    {
        const auto authorHeight = (
            AuthorAvatarRect_.bottom() +
            getAuthorAvatarBottomMargin());

        leftTop.ry() = authorHeight;
    }

    return QRect(leftTop, previewSize);
}

void FileSharingImagePreviewBlockLayout::setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect)
{
    assert(!previewRect.isEmpty());
    if (previewRect.isEmpty())
    {
        return;
    }

    if (block.isFailedSnap())
    {
        return;
    }

    const auto buttonSize = block.getCtrlButtonSize();

    assert(!buttonSize.isEmpty());
    if (buttonSize.isEmpty())
    {
        return;
    }

    QRect buttonRect(QPoint(), buttonSize);

    buttonRect.moveCenter(previewRect.center());

    block.setCtrlButtonGeometry(buttonRect);
}

namespace
{
    int32_t getAuthorAvatarBottomMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t getAuthorAvatarLeft(const bool isStandalone)
    {
        if (!isStandalone)
        {
            return 0;
        }

        return Utils::scale_value(12);
    }

    QSize getAuthorAvatarSizeInLayout()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    int32_t getAuthorNickBaseline()
    {
        return Utils::scale_value(30);
    }

    int32_t getAuthorNickLeftMargin()
    {
        return Utils::scale_value(10);
    }
}

UI_COMPLEX_MESSAGE_NS_END