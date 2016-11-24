#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "GenericBlock.h"
#include "ComplexMessageItem.h"
#include "Style.h"

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

GenericBlockLayout::BoxModel::BoxModel(const bool hasLeadLines, const QMargins &bubbleMargins)
    : HasLeadLines_(hasLeadLines)
    , BubbleMargins_(bubbleMargins)
{
}

GenericBlockLayout::BoxModel::~BoxModel()
{
}

QMargins GenericBlockLayout::BoxModel::getBubbleMargins() const
{
    return BubbleMargins_;
}

bool GenericBlockLayout::BoxModel::hasLeadLines() const
{
    return HasLeadLines_;
}

GenericBlockLayout::GenericBlockLayout()
{
}

GenericBlockLayout::~GenericBlockLayout()
{

}

void GenericBlockLayout::addItem(QLayoutItem* /*item*/)
{
}

QLayoutItem* GenericBlockLayout::itemAt(int /*index*/) const
{
    return nullptr;
}

QLayoutItem* GenericBlockLayout::takeAt(int /*index*/)
{
    return nullptr;
}

int GenericBlockLayout::count() const
{
    return 0;
}

void GenericBlockLayout::invalidate()
{
    QLayoutItem::invalidate();
}

void GenericBlockLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    const auto isResize = (r.topLeft() == QPoint());
    if (isResize)
    {
        return;
    }

    const QRect internalRect(
        0, 0, r.width(), r.height());

    BlockSize_ = setBlockGeometryInternal(internalRect);
    if (BlockSize_ == QSize())
    {
        return;
    }

    assert(BlockSize_.height() >= 0);

    BlockGeometry_ = r;
    BlockGeometry_.setSize(BlockSize_);
    assert(BlockGeometry_.height() >= 0);
    assert(BlockGeometry_.width() > 0);
}

QSize GenericBlockLayout::sizeHint() const
{
    const auto blockHeight = BlockSize_.height();
    assert(blockHeight >= -1);

    const auto height = std::max(
        blockHeight,
        MessageStyle::getMinBubbleHeight());

    return QSize(-1, height);
}

QSize GenericBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth > 0);

    return QSize();
}

QSize GenericBlockLayout::blockSizeHint() const
{
    return sizeHint();
}

const IItemBlockLayout::IBoxModel& GenericBlockLayout::getBlockBoxModel() const
{
    static BoxModel boxModel(
        false,
        Style::getDefaultBlockBubbleMargins());

    return boxModel;
}

QRect GenericBlockLayout::getBlockGeometry() const
{
    return BlockGeometry_;
}

bool GenericBlockLayout::onBlockContentsChanged()
{
    const auto sizeBefore = BlockSize_;

    const auto &blockGeometry = getBlockGeometry();
    const auto noGeometry = ((blockGeometry.width() == 0) && (blockGeometry.height() == 0));
    if (noGeometry)
    {
        return false;
    }

    BlockSize_ = setBlockGeometryInternal(blockGeometry);

    const auto isSizeChanged = (sizeBefore != BlockSize_);
    if (isSizeChanged)
    {
        update();
    }

    return isSizeChanged;
}

QRect GenericBlockLayout::setBlockGeometry(const QRect &ltr)
{
    setGeometry(ltr);

    return QRect(
        ltr.left(),
        ltr.top(),
        BlockSize_.width(),
        BlockSize_.height());
}

UI_COMPLEX_MESSAGE_NS_END