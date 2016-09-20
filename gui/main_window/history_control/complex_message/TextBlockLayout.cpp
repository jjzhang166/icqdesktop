#include "stdafx.h"

#include "../../../controls/TextEditEx.h"

#include "../MessageStyle.h"

#include "TextBlock.h"

#include "TextBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

TextBlockLayout::TextBlockLayout()
{

}

TextBlockLayout::~TextBlockLayout()
{

}

const IItemBlockLayout::IBoxModel& TextBlockLayout::getBlockBoxModel() const
{
    static const QMargins bubbleMargins(
        0,
        Utils::scale_value(4),
        0,
        Utils::scale_value(8)
    );

    static const BoxModel boxModel(
        false,
        bubbleMargins);

    return boxModel;

}

QSize TextBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
{
    QLayout::setGeometry(widgetGeometry);

    const auto contentLtr = evaluateContentLtr(widgetGeometry);

    const auto enoughSpace = (contentLtr.width() > 0);
    if (!enoughSpace)
    {
        return widgetGeometry.size();
    }

    const auto textCtrlGeometry = setTextControlGeometry(contentLtr);

    return QSize(
        widgetGeometry.width(),
        textCtrlGeometry.height());
}

QRect TextBlockLayout::evaluateContentLtr(const QRect &widgetGeometry) const
{
    return widgetGeometry;
}

QRect TextBlockLayout::setTextControlGeometry(const QRect &contentLtr)
{
    assert(contentLtr.width() > 0);

    auto &block = *blockWidget<TextBlock>();

    if (!block.TextCtrl_)
    {
        return contentLtr;
    }

    auto &textCtrl = *block.TextCtrl_;

    const auto textWidth = MessageStyle::roundTextWidthDown(contentLtr.width());

    const auto widthChanged = (textWidth != CurrentTextCtrlGeometry_.width());
    if (widthChanged)
    {
        textCtrl.setFixedWidth(textWidth);
        textCtrl.document()->setTextWidth(textWidth);
    }

    const QSize textSize(textWidth, textCtrl.getTextHeight());

    const QRect textCtrlGeometry(
        contentLtr.topLeft(),
        textSize);

    const auto geometryChanged = (textCtrlGeometry != CurrentTextCtrlGeometry_);
    if (geometryChanged)
    {
        assert(widthChanged);

        textCtrl.setGeometry(textCtrlGeometry);

        CurrentTextCtrlGeometry_ = textCtrlGeometry;
    }

    return textCtrlGeometry;
}

UI_COMPLEX_MESSAGE_NS_END