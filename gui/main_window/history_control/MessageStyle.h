#pragma once

#define UI_MESSAGE_STYLE_NS_BEGIN namespace Ui { namespace MessageStyle {
#define UI_MESSAGE_STYLE_NS_END } }

UI_MESSAGE_STYLE_NS_BEGIN

QFont getTextFont();

QColor getIncomingBodyColorA(const double alpha);

QColor getOutgoingBodyColorA(const double alpha);

QColor getIncomingBodyColorB(const double alpha);

QColor getOutgoingBodyColorB(const double alpha);

QBrush getBodyBrush(const bool isOutgoing, const bool isSelected, const int theme_id);

int32_t getBubbleHeight();

int32_t getBorderRadius();

int32_t getTopPadding(const bool hasTopMargin);

int32_t getLeftPadding(const bool isOutgoing);

int32_t getRightPadding(const bool isOutgoing);

int32_t getTimeStatusMargin();

int32_t getAvatarSize();

int32_t getAvatarRightMargin();

int32_t getBubbleHorPadding();

int getLastReadAvatarSize();

int getLastReadAvatarMargin();

UI_MESSAGE_STYLE_NS_END