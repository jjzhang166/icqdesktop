#pragma once

namespace Utils
{

    enum RenderBubbleFlags
    {
        RenderBubble_LeftTopRounded         = (1 << 0),
        RenderBubble_RightTopRounded        = (1 << 1),
        RenderBubble_RightBottomRounded     = (1 << 2),
        RenderBubble_LeftBottomRounded      = (1 << 3),

        RenderBubble_LeftRounded            = (RenderBubble_LeftTopRounded |
                                               RenderBubble_LeftBottomRounded),

        RenderBubble_RightRounded            = (RenderBubble_RightTopRounded |
                                                RenderBubble_RightBottomRounded),

        RenderBubble_TopRounded             = (RenderBubble_LeftTopRounded |
                                               RenderBubble_RightTopRounded),

        RenderBubble_BottomRounded          = (RenderBubble_LeftBottomRounded |
                                               RenderBubble_RightBottomRounded),

        RenderBubble_AllRounded             = (RenderBubble_TopRounded |
                                               RenderBubble_BottomRounded)
    };

    QPainterPath renderMessageBubble(
        const QSize &size,
        const int32_t borderRadius,
        const bool isOutgoing,
        const RenderBubbleFlags flags = RenderBubble_AllRounded);

    QPainterPath renderMessageBubble(
        const QRect &rect,
        const int32_t borderRadius,
        const bool isOutgoing,
        const RenderBubbleFlags flags = RenderBubble_AllRounded);

}