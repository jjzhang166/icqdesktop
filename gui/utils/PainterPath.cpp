#include "stdafx.h"

#include "utils.h"

#include "PainterPath.h"

namespace Utils
{

    QPainterPath renderMessageBubble(
        const QSize &size,
        const int32_t borderRadius,
        const bool isOutgoing)
    {
        assert(size.isValid());

        return renderMessageBubble(
            QRect(QPoint(0, 0), size),
            borderRadius,
            isOutgoing
        );
    }

    QPainterPath renderMessageBubble(
        const QRect &rect,
        const int32_t borderRadius,
        const bool isOutgoing)
    {
        assert(rect.isValid());
        assert(borderRadius > 0);
        assert((borderRadius * 2) <= rect.width());
        assert((borderRadius * 2) <= rect.height());

        const auto borderDiameter = (borderRadius * 2);

        const auto heightMinusBorder = (rect.height() - borderDiameter);
        const auto widthMinusBorder = (rect.width() - borderDiameter);

        assert(heightMinusBorder > 0);
        assert(widthMinusBorder > 0);

        QPainterPath clipPath;

        auto x = 0;
        auto y = 0;

        // lt

        if (isOutgoing)
        {
            x += borderRadius;
        }

        clipPath.moveTo(x, y);

        if (isOutgoing)
        {
            clipPath.arcTo(0, 0, borderDiameter, borderDiameter, 90, 90);
        }

        // lb

        x = 0;
        y = heightMinusBorder;
        clipPath.lineTo(x, y);

        clipPath.arcTo(x, y, borderDiameter, borderDiameter, 180, 90);

        // rb

        x = (isOutgoing ? rect.width() : widthMinusBorder);
        y = rect.height();
        clipPath.lineTo(x, y);

        if (!isOutgoing)
        {
            y -= borderDiameter;
            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 270, 90);
        }

        // rt

        x = rect.width();
        y = borderDiameter;
        clipPath.lineTo(x, y);

        x -= borderDiameter;
        y -= borderDiameter;
        clipPath.arcTo(x, y, borderDiameter, borderDiameter, 0, 90);

        // the final closure

        x = (isOutgoing ? borderRadius : 0);
        y = 0;
        clipPath.lineTo(x, y);

        clipPath = clipPath.translated(rect.left(), rect.top());

        return clipPath;
    }

}