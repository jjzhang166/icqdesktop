#include "stdafx.h"

#include "LiveChatItemDelegate.h"
#include "LiveChatMembersControl.h"

#include "../contact_list/Common.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../types/chat.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace
{
    const int top_margin = 10;
    const int bottom_margin = 8;
    const int right_margin = 24;
    const int avatar_size = 80;
    const int avatar_left_margin = 24;
    const int avatar_right_margin = 8;

#ifdef __APPLE__
    const int spacing = 4;
    const int desc_spacing = 8;
    const int friends_spacing = 9;
    const int k_count_spacing = 9;
    const int avatars_spacing = 6;
#else
    const int spacing = 6;
    const int desc_spacing = 13;
    const int friends_spacing = 15;
    const int k_count_spacing = 12;
    const int avatars_spacing = 12;
#endif //__APPLE__

    const int max_desc_chars = 60;


    QString getChatNameStylesheet(const QString& color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
            .arg(Utils::scale_value(18))
            .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI))
            .arg(color);
    };

    QString getChatDescriptionStylesheet(const QString& color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
            .arg(Utils::scale_value(15))
            .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI))
            .arg(color);
    };

    QString getFriendsStylesheet(const QString& color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI_SEMIBOLD))
            .arg(Utils::scale_value(12))
            .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI_SEMIBOLD))
            .arg(color);
    };

    QString formatMembersCount(int members, int avatarsVisible)
    {
        int resultInt = members - avatarsVisible;
        if (resultInt <= 0)
            return QString();

        QString result = "+";
        if (resultInt > 999)
        {
            float resultFloat = (float)resultInt / float(1000);
            result += QString::number(resultFloat, 'f', 1);
            result += "k";
        }
        else
        {
            result += QVariant(resultInt).toString();
        }

        return result;
    }
}

namespace Logic
{
    LiveChatItemDelegate::LiveChatItemDelegate(QWidget* parent)
        : QItemDelegate(parent)
        , parent_(parent)
    {
    }


    void LiveChatItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QVariant data = index.model()->data(index);
        if (data.isValid())
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->translate(option.rect.topLeft());

            static QColor baseColor(255, 255, 255, 0.95 * 255);
            static QColor hoverColor("#ECEAE9");
            static QColor selectedColor("#D5E8C3");

            QSize size = sizeHint(option, index);
            if (option.state & QStyle::State_MouseOver)
            {
                static QBrush hoverBrush(hoverColor);
                painter->fillRect(0, 0, size.width(), size.height(), hoverBrush);
            }

            if (option.state & QStyle::State_Selected)
            {
                static QBrush selectedBrush(selectedColor);
                painter->fillRect(0, 0, size.width(), size.height(), selectedBrush);
            }

            Data::ChatInfo info = data.value<Data::ChatInfo>();
            static Ui::ContactAvatarWidget avatar(0, info.AimId_, info.Name_, Utils::scale_value(avatar_size), false);
            avatar.UpdateParams(info.AimId_, info.Name_);
            avatar.render(painter, QPoint(Utils::scale_value(avatar_left_margin), Utils::scale_value(top_margin)), QRegion(), QWidget::DrawChildren);

            static auto name = ContactList::CreateTextBrowser("chat_name", getChatNameStylesheet("#282828"), 0);
            name->clear();
            QTextCursor cursorName = name->textCursor();
            Logic::Text2Doc(info.Name_, cursorName, Logic::Text2DocHtmlMode::Pass, false);
            int margin = Utils::scale_value(avatar_size) + Utils::scale_value(avatar_left_margin) + Utils::scale_value(avatar_right_margin);
            int max_width = ContactList::ItemWidth(false, false, false).px() - margin - Utils::scale_value(right_margin);
            name->setFixedWidth(max_width);
            name->render(painter, QPoint(margin, Utils::scale_value(top_margin - spacing)));

            static auto description = ContactList::CreateTextBrowser("chat_info", getChatDescriptionStylesheet("#696969"), 0);
            description->clear();
            QTextCursor cursorDescription = description->textCursor();
            QString about = info.About_;
            if (about.length() > max_desc_chars)
                about = about.left(max_desc_chars) + "...";

            int lineCount = 0;
            int eol = about.indexOf(QChar::LineSeparator);
            while (eol != -1 && lineCount <= 2)
            {
                if (++lineCount == 3)
                {
                    about = about.left(eol);
                    break;
                }
            }

            Logic::Text2Doc(about, cursorDescription, Logic::Text2DocHtmlMode::Pass, false);
            description->setFixedWidth(max_width);
            description->render(painter, QPoint(margin, Utils::scale_value(top_margin) + name->document()->size().height() - Utils::scale_value(desc_spacing)));

            QString friendsInfoStr;
            if (!info.Location_.isEmpty())
            {
                friendsInfoStr = info.Location_;
            }
            if (info.FriendsCount != 0)
            {
                if (!friendsInfoStr.isEmpty())
                    friendsInfoStr += " - ";

                friendsInfoStr.append(QVariant(info.FriendsCount).toString() + " " +
                    Utils::GetTranslator()->getNumberString(
                    info.FriendsCount,
                    QT_TRANSLATE_NOOP3("livechats", "friend", "1"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "2"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "5"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "21")));
            }

            static auto friends = ContactList::CreateTextBrowser("friends", getFriendsStylesheet("#696969"), 0);
            friends->clear();
            if (!friendsInfoStr.isEmpty())
            {
                QTextCursor cursorFriend = friends->textCursor();
                Logic::Text2Doc(friendsInfoStr, cursorFriend, Logic::Text2DocHtmlMode::Pass, false);
                int margin = Utils::scale_value(avatar_size) + Utils::scale_value(avatar_left_margin) + Utils::scale_value(avatar_right_margin);
                int max_width = ContactList::ItemWidth(false, false, false).px() - margin - Utils::scale_value(right_margin);
                friends->setFixedWidth(max_width);
                friends->render(painter, QPoint(margin, Utils::scale_value(top_margin) + name->document()->size().height() + description->document()->size().height() - Utils::scale_value(friends_spacing)));
            }

            Ui::LiveChatMembersControl avatarsCollection_(parent_, std::make_shared<Data::ChatInfo>(info), 4);
            int y = Utils::scale_value(top_margin) + name->document()->size().height() + description->document()->size().height();
            if (!friendsInfoStr.isEmpty())
                y += friends->document()->size().height();

            y -= Utils::scale_value(avatars_spacing);

            avatarsCollection_.adjustWidth();

            if (option.state & QStyle::State_Selected)
                avatarsCollection_.setColor(selectedColor);
            else if (option.state & QStyle::State_MouseOver)
                avatarsCollection_.setColor(hoverColor);
            else
                avatarsCollection_.setColor(baseColor);

            avatarsCollection_.render(painter, QPoint(margin, y), QRegion(), QWidget::DrawChildren);

            int height = y + avatarsCollection_.height() + Utils::scale_value(bottom_margin);
            if (height_[index.row()] != height)
            {
                height_[index.row()] = height;
                emit const_cast<QAbstractItemModel*>(index.model())->dataChanged(index, index);
            }

            y += Utils::scale_value(k_count_spacing);

            static QLabel kCount_;
            kCount_.setText(formatMembersCount(info.MembersCount_, avatarsCollection_.getRealCount()));
            kCount_.setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(12)));
            QPalette p;
            p.setColor(QPalette::Foreground, QColor(0x69, 0x69, 0x69));
            kCount_.setPalette(p);
            kCount_.adjustSize();
            kCount_.render(painter, QPoint(margin + avatarsCollection_.width(), y), QRegion(), QWidget::DrawChildren);

            painter->restore();
        }
    }

    QSize LiveChatItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!height_.contains(index.row()))
            height_[index.row()] = 200;

        return QSize(ContactList::ItemWidth(false, false, false).px(), height_[index.row()]);
    }
}