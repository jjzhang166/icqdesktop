#pragma once

#include "namespaces.h"

FONTS_NS_BEGIN

enum class FontFamily
{
    MIN,

    ARIAL,
    HELVETICA_NEUE,
    LUCIDA_GRANDE,
    SAN_FRANCISCO,
    SEGOE_UI,

    MAX,
};

enum class FontStyle
{
    MIN,

    NORMAL,
    BOLD,
    SEMIBOLD,
    LIGHT,

    MAX
};

QFont appFont(const int32_t _sizePx);

QFont appFont(const int32_t _sizePx, const FontFamily _family);

QFont appFont(const int32_t _sizePx, const FontStyle _style);

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontStyle _style);

QFont appFontScaled(const int32_t _sizePx);

QFont appFontScaled(const int32_t _sizePx, const FontStyle _style);

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontStyle _style);

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontStyle _fontStyle);

QString appFontFullNameQss(const FontFamily _fontFamily, const FontStyle _fontStyle);

QString appFontWeightQss(const QFont::Weight _weight);

FontFamily defaultAppFontFamily();

QString defaultAppFontQssName();

QString defaultAppFontQssWeight();

FontStyle defaultAppFontStyle();

FONTS_NS_END