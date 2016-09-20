#pragma once

#define UI_MESSAGE_STYLE_NS_BEGIN namespace Ui { namespace MessageStyle {
#define UI_MESSAGE_STYLE_NS_END } }

UI_MESSAGE_STYLE_NS_BEGIN

QFont getTextFont();

QColor getTextColor();

QColor getTimeColor();

QFont getTimeFont();

QColor getIncomingBodyColorA(const double alpha);

QColor getOutgoingBodyColorA(const double alpha);

QColor getIncomingBodyColorB(const double alpha);

QColor getOutgoingBodyColorB(const double alpha);

QBrush getBodyBrush(const bool isOutgoing, const bool isSelected, const int theme_id);

int32_t getMinBubbleHeight();

int32_t getBorderRadius();

int32_t getTopMargin(const bool hasTopMargin);

int32_t getLeftMargin(const bool isOutgoing);

int32_t getRightMargin(const bool isOutgoing);

int32_t getTimeMargin();

int32_t getAvatarSize();

int32_t getAvatarRightMargin();

int32_t getBubbleHorPadding();

int32_t getLastReadAvatarSize();

int32_t getLastReadAvatarMargin();

int32_t getHistoryWidgetMaxWidth();

QSize getImagePlaceholderSize();

QBrush getImagePlaceholderBrush();

QBrush getImageShadeBrush();

QSize getMinPreviewSize();

QSizeF getMinPreviewSizeF();

QFont getRotatingProgressBarTextFont();

QPen getRotatingProgressBarTextPen();

int32_t getRotatingProgressBarTextTopMargin();

int32_t getRotatingProgressBarPenWidth();

QPen getRotatingProgressBarPen();

int32_t getSenderBottomMargin();

int32_t getSenderHeight();

int32_t getTextWidthStep();

int32_t roundTextWidthDown(const int32_t width);

UI_MESSAGE_STYLE_NS_END