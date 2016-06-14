#include "stdafx.h"

#include "MessageStyle.h"

#include "../../cache/themes/themes.h"

#include "../../theme_settings.h"

#include "../../utils/utils.h"

UI_MESSAGE_STYLE_NS_BEGIN

namespace
{
    int32_t byteAlpha(const double alpha)
    {
        assert(alpha >= 0);
        assert(alpha <= 1.0);

        return (int32_t)(alpha * 255);
    }
}

QFont getTextFont()
{
    static QFont font(
        Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15))
    );

    return font;
}

QColor getIncomingBodyColorA(const double alpha)
{
    return QColor(0xff, 0xff, 0xff, byteAlpha(alpha));
}

QColor getOutgoingBodyColorA(const double alpha)
{
    return QColor(0xd8, 0xd4, 0xce, byteAlpha(alpha));
}

QColor getIncomingBodyColorB(const double alpha)
{
    return QColor(0xff, 0xff, 0xff, byteAlpha(alpha));
}

QColor getOutgoingBodyColorB(const double alpha)
{
    return QColor(0xd5, 0xd2, 0xce, byteAlpha(alpha));
}

QBrush getBodyBrush(
    const bool isOutgoing, 
    const bool isSelected, 
    const int _theme_id)
{
    auto _theme = get_qt_theme_settings()->themeForId(_theme_id);

    QLinearGradient grad(0, 0, 1, 0);

    grad.setCoordinateMode(QGradient::ObjectBoundingMode);

    QColor outgoingSelectedBackgroundColor0 = !isSelected ? _theme->outgoing_bubble_.bg1_color_ : QColor(0x57, 0x9e, 0x1c, (int32_t)(0.9 * 255));
    QColor incomingSelectedBackgroundColor0 = !isSelected ? _theme->incoming_bubble_.bg1_color_ : QColor(0x57, 0x9e, 0x1c, (int32_t)(1.0 * 255));

    QColor outgoingSelectedBackgroundColor1 = !isSelected ? _theme->outgoing_bubble_.bg2_color_ : QColor(0x57, 0x9e, 0x1c, (int32_t)(0.72 * 255));
    QColor incomingSelectedBackgroundColor1 = !isSelected ? _theme->incoming_bubble_.bg2_color_ : QColor(0x57, 0x9e, 0x1c, (int32_t)(0.72 * 255));

    QColor outgoingBackgroundColor0 = !isSelected ? _theme->outgoing_bubble_.bg1_color_ : QColor(0xd8, 0xd4, 0xce, (int32_t)(0.9 * 255));
    QColor incomingBackgroundColor0 = !isSelected ? _theme->incoming_bubble_.bg1_color_ : QColor(0xff, 0xff, 0xff, (int32_t)(1.0 * 255));

    QColor outgoingBackgroundColor1 = !isSelected ? _theme->outgoing_bubble_.bg2_color_ : QColor(0xd5, 0xd2, 0xce, (int32_t)(0.72 * 255));
    QColor incomingBackgroundColor1 = !isSelected ? _theme->incoming_bubble_.bg2_color_ : QColor(0xff, 0xff, 0xff, (int32_t)(0.72 * 255));

    if (isSelected)
    {
        const auto color0 = (
            isOutgoing ?
                outgoingSelectedBackgroundColor0 :
                incomingSelectedBackgroundColor0
        );


        grad.setColorAt(0, color0);

        const auto color1 = (
            isOutgoing ?
                outgoingSelectedBackgroundColor1 :
                incomingSelectedBackgroundColor1
        );
        grad.setColorAt(1, color1);
    }
    else
    {
        const auto color0 = (
            isOutgoing ?
                outgoingBackgroundColor0 :
                incomingBackgroundColor0
        );
        grad.setColorAt(0, color0);

        const auto color1 = (
            isOutgoing ?
                outgoingBackgroundColor1 :
                incomingBackgroundColor1
        );
        grad.setColorAt(1, color1);
    }

    QBrush result(grad);
    result.setColor(QColor(0, 0, 0, 0));

    return result;
}

int32_t getBubbleHeight()
{
    return Utils::scale_value(32);
}

int32_t getBorderRadius()
{
    return Utils::scale_value(8);
}

int32_t getTopPadding(const bool hasTopMargin)
{
    return Utils::scale_value(
        hasTopMargin ? 12 : 2
    );
}

int32_t getLeftPadding(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 118 : 24
    );
}

int32_t getRightPadding(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 16 : 72
    );
}

int32_t getTimeStatusMargin()
{
    return Utils::scale_value(8);
}

int32_t getAvatarSize()
{
    return Utils::scale_value(32);
}

int32_t getAvatarRightMargin()
{
    return Utils::scale_value(6);
}

int32_t getBubbleHorPadding()
{
    return Utils::scale_value(16);
}

int getLastReadAvatarSize()
{
    return Utils::scale_value(16);
}

int getLastReadAvatarMargin()
{
    return Utils::scale_value(4);
}

UI_MESSAGE_STYLE_NS_END