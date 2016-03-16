#pragma once

namespace Utils
{

    QPainterPath renderMessageBubble(
        const QSize &size,
        const int32_t borderRadius,
        const bool isOutgoing);

    QPainterPath renderMessageBubble(
        const QRect &rect,
        const int32_t borderRadius,
        const bool isOutgoing);

}