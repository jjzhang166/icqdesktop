#include "stdafx.h"
#include "CommonStyle.h"

#include "../utils/utils.h"

UI_COMMON_STYLE_NS_BEGIN

QString getCloseButtonStyle()
{
#ifdef __APPLE__
    const QString close_button_style =
        "QPushButton {"
        "border-image: url(:/resources/main_window/contr_close_100.png);"
        "width: 24dip; height: 24dip;"
        "background-color: transparent;"
        "padding: 0; margin: 0;"
        "border: none; } "
        "QPushButton:hover {"
        "border-image: url(:/resources/main_window/contr_close_100_hover.png);"
        "background-color: #e81123; }"
        "QPushButton:pressed {"
        "border-image: url(:/resources/main_window/contr_close_100_active.png);"
        "background-color: #d00516; }";
#else
    const QString close_button_style =
        "QPushButton {"
        "background-image: url(:/resources/main_window/contr_close_100.png);"
        "background-color: transparent;"
        "background-repeat: no-repeat;"
        "background-position: center;"
        "background-color: transparent;"
        "padding-top: 2dip; padding-bottom: 2dip;"
        "width: 24dip; height: 24dip;"
        "padding-left: 11dip; padding-right: 12dip;"
        "border: none; }"
        "QPushButton:hover {"
        "background-image: url(:/resources/main_window/contr_close_100_hover.png);"
        "background-color: #e81123; }"
        "QPushButton:pressed {"
        "background-image: url(:/resources/main_window/contr_close_100_active.png);"
        "background-color: #d00516; } ";
#endif
    return close_button_style;
}

QColor getContactListHoveredColor()
{
    return QColor(0xe8, 0xed, 0xed, (int32_t)(0.5 * 255));
}

QColor getContactListSelectedColor()
{
    return QColor(0xd9, 0xdd, 0xdd, (int32_t)(0.5 * 255));
}

QString getDisabledButtonStyle()
{
    return QString(
        "QPushButton {"
        "color: #a9a9a9;"
        "font-size: 16dip;"
        "background-color: #e4e4e4;"
        "border-style: none;"
        "margin: 0;"
        "padding-left: 20dip; padding-right: 20dip;"
        "min-width: 100dip;"
        "max-height: 32dip; min-height: 32dip; }"
    );
}

QString getGrayButtonStyle()
{
    return QString(
        "QPushButton {"
        "color: %1;"
        "font-size: 16dip;"
        "background-color: #c5c5c5;"
        "border-style: none;"
        "margin: 0;"
        "padding-left: 20dip; padding-right: 20dip;"
        "min-width: 100dip;"
        "max-height: 32dip; min-height: 32dip; }"
        "QPushButton:hover { background-color: #d2d2d2; }"
        "QPushButton:pressed { background-color: #bbbbbb; } "
    ).arg(Utils::rgbaStringFromColor(getTextCommonColor()));
}

QString getGreenButtonStyle()
{
    return QString(
        "QPushButton {"
            "color: #ffffff;"
            "font-size: 16dip;"
            "background-color: #579e1c;"
            "border-style: none;"
            "margin: 0;"
            "padding-left: 20dip; padding-right: 20dip;"
            "min-width: 100dip;"
            "max-height: 32dip; min-height: 32dip;"
            "text-align: center; }"
        "QPushButton:hover { background-color: #57a813; }"
        "QPushButton:pressed { background-color: #50901b; } "
    );
}

QString getGreenButtonStyleNoBorder()
{
    return QString(
        "QPushButton {"
            "color: #ffffff;"
            "font-size: 16dip;"
            "background-color: #579e1c;"
            "border-style: none;"
            "margin: 0;"
            "padding-left: 20dip; padding-right: 20dip;"
            "max-height: 32dip; min-height: 32dip; }"
        "QPushButton:hover { background-color: #57a813; }"
        "QPushButton:pressed { background-color: #50901b; }"
    );
}

QColor getFrameTransparency()
{
    return QColor(0xff, 0xff, 0xff, (int32_t)(0.95 * 255));
}

QColor getLinkColor()
{
    return QColor(0x57, 0x9e, 0x1c);
}
QColor getLinkColorHovered()
{
    return QColor(0x60, 0xaa, 0x23);
}
QColor getLinkColorPressed()
{
    return QColor(0x48, 0x9b, 0x12);
}

QColor getRedLinkColor()
{
    return QColor(0xe3, 0x0f, 0x04);
}
QColor getRedLinkColorHovered()
{
    return QColor(0xff, 0x35, 0x2b);
}
QColor getRedLinkColorPressed()
{
    return QColor(0xd1, 0x14, 0x0b);
}

QColor getTextCommonColor()
{
    return QColor(0x28, 0x28, 0x28);
}

QString getLineEditStyle()
{
    return QString(
        "QLineEdit {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "color: %1;"
            "border-style: none;"
            "border-bottom-color: #cccccc;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        "QLineEdit:focus {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "color: %1;"
            "border-style: none;"
            "border-bottom-color: #579e1c;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
    ).arg(Utils::rgbaStringFromColor(getTextCommonColor()));
}

QString getTextEditStyle()
{
    return QString(
        "QTextBrowser {"
        "background-color: transparent;"
        "color: %1;"
        "border-style: none;"
        "border-bottom-color: #cccccc;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid;}"
        "QTextBrowser:focus {"
        "background-color: transparent;"
        "color: %1;"
        "border-style: none;"
        "border-bottom-color: #579e1c;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }"
        ).arg(Utils::rgbaStringFromColor(getTextCommonColor()));
}

QString getLineEditErrorStyle()
{
    return QString(
        "QLineEdit {"
        "min-height: 48dip; max-height: 48dip;"
        "background-color: transparent;"
        "color: #e20b00;"
        "border-style: none;"
        "border-bottom-color: #e20b00;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }"
        "QLineEdit:focus {"
        "min-height: 48dip; max-height: 48dip;"
        "background-color: transparent;"
        "color: #e20b00;"
        "border-style: none;"
        "border-bottom-color: #e20b00;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }");
}

UI_COMMON_STYLE_NS_END
