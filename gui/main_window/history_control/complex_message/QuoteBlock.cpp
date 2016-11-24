#include "stdafx.h"

#include "ComplexMessageItem.h"
#include "../MessageStyle.h"
#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../controls/PictureWidget.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../my_info.h"
#include "../../contact_list/ContactListModel.h"

#include "QuoteBlockLayout.h"
#include "Selection.h"
#include "Style.h"
#include "QuoteBlock.h"
#include "TextBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlock::QuoteBlock(ComplexMessageItem *parent, const Data::Quote& quote)
    : GenericBlock(parent, quote.senderFriendly_, MenuFlagNone, false)
    , Quote_(quote)
    , Layout_(nullptr)
    , TextCtrl_(nullptr)
    , ForwardLabel_(nullptr)
    , Selection_(BlockSelectionType::None)
    , Parent_(parent)
    , ReplyBlock_(nullptr)
    , ForwardIcon_(nullptr)
{
    Layout_ = new QuoteBlockLayout();
    setLayout(Layout_);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    ForwardLabel_ = new QLabel(this);
    ForwardLabel_->setFont(Fonts::appFontScaled(15));
    QPalette p;
    p.setColor(QPalette::Foreground, QColor("#579e1c"));
    ForwardLabel_->setPalette(p);
    ForwardLabel_->setText(QT_TRANSLATE_NOOP("chat_page", "forwarded messages"));
    ForwardLabel_->adjustSize();
    ForwardLabel_->setVisible(needForwardBlock());
    ForwardIcon_ = new PictureWidget(this, ":/resources/content_forwardmsg_100.png");
    ForwardIcon_->setVisible(needForwardBlock());
    ForwardIcon_->setFixedSize(Utils::scale_value(24), Utils::scale_value(24));
}

QuoteBlock::~QuoteBlock()
{

}

void QuoteBlock::clearSelection()
{
    for (auto b : Blocks_)
    {
        b->clearSelection();
    }
    update();
}

IItemBlockLayout* QuoteBlock::getBlockLayout() const
{
    return Layout_;
}

QString QuoteBlock::getSelectedText(bool isFullSelect) const
{
    QString result;
    for (auto b : Blocks_)
    {
        if (!b->isSelected())
            continue;

        if (isFullSelect)
            result += QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));

        QString source = b->getSelectedText();
        result += source;
        result += QChar::LineFeed;
    }

    return result;
}

QString QuoteBlock::getSourceText() const
{
    QString result;
    int i = 0;
    for (auto b : Blocks_)
    {
        ++i;
        result += QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));
        QString source = b->getSourceText();
        result += source;
        result += QChar::LineFeed;
    }
    
    return result;
}

QString QuoteBlock::formatRecentsText() const
{
    if (standaloneText())
        return ReplyBlock_ ? ReplyBlock_->formatRecentsText() : QString();

    QString result;
    for (auto b : Blocks_)
    {
        auto format = QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));
        format += b->getSourceText();
        result += format;
        result += QChar::LineFeed;
    }
    return result;
}

bool QuoteBlock::standaloneText() const
{
    return !quoteOnly() && !Quote_.isForward_;
}

Data::Quote QuoteBlock::getQuote() const
{
    Data::Quote quote;
    if (quoteOnly() || Quote_.isForward_)
        quote = Quote_;
    else
        quote.id_ = Quote_.id_;

   return quote;
}

IItemBlock* QuoteBlock::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        assert(block);
        if (!block)
        {
            continue;
        }

        const auto blockLayout = block->getBlockLayout();
        assert(blockLayout);

        if (!blockLayout)
        {
            continue;
        }

        const auto &blockGeometry = blockLayout->getBlockGeometry();

        const auto topY = blockGeometry.top();
        const auto bottomY = blockGeometry.bottom();
        const auto posY = pos.y();

        const auto isPosOverBlock = ((posY >= topY) && (posY <= bottomY));
        if (isPosOverBlock)
        {
            return block;
        }
    }

    return nullptr;
}

bool QuoteBlock::hasRightStatusPadding() const
{
    return true;
}

bool QuoteBlock::isSelected() const
{
    for (auto b : Blocks_)
    {
        if (b->isSelected())
            return true;
    }
    
    return false;
}

void QuoteBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    for (auto b : Blocks_)
    {
        b->selectByPos(from, to, selection);
    }
}

bool QuoteBlock::needForwardBlock() const
{
    return Quote_.isForward_ && Quote_.isFirstQuote_;
}

void QuoteBlock::setReplyBlock(GenericBlock* block)
{
    if (!ReplyBlock_)
        ReplyBlock_ = block;
}

void QuoteBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onVisibilityChanged(isVisible);
    }
}

QRect QuoteBlock::setBlockGeometry(const QRect &ltr)
{
    QRect b = ltr;
    b.moveLeft(b.x() + Style::getQuoteOffsetLeft());
    QRect r = b;
    b.moveTop(b.y() + Style::getQuoteOffsetTop());
    b.moveTopLeft(GenericBlock::setBlockGeometry(b).bottomLeft());

    if (needForwardBlock())
       b.moveTop(b.y() + ForwardLabel_->height() + Style::getForwardLabelBottomMargin());

    int i = 1;
    for (auto block : Blocks_)
    {
        block->setMaxPreviewWidth(Style::getMaxImageWidthInQuote());
        auto blockGeometry = block->setBlockGeometry(b);
        auto bl = QPoint(blockGeometry.bottomLeft().x(), blockGeometry.bottomLeft().y() + Style::getQuoteBlockSpacing());
        b.moveTopLeft(bl);
        r.setBottomLeft(i == Blocks_.size() ? blockGeometry.bottomLeft() : bl);
        ++i;
    }
    r.setBottom(r.bottom() + Style::getQuoteBottomSpace());
    r.moveLeft(r.x() - Style::getQuoteOffsetLeft());
    r.moveLeft(r.x() - Style::getForwardIconOffset());

    if (!quoteOnly())
    {
        if (Quote_.isLastQuote_)
            r.setHeight(r.height() + Style::getQuoteOffsetBottom());
        else
            r.setHeight(r.height() + Style::getTextQuoteOffset());
    }

    setGeometry(r);
    r.setHeight(r.height() - Style::getQuoteSpacing());
    Geometry_ = r;
    return r;
}

void QuoteBlock::onActivityChanged(const bool isVisible)
{
    GenericBlock::onActivityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onActivityChanged(isVisible);
    }
}

void QuoteBlock::addBlock(GenericBlock* block)
{
    block->setBubbleRequired(false);
    block->setFontSize(Utils::scale_value(14));
    block->setTextOpacity(0.8);
    Blocks_.push_back(block);

    connect(block, SIGNAL(clicked()), this, SLOT(blockClicked()), Qt::QueuedConnection);
}

bool QuoteBlock::quoteOnly() const
{
    return !ReplyBlock_;
}

void QuoteBlock::blockClicked()
{
//     if (!Quote_.isForward_)
//     {
//         Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), Quote_.msgId_, true);
//     }
}

void QuoteBlock::drawBlock(QPainter &p)
{
    auto pen = Style::getQuoteSeparatorPen();
    p.save();
    p.setPen(pen);
    auto bl = rect().bottomLeft();
    bl.setX(bl.x() + Style::getLineOffset());
    if (Quote_.isLastQuote_ && !quoteOnly())
        bl = QPoint(rect().bottomLeft().x() + Style::getLineOffset(), rect().bottomLeft().y() - Style::getQuoteOffsetBottom());
    
    auto tl = rect().topLeft();
    tl.setX(tl.x() + Style::getLineOffset());
    if (Quote_.isFirstQuote_)
    {
        tl.setY(tl.y() + Style::getFirstQuoteOffset());
        if (needForwardBlock())
          tl.setY(tl.y() + ForwardLabel_->height() + Style::getForwardLabelBottomMargin());
    }

    if (Quote_.isLastQuote_)
        bl.setY(bl.y() - Style::getQuoteLineBottomMargin());

    p.drawLine(tl, bl);
    p.restore();
}

void QuoteBlock::initialize()
{
    GenericBlock::initialize();

    TextCtrl_ = new TextEmojiWidget(this, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(12), QColor("#579e1c"));

    Avatar_ = new ContactAvatarWidget(this, Quote_.senderId_, Quote_.senderFriendly_, Utils::scale_value(20), true);
    
    TextCtrl_->setText(Quote_.senderFriendly_);
    TextCtrl_->show();
    Avatar_->show();
}

bool QuoteBlock::replaceBlockWithSourceText(IItemBlock *block)
{
    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);
    
    if (iter == Blocks_.end())
    {
        return false;
    }
    
    auto &existingBlock = *iter;
    assert(existingBlock);
    
    auto textBlock = new TextBlock(Parent_, existingBlock->getSourceText());
    
    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);
    
    textBlock->show();
    
    connect(textBlock, SIGNAL(clicked()), this, SLOT(blockClicked()), Qt::QueuedConnection);

    existingBlock->deleteLater();
    existingBlock = textBlock;
    
    return true;
}

bool QuoteBlock::isSharingEnabled() const
{
    return false;
}

bool QuoteBlock::containSharingBlock() const
{
    for (auto b : Blocks_)
    {
        if (b->isSharingEnabled())
            return true;
    }

    return false;
}

UI_COMPLEX_MESSAGE_NS_END
