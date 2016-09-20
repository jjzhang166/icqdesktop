#include "stdafx.h"

#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "IItemBlock.h"
#include "IItemBlockLayout.h"
#include "Style.h"

#include "ComplexMessageItemLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int32_t getShareButtonRightMargin();

    bool isShareButtonMarginsEnabled();
}

ComplexMessageItemLayout::ComplexMessageItemLayout(ComplexMessageItem *parent)
    : QLayout(parent)
    , Item_(parent)
    , WidgetHeight_(-1)
{
    assert(Item_);
}

ComplexMessageItemLayout::~ComplexMessageItemLayout()
{
}

int32_t ComplexMessageItemLayout::evaluateStatusLineWidth() const
{
    const auto statusWidth = (
        MessageStyle::getTimeMargin() +
        MessageStatusWidget::getMaxWidth());

    return statusWidth;
}

QRect ComplexMessageItemLayout::evaluateAvatarRect(const QRect &senderContentLtr) const
{
    assert(senderContentLtr.width() > 0);
    assert(senderContentLtr.height() >= 0);

    QRect result(
        senderContentLtr.left(),
        senderContentLtr.top(),
        MessageStyle::getAvatarSize(),
        MessageStyle::getAvatarSize());

    return result;
}

QRect ComplexMessageItemLayout::evaluateBlocksBubbleGeometry(
    const bool isBubbleRequired,
    const bool isShareButtonEnabled,
    const QRect &blocksContentLtr,
    const QRect &blocksGeometry) const
{
    QRect bubbleGeometry;

    if (isBubbleRequired)
    {
        bubbleGeometry = blocksContentLtr;

        const auto bubbleHeight = blocksGeometry.height();

        bubbleGeometry.setHeight(bubbleHeight);
    }
    else
    {
        bubbleGeometry = blocksGeometry;
    }

    auto bubblePadding = evaluateBlocksContentRectMargins(isShareButtonEnabled);

    if (!isBubbleRequired)
    {
        bubblePadding.setTop(0);
        bubblePadding.setLeft(0);
        bubblePadding.setBottom(0);
    }

    bubbleGeometry = bubbleGeometry.marginsAdded(bubblePadding);

    const auto validBubbleHeight = std::max(
        bubbleGeometry.height(),
        MessageStyle::getMinBubbleHeight());

    bubbleGeometry.setHeight(validBubbleHeight);

    assert(!bubbleGeometry.isEmpty());
    return bubbleGeometry;
}

QRect ComplexMessageItemLayout::evaluateBlocksContainerLtr(
    const bool isBubbleRequired,
    const bool isShareButtonEnabled,
    const QRect &avatarRect,
    const QRect &senderContentLtr) const
{
    assert(avatarRect.width() > 0);
    assert(avatarRect.height() > 0);
    assert(avatarRect.top() >= senderContentLtr.top());
    assert(avatarRect.left() >= senderContentLtr.left());

    auto left = 0;

    if (isOutgoing())
    {
        left = senderContentLtr.left();
    }
    else
    {
        left = (avatarRect.right() + 1);
        left += MessageStyle::getAvatarRightMargin();
    }

    auto right = (senderContentLtr.right() + 1);

    auto top = senderContentLtr.top();

    const auto &margins = evaluateBlocksContentRectMargins(isShareButtonEnabled);

    if (isBubbleRequired)
    {
        left += margins.left();
        top += margins.top();
    }

    right -= margins.right();
    assert(right > left);

    QRect blocksContainerLtr;

    blocksContainerLtr.setLeft(left);
    blocksContainerLtr.setRight(right);
    blocksContainerLtr.setTop(top);
    blocksContainerLtr.setHeight(0);

    return blocksContainerLtr;
}

QMargins ComplexMessageItemLayout::evaluateBlocksContentRectMargins(const bool isShareButtonEnabled) const
{
    auto margins = Style::getDefaultBlockBubbleMargins();

    if (isShareButtonMarginsEnabled() && isShareButtonEnabled)
    {
        assert(Item_->ShareButton_);

        const auto &btn = *Item_->ShareButton_;

        const auto fullShareButtonWidth = (
            Style::getShareButtonLeftMargin() +
            btn.sizeHint().width());

        const auto extendedRightMargin = (
            margins.right() +
            fullShareButtonWidth);

        margins.setRight(extendedRightMargin);
    }

    assert(Item_);
    const auto &blocks = Item_->Blocks_;

    assert(!blocks.empty());
    if (blocks.empty())
    {
        return margins;
    }

    const auto &firstBlock = blocks.front();
    const auto firstBlockLayout = firstBlock->getBlockLayout();

    assert(firstBlockLayout);
    if (!firstBlockLayout)
    {
        return margins;
    }

    const auto &lastBlock = blocks.back();
    const auto lastBlockLayout = lastBlock->getBlockLayout();

    assert(lastBlockLayout);
    if (!lastBlockLayout)
    {
        return margins;
    }

    const auto &firstBoxModel = firstBlockLayout->getBlockBoxModel();
    margins.setTop(firstBoxModel.getBubbleMargins().top());

    const auto &lastBoxModel = lastBlockLayout->getBlockBoxModel();
    margins.setBottom(lastBoxModel.getBubbleMargins().bottom());

    return margins;
}

QRect ComplexMessageItemLayout::evaluateBlockLtr(
    const QRect &blocksContentLtr,
    IItemBlock *block,
    const int32_t blockY,
    const int32_t statusWidth,
    const bool isBubbleRequired)
{
    assert(blocksContentLtr.width() > 0);
    assert(blocksContentLtr.height() == 0);
    assert(block);
    assert(blockY >= 0);
    assert(statusWidth > 0);

    auto blocksContentWidth = blocksContentLtr.width();

   if (block->hasRightStatusPadding())
   {
       blocksContentWidth -= statusWidth;
       assert(blocksContentWidth > 0);
   }

    QRect blockLtr(
        blocksContentLtr.left(),
        blockY,
        blocksContentWidth,
        0);

    if (isBubbleRequired)
    {
        return blockLtr;
    }

    const auto blockSize = block->blockSizeForMaxWidth(blocksContentWidth);

    if (blockSize.isEmpty())
    {
        return blockLtr;
    }

    auto outgoingBlockLeft = (
        blockLtr.right() -
        blockSize.width());

    outgoingBlockLeft = std::max(outgoingBlockLeft, blockLtr.left());

    const auto incomingBlockLeft = blockLtr.left();

    const auto blockLeft = (isOutgoing() ? outgoingBlockLeft : incomingBlockLeft);

    assert(blockLeft > 0);
    blockLtr.setLeft(blockLeft);

    assert(blockLtr.width() > 0);
    return blockLtr;
}

QRect ComplexMessageItemLayout::evaluateSenderContentLtr(const QRect &widgetContentLtr) const
{
    assert(widgetContentLtr.width() > 0);
    assert(widgetContentLtr.height() >= 0);

    auto senderContentLtr(widgetContentLtr);

    if (Item_->isSenderVisible())
    {
        auto senderContentTop = widgetContentLtr.top();

        senderContentTop += MessageStyle::getSenderHeight();
        senderContentTop += MessageStyle::getSenderBottomMargin();

        senderContentLtr.moveTop(senderContentTop);
    }

    return senderContentLtr;
}

int32_t ComplexMessageItemLayout::evaluateWidgetContentLeftMargin() const
{
    auto margin = MessageStyle::getLeftMargin(isOutgoing());

    return margin;
}

QRect ComplexMessageItemLayout::evaluateWidgetContentLtr(const int32_t widgetWidth) const
{
    assert(widgetWidth > 0);

    const auto widgetContentLeftMargin = evaluateWidgetContentLeftMargin();
    assert(widgetContentLeftMargin > 0);

    const auto widgetContentRightMargin = evaluateWidgetContentRightMargin();
    assert(widgetContentRightMargin > 0);

    auto widgetContentWidth = widgetWidth;
    widgetContentWidth -= widgetContentLeftMargin;
    widgetContentWidth -= widgetContentRightMargin;

    const QRect result(
        widgetContentLeftMargin,
        MessageStyle::getTopMargin(Item_->hasTopMargin()),
        widgetContentWidth,
        0);

    return result;
}

int32_t ComplexMessageItemLayout::evaluateWidgetContentRightMargin() const
{
    const auto margin = MessageStyle::getRightMargin(isOutgoing());
    assert(margin > 0);

    return margin;
}

void ComplexMessageItemLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    setGeometryInternal(r.width());

    LastGeometry_ = r;
}

void ComplexMessageItemLayout::addItem(QLayoutItem *item)
{
    item;
}

QLayoutItem* ComplexMessageItemLayout::itemAt(int index) const
{
    index;
    return nullptr;
}

QLayoutItem* ComplexMessageItemLayout::takeAt(int index)
{
    index;
    return nullptr;
}

int ComplexMessageItemLayout::count() const
{
    return 0;
}

QSize ComplexMessageItemLayout::sizeHint() const
{
    const auto height = std::max(
        WidgetHeight_,
        MessageStyle::getMinBubbleHeight());

    return QSize(-1, height);
}

void ComplexMessageItemLayout::invalidate()
{
    QLayoutItem::invalidate();
}

bool ComplexMessageItemLayout::isOverAvatar(const QPoint &pos) const
{
    return getAvatarRect().contains(pos);
}

const QRect& ComplexMessageItemLayout::getAvatarRect() const
{
    return AvatarRect_;
}

QRect ComplexMessageItemLayout::getBlockSeparatorRect(const IItemBlock *block) const
{
    assert(block);

    if (!hasSeparator(block))
    {
        return QRect();
    }

    const auto blockLayout = block->getBlockLayout();
    assert(blockLayout);

    const auto blockGeometry = blockLayout->getBlockGeometry();

    const auto isBlockGeometrySet = (blockGeometry.width() > 0);
    if (!isBlockGeometrySet)
    {
        return QRect();
    }

    auto separatorRectHeight = Style::getBlocksSeparatorVertMargins();

    const auto &pen = Style::getBlocksSeparatorPen();
    separatorRectHeight += pen.width();

    separatorRectHeight *= 2;

    const auto separatorRectWidth = blockGeometry.width();

    QRect separatorRect(
        blockGeometry.left(),
        blockGeometry.top(),
        separatorRectWidth,
        separatorRectHeight);

    separatorRect.translate(0, -separatorRectHeight);

    return separatorRect;
}

const QRect& ComplexMessageItemLayout::getBlocksContentRect() const
{
    return BlocksContentRect_;
}

const QRect& ComplexMessageItemLayout::getBubbleRect() const
{
    return BubbleRect_;
}

QRect ComplexMessageItemLayout::getShareButtonGeometry(
    const IItemBlock &block,
    const QSize &buttonSize,
    const bool isBubbleRequired) const
{
    assert(!buttonSize.isEmpty());

    const auto blockLayout = block.getBlockLayout();
    assert(blockLayout);

    const auto blockGeometry = blockLayout->getBlockGeometry();

    auto buttonX = 0;

    if (isBubbleRequired)
    {
        assert(BubbleRect_.width() > 0);
        buttonX =
            BubbleRect_.right() + 1 -
            buttonSize.width() -
            getShareButtonRightMargin();
    }
    else
    {
        buttonX =
            blockGeometry.right() + 1 +
            MessageStyle::getTimeMargin();
    }

    const auto buttonY = blockGeometry.top();

    const QRect shareButtonRect(
        QPoint(buttonX, buttonY),
        buttonSize);

    return shareButtonRect;
}

void ComplexMessageItemLayout::onBlockSizeChanged()
{
    if (LastGeometry_.width() <= 0)
    {
        return;
    }

    const auto sizeBefore = sizeHint();

    setGeometryInternal(LastGeometry_.width());

    const auto sizeAfter = sizeHint();

    if (sizeBefore != sizeAfter)
    {
        update();
    }
}

bool ComplexMessageItemLayout::hasSeparator(const IItemBlock *block) const
{
    const auto &blocks = Item_->Blocks_;

    auto blockIter = std::find(blocks.cbegin(), blocks.cend(), block);

    const auto isFirstBlock = (blockIter == blocks.begin());
    if (isFirstBlock)
    {
        return false;
    }

    auto blockLayout = (*blockIter)->getBlockLayout();
    if (!blockLayout)
    {
        return false;
    }

    auto prevBlockIter = blockIter;
    --prevBlockIter;

    const auto prevBlockLayout = (*prevBlockIter)->getBlockLayout();
    if (!prevBlockLayout)
    {
        return false;
    }

    const auto &prevBlockBox = prevBlockLayout->getBlockBoxModel();

    const auto &blockBox = blockLayout->getBlockBoxModel();

    const auto hasSeparator = (blockBox.hasLeadLines() || prevBlockBox.hasLeadLines());

    return hasSeparator;
}

bool ComplexMessageItemLayout::isOutgoing() const
{
    assert(Item_);
    return Item_->isOutgoing();
}

QRect ComplexMessageItemLayout::setBlocksGeometry(
    const bool isBubbleRequired,
    const QRect &blocksContentLtr)
{
    const auto topY = blocksContentLtr.top();

    const auto statusWidth = evaluateStatusLineWidth();

    auto blocksHeight = 0;
    auto blocksWidth = 0;
    auto blocksLeft = blocksContentLtr.left();

    auto &blocks = Item_->Blocks_;
    for (auto block : blocks)
    {
        assert(block);

        const auto addSeparator = hasSeparator(block);

        const auto blockSeparatorHeight = (
            addSeparator ?
                getBlockSeparatorRect(block).height() :
                Utils::scale_value(2));

        const auto blockY = (
            topY +
            blocksHeight +
            blockSeparatorHeight);

        const auto blockLtr = evaluateBlockLtr(blocksContentLtr, block, blockY, statusWidth, isBubbleRequired);

        const auto blockGeometry = block->setBlockGeometry(blockLtr);

        const auto blockHeight = blockGeometry.height();
        assert(blocksHeight >= 0);

        blocksHeight += blockHeight;

        blocksHeight += blockSeparatorHeight;

        blocksWidth = std::max(blocksWidth, blockGeometry.width());

        blocksLeft = std::max(blocksLeft, blockGeometry.left());
    }

    assert(blocksWidth >= 0);

    QRect result(
        blocksLeft,
        topY,
        blocksWidth,
        blocksHeight);

    return result;
}

void ComplexMessageItemLayout::setGeometryInternal(const int32_t widgetWidth)
{
    assert(widgetWidth > 0);

    const auto widgetContentLtr = evaluateWidgetContentLtr(widgetWidth);

    const auto enoughSpace = (widgetContentLtr.width() > 0);
    if (!enoughSpace)
    {
        return;
    }

    const auto senderContentLtr = evaluateSenderContentLtr(widgetContentLtr);

    AvatarRect_ = evaluateAvatarRect(senderContentLtr);

    const auto isBubbleRequired = Item_->isBubbleRequired();

    const auto isShareButtonEnabled = Item_->containsShareableBlocks();

    const auto blocksContainertLtr = evaluateBlocksContainerLtr(isBubbleRequired, isShareButtonEnabled,  AvatarRect_, senderContentLtr);

    const auto blocksGeometry = setBlocksGeometry(isBubbleRequired, blocksContainertLtr);

    BlocksContentRect_ = blocksGeometry;

    const auto bubbleRect = evaluateBlocksBubbleGeometry(
        isBubbleRequired,
        isShareButtonEnabled,
        blocksContainertLtr,
        blocksGeometry);
    assert(bubbleRect.height() >= MessageStyle::getMinBubbleHeight());

    setStatusGeometry(isBubbleRequired, bubbleRect, blocksGeometry);

    setSenderGeometry(AvatarRect_, widgetContentLtr);

    BubbleRect_ = bubbleRect;

    WidgetHeight_ = (bubbleRect.bottom() + 1);

    if (Item_->IsLastRead_)
    {
        const auto seenHeight = (
            MessageStyle::getLastReadAvatarSize() +
            (2 * MessageStyle::getLastReadAvatarMargin()));

        WidgetHeight_ += seenHeight;
    }

    assert(WidgetHeight_ >= 0);
}

void ComplexMessageItemLayout::setSenderGeometry(
    const QRect &avatarRect,
    const QRect &widgetContentLtr)
{
    avatarRect;

    if (!Item_->Sender_)
    {
        return;
    }

    auto &sender = *Item_->Sender_;

    if (!Item_->isSenderVisible())
    {
        sender.setVisible(false);

        return;
    }

    const auto senderPos = widgetContentLtr.topLeft();
    sender.move(senderPos);

    sender.setVisible(true);
}

void ComplexMessageItemLayout::setStatusGeometry(
    const bool isBubbleRequired,
    const QRect &bubbleGeometry,
    const QRect &blocksGeometry)
{
    if (!Item_->Status_)
    {
        return;
    }

    auto &status = *Item_->Status_;

    const auto statusWidgetSize = status.sizeHint();

    auto statusX = 0;

    if (isBubbleRequired)
    {
        assert(!bubbleGeometry.isEmpty());

        const auto fullStatusLineWidth = evaluateStatusLineWidth();

        statusX = (
            bubbleGeometry.right() + 1 -
            fullStatusLineWidth);
    }
    else
    {
        assert(!blocksGeometry.isEmpty());
        statusX = (
            bubbleGeometry.right() + 1);
    }

    const auto statusY = (
        bubbleGeometry.bottom() + 1 -
        MessageStyle::getTimeMargin() -
        statusWidgetSize.height());

    const QRect statusGeometry(
        statusX,
        statusY,
        MessageStatusWidget::getMaxWidth(),
        statusWidgetSize.height());

    status.setGeometry(statusGeometry);

    status.show();
}

namespace
{
    int32_t getShareButtonRightMargin()
    {
        return Utils::scale_value(16);
    }

    bool isShareButtonMarginsEnabled()
    {
        return false;
    }
}

UI_COMPLEX_MESSAGE_NS_END