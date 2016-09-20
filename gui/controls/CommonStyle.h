#pragma once

#define UI_COMMON_STYLE_NS_BEGIN namespace Ui { namespace CommonStyle {
#define UI_COMMON_STYLE_NS_END } }

UI_COMMON_STYLE_NS_BEGIN

QString getCloseButtonStyle();

QColor getContactListHoveredColor();
QColor getContactListSelectedColor();

QString getDisabledButtonStyle();
QString getGrayButtonStyle();
QString getGreenButtonStyle();
QString getGreenButtonStyleNoBorder();

QColor getFrameTransparency();

QColor getLinkColor();
QColor getLinkColorHovered();
QColor getLinkColorPressed();

QColor getRedLinkColor();
QColor getRedLinkColorHovered();
QColor getRedLinkColorPressed();

QColor getTextCommonColor();

QString getLineEditStyle();
QString getTextEditStyle();
QString getLineEditErrorStyle();


UI_COMMON_STYLE_NS_END
