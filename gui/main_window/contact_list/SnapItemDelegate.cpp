#include "stdafx.h"

#include "SnapItemDelegate.h"
#include "ContactList.h"
#include "ContactListModel.h"
#include "../../cache/snaps/SnapStorage.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/Text2DocConverter.h"

namespace
{
    Emoji::EmojiSizePx getEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
            case 100:
                emojiSize = Emoji::EmojiSizePx::_32;
                break;
            case 125:
                emojiSize = Emoji::EmojiSizePx::_40;
                break;
            case 150:
                emojiSize = Emoji::EmojiSizePx::_48;
                break;
            case 200:
                emojiSize = Emoji::EmojiSizePx::_64;
                break;
            default:
                assert(!"invalid scale");
        }
        
        return emojiSize;
    }
    
    const int SMILE_SIZE = 24;
}

namespace Logic
{
    SnapItemDelegate::SnapItemDelegate(QObject* parent)
        : QItemDelegate(parent)
        , fixedWidth_(-1)
    {
    }

    SnapItemDelegate::~SnapItemDelegate()
    {
    }

    void SnapItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        auto snap = index.data(Qt::DisplayRole).value<Logic::SnapItem>();

        const bool isSelected = (option.state & QStyle::State_Selected);
        const bool isHovered =  (option.state & QStyle::State_MouseOver);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(option.rect.topLeft());
        painter->setPen(QPen(Qt::transparent, 0));

        auto params = ContactList::GetContactListParams();
        auto r = QRect(0, 0, params.snapItemWidth(), params.snapItemHeight());
        painter->fillRect(r, Qt::white);

        painter->drawPixmap(params.snapLeftPadding(), params.snapTopPadding(), params.snapPreviewWidth(), params.snapPreviewHeight(), snap.Snap_);

        const auto fontQss = Fonts::appFontFullQss(Utils::scale_value(12), Fonts::defaultAppFontFamily(), Fonts::FontWeight::Normal);
        const auto styleSheetQss =
            QString("%1; color: %2; background-color: transparent;").arg(fontQss).arg(Utils::rgbaStringFromColor(QColor("#ffffff")));
        static auto textControl = ContactList::CreateTextBrowser("name", styleSheetQss, params.contactNameHeight());
        textControl->setWordWrapMode(QTextOption::NoWrap);
        textControl->setStyleSheet(styleSheetQss);
        textControl->setStyle(QApplication::style());
        auto &doc = *textControl->document();
        doc.clear();

        QString friendly = Logic::getContactListModel()->getDisplayName(snap.AimId_);
        if (snap.AimId_ == friendly && !snap.Friendly_.isEmpty())
            friendly = snap.Friendly_;

        auto maxWidth = params.snapItemWidth() - params.snapLeftPadding() * 2;
        QFontMetrics f(textControl->font());
        friendly = f.elidedText(friendly, Qt::ElideRight, maxWidth);
        textControl->setFixedWidth(maxWidth);

        QTextCursor cursor = textControl->textCursor();

        Logic::Text2Doc(friendly, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, Emoji::EmojiSizePx::_16);
        Logic::FormatDocument(doc, params.contactNameHeight());

        auto textBlockFormat = cursor.blockFormat();
        textBlockFormat.setAlignment(Qt::AlignCenter);
        cursor.mergeBlockFormat(textBlockFormat);
        textControl->setTextCursor(cursor);

        textControl->render(painter, QPoint(params.snapLeftPadding(), params.snapItemHeight() - params.snapNameBottomPadding() - textControl->height()));

        if (snap.HaveNewSnap_)
        {
            auto fire = Emoji::GetEmoji(0x1f525, 0x0, getEmojiSize());
            painter->drawImage(QRect(params.snapPreviewWidth() - Utils::scale_value(SMILE_SIZE) / 2 - Utils::scale_value(platform::is_apple() ? 2 : 4), params.snapTopPadding() / 2, Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE + (platform::is_apple() ? 0 : 2))), fire, fire.rect());
        }

        painter->restore();
    }

    QSize SnapItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        return getSnapItemSize();
    }

    QSize SnapItemDelegate::getSnapItemSize()
    {
        auto params = ContactList::GetContactListParams();
        return QSize(params.snapItemWidth(), params.snapItemHeight());
    }

    QSize SnapItemDelegate::getSnapPreviewItemSize()
    {
        auto params = ContactList::GetContactListParams();
        return QSize(params.snapPreviewWidth(), params.snapItemHeight());
    }

    int SnapItemDelegate::getGradientHeight()
    {
        auto params = ContactList::GetContactListParams();
        return params.snapGradientHeight();
    }
}
