#pragma once

#define UI_MESSAGE_STYLE_NS_BEGIN namespace Ui { namespace MessageStyle {
#define UI_MESSAGE_STYLE_NS_END } }

UI_MESSAGE_STYLE_NS_BEGIN

QColor getIncomingBodyColorA(const double alpha);

QColor getOutgoingBodyColorA(const double alpha);

QColor getIncomingBodyColorB(const double alpha);

QColor getOutgoingBodyColorB(const double alpha);

QBrush getBodyBrush(const bool isOutgoing, const bool isSelected, int theme_id);

UI_MESSAGE_STYLE_NS_END