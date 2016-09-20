#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;
class IItemBlock;

class ComplexMessageItemLayout final : public QLayout
{
public:
    ComplexMessageItemLayout(ComplexMessageItem *parent);

    virtual ~ComplexMessageItemLayout() override;

    virtual void setGeometry(const QRect &r) override;

    virtual void addItem(QLayoutItem *item) override;

    virtual QLayoutItem *itemAt(int index) const override;

    virtual QLayoutItem *takeAt(int index) override;

    virtual int count() const override;

    virtual QSize sizeHint() const override;

    virtual void invalidate() override;

    bool isOverAvatar(const QPoint &pos) const;

    const QRect& getAvatarRect() const;

    QRect getBlockSeparatorRect(const IItemBlock *block) const;

    const QRect& getBlocksContentRect() const;

    const QRect& getBubbleRect() const;

    QRect getShareButtonGeometry(
        const IItemBlock &block,
        const QSize &buttonSize,
        const bool isBubbleRequired) const;

    void onBlockSizeChanged();

private:
    int32_t evaluateStatusLineWidth() const;

    QRect evaluateAvatarRect(const QRect &senderContentLtr) const;

    QRect evaluateBlocksBubbleGeometry(
        const bool isBubbleRequired,
        const bool isShareButtonEnabled,
        const QRect &blocksContentLtr,
        const QRect &blocksGeometry) const;

    QRect evaluateBlocksContainerLtr(
        const bool isBubbleRequired,
        const bool isShareButtonEnabled,
        const QRect &avatarRect,
        const QRect &senderContentLtr) const;

    QMargins evaluateBlocksContentRectMargins(const bool isShareButtonEnabled) const;

    QRect evaluateBlockLtr(
        const QRect &blocksContentLtr,
        IItemBlock *block,
        const int32_t blockY,
        const int32_t statusWidth,
        const bool isBubbleRequired);

    QRect evaluateSenderContentLtr(const QRect &widgetContentLtr) const;

    int32_t evaluateWidgetContentLeftMargin() const;

    QRect evaluateWidgetContentLtr(const int32_t widgetWidth) const;

    int32_t evaluateWidgetContentRightMargin() const;

    bool hasSeparator(const IItemBlock *block) const;

    bool isOutgoing() const;

    QRect setBlocksGeometry(
        const bool isBubbleRequired,
        const QRect &blocksContentLtr);

    void setGeometryInternal(const int32_t widgetWidth);

    void setSenderGeometry(
        const QRect &avatarRect,
        const QRect &widgetContentLtr);

    void setStatusGeometry(
        const bool isBubbleRequired,
        const QRect &bubbleGeometry,
        const QRect &blocksGeometry);

    ComplexMessageItem *Item_;

    QRect LastGeometry_;

    QRect BubbleRect_;

    int32_t WidgetHeight_;

    QRect AvatarRect_;

    QRect BlocksContentRect_;

};

UI_COMPLEX_MESSAGE_NS_END