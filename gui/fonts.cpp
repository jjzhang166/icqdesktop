#include "stdafx.h"

#include "platform.h"
#include "utils/utils.h"

#include "fonts.h"

FONTS_NS_BEGIN

namespace
{
    QString appFontStyleName(const FontFamily _fontFamily, const FontStyle _fontStyle);

    void applyFontFamilyName(const FontFamily _fontFamily, Out QFont &_font);

    void applyFontStyleName(const FontFamily _fontFamily, const FontStyle _fontStyle, Out QFont &_font);

    QString evalNormalFontWeight(const FontFamily _fontFamily, const FontStyle _fontStyle);

    QString segoeUiFullName(const FontStyle _fontStyle);

}

QFont appFont(const int32_t _sizePx)
{
    return appFont(_sizePx, defaultAppFontFamily(), defaultAppFontStyle());
}

QFont appFont(const int32_t _sizePx, const FontFamily _family)
{
    return appFont(_sizePx, _family, defaultAppFontStyle());
}

QFont appFont(const int32_t _sizePx, const FontStyle _style)
{
    return appFont(_sizePx, defaultAppFontFamily(), _style);
}

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontStyle _style)
{
    assert(_sizePx > 0);
    assert(_sizePx < 500);
    assert(_family > FontFamily::MIN);
    assert(_family < FontFamily::MAX);
    assert(_style > FontStyle::MIN);
    assert(_style < FontStyle::MAX);

    QFont result;

    applyFontFamilyName(_family, Out result);

    applyFontStyleName(_family, _style, Out result);

    result.setPixelSize(_sizePx);

    return result;
}

QFont appFontScaled(const int32_t _sizePx)
{
    return appFont(Utils::scale_value(_sizePx));
}

QFont appFontScaled(const int32_t _sizePx, const FontStyle _style)
{
    return appFont(Utils::scale_value(_sizePx), _style);
}

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontStyle _style)
{
    return appFont(Utils::scale_value(_sizePx), _family, _style);
}

FontFamily defaultAppFontFamily()
{
    if (platform::is_apple())
    {
        const auto osxVersion = platform::osxVersion();

        if (osxVersion < platform::OsxVersion::MV_10_10)
        {
            return FontFamily::LUCIDA_GRANDE;
        }

        if (osxVersion <= platform::OsxVersion::MV_10_10)
        {
            return FontFamily::HELVETICA_NEUE;
        }

        return FontFamily::SAN_FRANCISCO;
    }

    if (platform::is_windows_vista_or_late())
    {
        return FontFamily::SEGOE_UI;
    }

    return FontFamily::ARIAL;
}

FontStyle defaultAppFontStyle()
{
    return FontStyle::NORMAL;
}

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontStyle _fontStyle)
{
    assert(_sizePx > 0);
    assert(_sizePx < 1000);
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);
    assert(_fontStyle > FontStyle::MIN);
    assert(_fontStyle < FontStyle::MAX);

    QString result;
    result.reserve(512);

    const auto appFontStyle = appFontStyleName(_fontFamily, _fontStyle);
    if (!appFontStyle.isEmpty())
    {
        result += "font-style: ";
        result += appFontStyle;
        result += "; ";
    }

    result += "font-size: ";
    result += QString::number(_sizePx);
    result += "px; font-family: \"";
    result += appFontFullNameQss(_fontFamily, _fontStyle);
    result += "\"";

    const auto weight = evalNormalFontWeight(_fontFamily, _fontStyle);
    if (!weight.isEmpty())
    {
        result += "; font-weight: ";
        result += weight;
    }

    return result;
}

QString appFontFullNameQss(const FontFamily _fontFamily, const FontStyle _fontStyle)
{
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);
    assert(_fontStyle > FontStyle::MIN);
    assert(_fontStyle < FontStyle::MAX);

    switch (_fontFamily)
    {
        case FontFamily::ARIAL:
            return "Arial";

        case FontFamily::HELVETICA_NEUE:
            return "Helvetica Neue";

        case FontFamily::LUCIDA_GRANDE:
            return "Lucida Grande";

        case FontFamily::SAN_FRANCISCO:
            return ".SF NS Text";

        case FontFamily::SEGOE_UI:
            return segoeUiFullName(_fontStyle);
    }

    assert(!"unexpected font family");
    return QString();
}

QString appFontWeightQss(const QFont::Weight _weight)
{
    switch (_weight)
    {
        case QFont::Weight::Light: return "200";
        case QFont::Weight::Normal: return "400";
        case QFont::Weight::DemiBold: return "600";
        case QFont::Weight::Bold: return "800";
    }

    assert(!"unknown weight");
    return "400";
}

QString defaultAppFontQssName()
{
    return appFontFullNameQss(defaultAppFontFamily(), defaultAppFontStyle());
}

QString defaultAppFontQssWeight()
{
    return "400";
}

/*const QString& appFontFamily(const FontsFamily _fontFamily)
{
static std::map<FontsFamily, QString> fontFamilyMap;
if (fontFamilyMap.empty())
{
#if defined (_WIN32)
if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
{
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Segoe UI")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Segoe UI Bold")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Segoe UI Semibold")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Segoe UI Light")));
}
else
{
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Arial")));
}
#elif defined (__APPLE__)
if (QSysInfo().macVersion() > QSysInfo().MV_10_11)
{
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, ".Helvetica Neue DeskInterface"));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, ".Helvetica Neue DeskInterface"));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, ".Helvetica Neue DeskInterface"));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, ".Helvetica Neue DeskInterface"));
}
else if (QSysInfo().macVersion() == QSysInfo().MV_10_11)
{
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Helvetica Neue")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Helvetica Neue")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Helvetica Neue")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Helvetica Neue")));
}
else
{
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Helvetica Neue")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Helvetica Neue Bold")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Helvetica Neue Medium")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Helvetica Neue Light")));
}
#else //linux
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_BOLD, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_SEMIBOLD, QString("Arial")));
fontFamilyMap.emplace(std::make_pair(FontsFamily::SEGOE_UI_LIGHT, QString("Arial")));
#endif
}

return fontFamilyMap[_fontFamily];
}

*/

namespace
{
    QString evalNormalFontWeight(const FontFamily _fontFamily, const FontStyle _fontStyle)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontStyle > FontStyle::MIN);
        assert(_fontStyle < FontStyle::MAX);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            return QString();
        }

        if (_fontFamily == FontFamily::ARIAL)
        {
            if (_fontStyle == FontStyle::BOLD)
                return appFontWeightQss(QFont::Weight::Bold);

            if (_fontStyle == FontStyle::LIGHT)
                return appFontWeightQss(QFont::Weight::Light);

            if (_fontStyle == FontStyle::SEMIBOLD)
                return appFontWeightQss(QFont::Weight::DemiBold);

            return QString();
        }

        if (_fontFamily == FontFamily::SAN_FRANCISCO)
        {
            if (_fontStyle == FontStyle::LIGHT)
                return appFontWeightQss(QFont::Weight::Light);

            if (_fontStyle == FontStyle::SEMIBOLD)
                return appFontWeightQss(QFont::Weight::DemiBold);

            return QString();
        }

        if (_fontFamily == FontFamily::HELVETICA_NEUE)
        {
            //if (_fontStyle == FontStyle::LIGHT)
            //    return appFontWeightQss()
        }

        assert(!"unknown font family / style comnbination");
        return QString();
    }

    QString segoeUiFullName(const FontStyle _fontStyle)
    {
        assert(_fontStyle > FontStyle::MIN);
        assert(_fontStyle < FontStyle::MAX);

        QString result;
        result.reserve(128);

        result += "Segoe UI";

        switch(_fontStyle)
        {
            case FontStyle::BOLD:
            case FontStyle::SEMIBOLD:
                result += " Semibold";
                break;

            case FontStyle::LIGHT:
                result += " Light";
                break;

            case FontStyle::NORMAL:
                break;

            default:
                assert(!"unknown font style");
        }

        return result;
    }

    QString appFontStyleName(const FontFamily _fontFamily, const FontStyle _fontStyle)
    {
        if (_fontFamily == FontFamily::SAN_FRANCISCO)
        {
            if (_fontStyle == FontStyle::BOLD)
            {
                return "Bold";
            }
        }

        return QString();
    }

    void applyFontFamilyName(const FontFamily _fontFamily, Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);

        switch (_fontFamily)
        {
            case FontFamily::SEGOE_UI:
                _font.setFamily("Segoe UI");
                return;

            case FontFamily::SAN_FRANCISCO:
                _font.setFamily(".SF NS Text");
                return;

            default:
                assert(!"unexpected font family");
                return;
        }
    }

    void applyFontStyleName(const FontFamily _fontFamily, const FontStyle _fontStyle, Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontStyle > FontStyle::MIN);
        assert(_fontStyle < FontStyle::MAX);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            _font.setFamily("Segoe UI");

            switch (_fontStyle)
            {
                case FontStyle::SEMIBOLD:
                    _font.setFamily("Segoe UI Semibold");
                    return;

                case FontStyle::LIGHT:
                    _font.setFamily("Segoe UI Light");
                    return;

                case FontStyle::NORMAL:
                    return;

                case FontStyle::BOLD:
                    _font.setWeight(QFont::Weight::Bold);
                    return;
            }

            assert(!"unexpected font style");
            return;
        }

        if (_fontFamily == FontFamily::SAN_FRANCISCO)
        {
            _font.setFamily(".SF NS Text");

            switch (_fontStyle)
            {
                case FontStyle::BOLD:
                    _font.setStyleName("Bold");
                    return;

                case FontStyle::LIGHT:
                    _font.setWeight(QFont::Weight::Light);
                    return;

                case FontStyle::NORMAL:
                    return;

                case FontStyle::SEMIBOLD:
                    _font.setWeight(QFont::Weight::DemiBold);
                    return;
            }

            assert(!"unexpected font style");
            return;
        }

        assert(!"unexpected font family");
        return;
    }
}

FONTS_NS_END