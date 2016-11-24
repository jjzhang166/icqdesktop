#include "stdafx.h"

#include "../../../controls/TextEmojiWidget.h"

#include "../../../controls/PictureWidget.h"

#include "../../../controls/ContactAvatarWidget.h"

#include "../MessageStyle.h"

#include "QuoteBlock.h"

#include "QuoteBlockLayout.h"

#include "Style.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlockLayout::QuoteBlockLayout()
{

}

QuoteBlockLayout::~QuoteBlockLayout()
{

}

const IItemBlockLayout::IBoxModel& QuoteBlockLayout::getBlockBoxModel() const
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

QRect QuoteBlockLayout::getBlockGeometry() const
{
    auto &block = *blockWidget<QuoteBlock>();
    return block.Geometry_;
}

QSize QuoteBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
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

QRect QuoteBlockLayout::evaluateContentLtr(const QRect &widgetGeometry) const
{
    return widgetGeometry;
}

QRect QuoteBlockLayout::setTextControlGeometry(const QRect &contentLtr)
{
    assert(contentLtr.width() > 0);

    auto &block = *blockWidget<QuoteBlock>();

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
    }

    QRect avatarGeometry(QPoint(contentLtr.topLeft().x() + Style::getQuoteOffsetLeft() + Style::getForwardIconOffset(), contentLtr.topLeft().y() + Style::getQuoteLineTopMargin()),
        QSize(Utils::unscale_value(20), Utils::unscale_value(20)));

    const QSize textSize(textWidth, textCtrl.height());

    QRect textCtrlGeometry(
                                 QPoint(contentLtr.topLeft().x() + Style::getQuoteAvatarOffset() + Style::getQuoteOffsetLeft() + Style::getForwardIconOffset(), contentLtr.topLeft().y() + Style::getQuoteLineTopMargin() + Style::getQuoteAvatarOffsetTop()),
        textSize);

    if (block.needForwardBlock())
    {
        const QRect forwardIconRect(
            QPoint(contentLtr.topLeft().x() - Style::getForwardIconOffset(), contentLtr.topLeft().y() + Style::getQuoteLineTopMargin())
            , block.ForwardIcon_->size());
        block.ForwardIcon_->setGeometry(forwardIconRect);

        const QRect forwardLabelGeometry(
            QPoint(contentLtr.topLeft().x() + Style::getQuoteOffsetLeft() + Style::getForwardLabelOffset(), contentLtr.topLeft().y() + Style::getQuoteLineTopMargin()),
            block.ForwardLabel_->size());
        block.ForwardLabel_->setGeometry(forwardLabelGeometry);

        textCtrlGeometry.moveTop(forwardLabelGeometry.bottomLeft().y() + Style::getForwardLabelBottomMargin() + Style::getQuoteAvatarOffsetTop());

        avatarGeometry.moveTop(forwardLabelGeometry.bottomLeft().y() + Style::getForwardLabelBottomMargin());
    }

    const auto geometryChanged = (textCtrlGeometry != CurrentTextCtrlGeometry_);
    if (geometryChanged)
    {
        assert(widthChanged);

        textCtrl.setGeometry(textCtrlGeometry);

        block.Avatar_->setGeometry(avatarGeometry);

        CurrentTextCtrlGeometry_ = textCtrlGeometry;
    }

    return textCtrlGeometry;
}

UI_COMPLEX_MESSAGE_NS_END