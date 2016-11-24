#include "stdafx.h"

#include "RecentsItemRenderer.h"
#include "Common.h"
#include "RecentsModel.h"
#include "ContactList.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"

namespace ContactList
{
    RecentItemVisualData::RecentItemVisualData(
        const QString &aimId,
        const QPixmap &avatar,
        const QString &state,
        const QString &status,
        const bool isHovered,
        const bool isSelected,
        const QString &contactName,
        const bool haveLastSeen,
        const QDateTime &lastSeen,
        const int unreadsCounter,
        const bool muted,
        const QString &senderNick,
        const bool isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const bool isTyping,
        const DeliveryState deliveryState,
        const QString term,
        const bool haveLastMsg,
        qint64 _msg_id)
        : VisualDataBase(aimId, avatar, state, status, isHovered, isSelected, contactName, haveLastSeen, lastSeen, false /*_isWithCheckBox*/
            , false /* _isChatMember */, isOfficial, _drawLastRead, _lastReadAvatar, QString() /* role */, unreadsCounter, term)
        , DeliveryState_(deliveryState)
        , Muted_(muted)
        , senderNick_(senderNick)
        , IsTyping_(isTyping)
        , HaveLastMsg_(haveLastMsg)
        , msg_id_(_msg_id)
    {
        assert(deliveryState >= DeliveryState::Min);
        assert(deliveryState <= DeliveryState::Max);
        assert(unreadsCounter >= 0);
    }

    void RenderRecentsItem(QPainter &painter, const RecentItemVisualData &item, const ViewParams& viewParams)
    {
        auto contactListPxInRecents = GetRecentsParams(viewParams.regim_);
        painter.save();

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(painter, item.IsHovered_, item.IsSelected_, contactListPxInRecents, viewParams);
        RenderAvatar(painter, contactListPxInRecents.avatarX().px(), item.Avatar_, contactListPxInRecents);

        int rightBorderPx = CorrectItemWidth(ItemWidth(viewParams).px(), viewParams.fixedWidth_) - contactListPxInRecents.itemHorPadding().px();

        if (viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT && viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN)
            {
                if (!item.unreadsCounter_)
                {
                    if (!viewParams.pictOnly_)
                        rightBorderPx = RenderAddContact(painter, true, contactListPxInRecents, viewParams);
                }
                else
                {
                    rightBorderPx = RenderNotifications(painter, item.unreadsCounter_, false /* muted */, viewParams, contactListPxInRecents, false /* isUnknownHeader */);
                }
            }
            else
            {
                if (!item.drawLastRead_ || item.Muted_)
                {
                    rightBorderPx = RenderNotifications(painter, item.unreadsCounter_, item.Muted_, viewParams, contactListPxInRecents, false /* isUnknownHeader */);
                }
            }
        }

        if (viewParams.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH && item.HaveLastMsg_)
            rightBorderPx = RenderDate(painter, item.LastSeen_, item, contactListPxInRecents, viewParams);

        if (viewParams.pictOnly_)
        {
            painter.restore();
            return;
        }

        RenderContactName(painter, item, contactListPxInRecents.nameY().px(), rightBorderPx, viewParams, contactListPxInRecents);

        const int lastReadLeftMargin = Utils::scale_value(4);
        const int lastReadRightMargin = Utils::scale_value(4);
        const int lastReadWidth = contactListPxInRecents.getLastReadAvatarSize() + lastReadRightMargin + lastReadLeftMargin;

        rightBorderPx -= (item.drawLastRead_ ? lastReadWidth : 0);

        const int messageWidth = RenderContactMessage(painter, item, rightBorderPx, viewParams, contactListPxInRecents);

        if (viewParams.regim_ == Logic::MembersWidgetRegim::UNKNOWN)
        {
            RenderRemoveContact(painter, item.IsHovered_, contactListPxInRecents, viewParams);
            painter.restore();
            return;
        }

        const auto showLastMessage = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);

        if (item.drawLastRead_ && !item.IsTyping_ && showLastMessage)
        {
            RenderLastReadAvatar(painter, item.lastReadAvatar_, contactListPxInRecents.messageX().px() + messageWidth + lastReadLeftMargin, contactListPxInRecents);
        }

        painter.restore();
    }

    void RenderRecentsDragOverlay(QPainter &painter, const ViewParams& viewParams)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);
        auto recentParams = GetRecentsParams(viewParams.regim_);

        auto width = CorrectItemWidth(ItemWidth(viewParams).px(), viewParams.fixedWidth_);
        painter.fillRect(0, 0, width, recentParams.itemHeight().px(), QBrush(QColor(255, 255, 255, 255 * 0.9)));
        painter.setBrush(QBrush(QColor(255, 255, 255, 0)));;
        QPen pen (QColor(0x57, 0x9e, 0x1c), recentParams.dragOverlayBorderWidth().px(), Qt::DashLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawRoundedRect(
            recentParams.dragOverlayPadding().px(),
            recentParams.dragOverlayVerPadding().px(),
            width - recentParams.itemHorPadding().px() - Utils::scale_value(1),
            recentParams.itemHeight().px() - recentParams.dragOverlayVerPadding().px(),
            recentParams.dragOverlayBorderRadius().px(),
            recentParams.dragOverlayBorderRadius().px()
            );

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = width / 2 - p.width() / 2. / ratio;
        int y = (recentParams.itemHeight().px() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);

        painter.restore();
    }

    void RenderServiceItem(QPainter &painter, const QString& text, bool renderState, bool drawLine, const ViewParams& viewParams)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        QPen pen;
        pen.setColor(QColor(0x57,0x9e,0x1c));
        painter.setPen(pen);

        auto font = Fonts::appFontScaled(12, Fonts::defaultAppFontFamily(), Fonts::FontWeight::Bold);
        painter.setFont(font);

        auto recentParams = GetRecentsParams(viewParams.regim_);

        if (!viewParams.pictOnly_)
        {
            Utils::drawText(painter, QPointF(recentParams.itemHorPadding().px(), recentParams.serviceItemHeight().px() / 2), Qt::AlignVCenter, text);
        }
        else if (drawLine)
        {
            painter.save();
            QPen line_pen;
            line_pen.setColor(QColor("#dbdbdb"));
            painter.setPen(line_pen);
            int y = recentParams.serviceItemHeight().px() / 2;
            painter.drawLine(0, y, recentParams.avatarW().px() + 2 * recentParams.itemHorPadding().px(), y);
            painter.restore();
        }

        if (renderState)
        {
            QPixmap p(Utils::parse_image_name(Logic::getRecentsModel()->isFavoritesVisible() ? ":/resources/cl_group_close_100.png" : ":/resources/cl_group_open_100.png"));
            Utils::check_pixel_ratio(p);
            double ratio = Utils::scale_bitmap(1);
            QFontMetrics m(font);
            int x = recentParams.itemHorPadding().px() + m.width(text) + recentParams.favoritesStatusPadding().px();
            int y = recentParams.serviceItemHeight().px() / 2 - (p.height() / 2. / ratio) + Utils::scale_value(1);
            painter.drawPixmap(x, y, p);
        }

        painter.restore();
    }

    void RenderUnknownsHeader(QPainter &painter, const QString& title, const int count, const ViewParams& viewParams)
    {
        auto recentParams = GetRecentsParams(viewParams.regim_);
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        if (!viewParams.pictOnly_)
        {
            QPen pen;
            pen.setColor(Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor()));
            painter.setPen(pen);

            QFont f;
            if (count)
            {
                f = Fonts::appFontScaled(16, Fonts::FontWeight::Semibold);
            }
            else
            {
                f = Fonts::appFontScaled(16);
            }
            painter.setFont(f);
            QFontMetrics metrics(f);
            Utils::drawText(painter, QPointF(recentParams.itemHorPadding().px(), recentParams.unknownsItemHeight().px() / 2. - dip(4).px()), Qt::AlignVCenter, title);
        }
        else
        {
            QPixmap pict(Utils::parse_image_name(":/resources/sidebar_allmembers_100.png"));
            Utils::check_pixel_ratio(pict);
            painter.drawPixmap(recentParams.avatarX().px() + recentParams.avatarW().px() / 2 - pict.width() / 2, recentParams.avatarY().px(), pict);
        }

        painter.restore();

        if (count)
        {
            painter.save();

            painter.setBrush(Qt::NoBrush);
            painter.setPen(Qt::NoPen);
            painter.setRenderHint(QPainter::Antialiasing);

            RenderNotifications(painter, count, false, viewParams, recentParams, true /* isUnknownHeader */);
            painter.restore();
        }
    }
}

namespace ContactList
{
    int RenderContactMessage(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx, const ViewParams& viewParams, ContactListParams& _recentParams)
    {
        if (!visData.HasStatus())
        {
            return 0;
        }

        static auto plainTextControl = CreateTextBrowser("message", _recentParams.getMessageStylesheet(false), _recentParams.messageHeight().px());
        static auto unreadsTextControl = CreateTextBrowser("messageUnreads", _recentParams.getMessageStylesheet(true), _recentParams.messageHeight().px());

        const auto hasUnreads = (viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT
            && viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH
            && (visData.unreadsCounter_ > 0));
        auto &textControl = (hasUnreads ? unreadsTextControl : plainTextControl);

        const auto maxWidth = (rightBorderPx - _recentParams.messageX().px());
        textControl->setFixedWidth(maxWidth);
        textControl->setWordWrapMode(QTextOption::WrapMode::NoWrap);

        auto messageTextMaxWidth = maxWidth;

        const auto font = textControl->font();

        auto &doc = *textControl->document();
        doc.clear();

        auto cursor = textControl->textCursor();

        const auto &senderNick = visData.senderNick_;
        const auto showLastMessage = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);

        if (!showLastMessage && viewParams.regim_ != Logic::MembersWidgetRegim::FROM_ALERT && viewParams.regim_ != Logic::MembersWidgetRegim::HISTORY_SEARCH)
            return -1;

        if (!senderNick.isEmpty() && !visData.IsTyping_ && showLastMessage)
        {
            static const QString fix(": ");

            QString fixedNick;
            fixedNick.reserve(senderNick.size() + fix.length());
            fixedNick.append(senderNick.simplified());

            fixedNick.append(fix);

            const auto charFormat = cursor.charFormat();

            auto boldCharFormat = charFormat;

            const auto boldFontSize = 15;
            boldCharFormat.setFont(Fonts::appFontScaled(boldFontSize, Fonts::FontWeight::Normal));

            cursor.setCharFormat(boldCharFormat);

            Logic::Text2Doc(
                fixedNick,
                cursor,
                Logic::Text2DocHtmlMode::Pass,
                false);

            cursor.setCharFormat(charFormat);

            messageTextMaxWidth -= doc.idealWidth();
        }

        const auto text = visData.GetStatus().trimmed().simplified();

        const auto boldFontSize = 15;
        auto base_font = Fonts::appFontScaled(boldFontSize, Fonts::FontWeight::Semibold);
        auto term_font = Fonts::appFontScaled(boldFontSize, Fonts::FontWeight::Semibold);

        QFontMetrics m(font);
        QFontMetrics term_m(term_font);

        auto term = visData.searchTerm_;
        auto elidedText = text;
        int begin_term = term == "" ? 0 : text.indexOf(term, 0, Qt::CaseInsensitive);

        QString left_part;
        QString right_part;
        QString term_visible_part;

        if (term != "" && begin_term != -1)
        {
            static QString stat_term = "";
            static QHash<qint64, std::pair<int, std::vector<QString>>> words_parts_and_width;

            if (stat_term != term)
            {
                stat_term = term;
                words_parts_and_width.clear();
            }

            auto need_update = false;
            if (words_parts_and_width.count(visData.msg_id_) == 0)
            {
                need_update = true;
            }
            else if (words_parts_and_width[visData.msg_id_].first != messageTextMaxWidth)
            {
                words_parts_and_width.remove(visData.msg_id_);
                need_update = true;
            }

            if (need_update)
            {
                Logic::CutText(text, term, messageTextMaxWidth, m, term_m, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, left_part, right_part, term_visible_part);

                std::vector<QString> parts;
                parts.push_back(left_part);
                parts.push_back(right_part);
                parts.push_back(term_visible_part);
                words_parts_and_width.insert(visData.msg_id_, std::make_pair(messageTextMaxWidth, parts));
            }
            else
            {
                auto width_and_parts = words_parts_and_width[visData.msg_id_];
                left_part = width_and_parts.second[0];
                right_part = width_and_parts.second[1];
                term_visible_part = width_and_parts.second[2];
            }

        }
        else
        {
            left_part = m.elidedText(text, Qt::ElideRight, messageTextMaxWidth);
        }

        {
            const auto charFormat = cursor.charFormat();
            auto boldCharFormat = charFormat;
            boldCharFormat.setFont(hasUnreads ? base_font : term_font);
            boldCharFormat.setForeground(QBrush(QColor("#579e1c")));

            Logic::Text2Doc(left_part, cursor, Logic::Text2DocHtmlMode::Pass, false);

            cursor.setCharFormat(boldCharFormat);
            Logic::Text2Doc(term_visible_part, cursor, Logic::Text2DocHtmlMode::Pass, false);

            cursor.setCharFormat(hasUnreads ? boldCharFormat : charFormat);
            Logic::Text2Doc(right_part, cursor, Logic::Text2DocHtmlMode::Pass, false);
        }

        Logic::FormatDocument(doc, _recentParams.messageHeight().px());

        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();
            qreal correction = ((realHeight > 17) ? 0 : 4);

            textControl->render(
                &painter,
                QPoint(
                    _recentParams.messageX().px(),
                     _recentParams.messageY().px() + correction));
        }
        else
        {
            textControl->render(&painter, QPoint(_recentParams.messageX().px(), _recentParams.messageY().px()));
        }

        return doc.idealWidth();
    }

    void RenderLastReadAvatar(QPainter &painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams)
    {
        painter.drawPixmap(_xOffset, _recentParams.lastReadY().px(), _recentParams.getLastReadAvatarSize(), _recentParams.getLastReadAvatarSize(), _avatar);
    }

    int RenderNotifications(QPainter &painter, const int unreads, bool muted, const ViewParams& viewParams, ContactListParams& _recentParams, bool _isUnknownHeader)
    {
        auto width = CorrectItemWidth(ItemWidth(viewParams).px(), viewParams.fixedWidth_);
        assert(unreads >= 0);

        if (muted)
        {
            if (!viewParams.pictOnly_)
            {
                Themes::IThemePixmapSptr pixmap = Themes::GetPixmap(unreads == 0 ? Themes::PixmapResourceId::ContentMuteNotify : Themes::PixmapResourceId::ContentMuteNotifyNew);
                auto mutedX = (width - _recentParams.itemHorPadding().px()) - pixmap->GetWidth();
                pixmap->Draw(painter, mutedX, _recentParams.unreadsY().px());
                return ((width - _recentParams.itemHorPadding().px()) - pixmap->GetWidth() - _recentParams.unreadsLeftPadding().px());
            }
            else
            {
                return width - _recentParams.itemHorPadding().px();
            }
        }

        if (unreads <= 0)
        {
            return width - _recentParams.itemHorPadding().px();
        }

        const auto text = (unreads > 99) ? QString("99+") : QVariant(unreads).toString();

        static QFontMetrics m(_recentParams.unreadsFont().font());

        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
        const auto unreadsHeight = unreadsRect.height();

        auto balloonWidth = unreadsWidth;
        const auto isLongText = (text.length() > 1);
        if (isLongText)
        {
            balloonWidth += (_recentParams.unreadsPadding().px() * 2);
        }
        else
        {
            balloonWidth = _recentParams.unreadsMinimumExtent().px();
        }

        const auto ballonHeight = _recentParams.unreadsMinimumExtent().px();

        auto unreadsX = ((width - (viewParams.regim_ != Logic::MembersWidgetRegim::UNKNOWN ? _recentParams.itemHorPadding().px() : _recentParams.itemHorPaddingUnknown().px())) - balloonWidth);
        const auto balloonRadius = (ballonHeight / 2);
        auto unreadsY = _isUnknownHeader ? _recentParams.unknownsUnreadsY().px() : _recentParams.unreadsY().px();

        if (viewParams.pictOnly_)
        {
            unreadsX = _recentParams.itemHorPadding().px();
            unreadsY = (_recentParams.itemHeight().px() - _recentParams.avatarH().px()) / 2;
            auto gap = Utils::scale_value(2);
            painter.setBrush(QColor("#FFFFFF"));
            painter.drawRoundedRect(unreadsX - gap, unreadsY - gap, balloonWidth + 2 * gap, ballonHeight + 2 * gap, balloonRadius + gap, balloonRadius + gap);
        }
        else if (viewParams.regim_ == Logic::MembersWidgetRegim::UNKNOWN)
        {
            unreadsX -= _recentParams.interPadding().px() + _recentParams.removeContactSize().px();
        }

        painter.setBrush(QColor("#579e1c"));
        painter.drawRoundedRect(unreadsX, unreadsY, balloonWidth, ballonHeight, balloonRadius, balloonRadius);

        painter.setFont(_recentParams.unreadsFont().font());
        painter.setPen(Qt::white);
        if (platform::is_apple())
        {
            painter.drawText(QRectF(unreadsX, unreadsY, balloonWidth, ballonHeight), text, QTextOption(Qt::AlignCenter));
        }
        else
        {
            const float textX = floorf((float)unreadsX + ((float)balloonWidth - (float)unreadsWidth) / 2.);
            const float textY = ceilf((float)unreadsY + ((float)ballonHeight + (float)unreadsHeight) / 2.);
            painter.drawText(textX, textY, text);
        }

        return (unreadsX - _recentParams.unreadsLeftPadding().px());
    }

    int RenderAddContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams)
    {
        auto img = Utils::parse_image_name(hasMouseOver ? ":/resources/cl_addcontact_100_hover.png" : ":/resources/cl_addcontact_100.png");
        _recentParams.addContactFrame().setX(CorrectItemWidth(ItemWidth(false, false, false).px(), _viewParams.fixedWidth_)
            - _recentParams.itemHorPaddingUnknown().px() - _recentParams.addContactSize().px()
            - _recentParams.interPadding().px() - _recentParams.removeContactSize().px());
        _recentParams.addContactFrame().setY((_recentParams.itemHeight().px() - _recentParams.addContactSize().px()) / 2);
        _recentParams.addContactFrame().setWidth(_recentParams.addContactSize().px());
        _recentParams.addContactFrame().setHeight(_recentParams.addContactSize().px());
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(_recentParams.addContactFrame().x(),
            _recentParams.addContactFrame().y(),
            _recentParams.addContactFrame().width(),
            _recentParams.addContactFrame().height(),
            img);
        painter.restore();
        return _recentParams.addContactFrame().x();
    }

    int RenderRemoveContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams)
    {
        auto img = Utils::parse_image_name(hasMouseOver ? ":/resources/contr_clear_100_hover.png" : ":/resources/contr_clear_100.png");
        _recentParams.removeContactFrame().setX(CorrectItemWidth(ItemWidth(false, false, false).px(), _viewParams.fixedWidth_) - _recentParams.itemHorPaddingUnknown().px() - _recentParams.removeContactSize().px());
        _recentParams.removeContactFrame().setY((_recentParams.itemHeight().px() - _recentParams.removeContactSize().px()) / 2);
        _recentParams.removeContactFrame().setWidth(_recentParams.removeContactSize().px());
        _recentParams.removeContactFrame().setHeight(_recentParams.removeContactSize().px());
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(_recentParams.removeContactFrame().x(),
            _recentParams.removeContactFrame().y(),
            _recentParams.removeContactFrame().width(),
            _recentParams.removeContactFrame().height(),
            img);
        painter.restore();
        return _recentParams.removeContactFrame().x();
    }

    void RenderDeleteAllItem(QPainter &painter, const QString& title, bool isMouseOver, const ViewParams& _viewParams)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPen pen;
        pen.setColor(isMouseOver ? Ui::CommonStyle::getRedLinkColorHovered() : Ui::CommonStyle::getRedLinkColor());
        painter.setPen(pen);
        const auto font_size = Utils::scale_value(16);
        QFont f = Fonts::appFont(font_size);
        painter.setFont(f);
        QFontMetrics metrics(f);
        auto titleRect = metrics.boundingRect(title);
        auto recentParams = GetRecentsParams(_viewParams.regim_);

        recentParams.deleteAllFrame().setX(CorrectItemWidth(ItemWidth(false, false, false).px(), _viewParams.fixedWidth_) - recentParams.itemHorPadding().px() - recentParams.interPadding().px() - titleRect.width());
        recentParams.deleteAllFrame().setY(recentParams.unknownsItemHeight().px() / 2 - font_size / 2);
        recentParams.deleteAllFrame().setWidth(ItemWidth(false, false, false).px() - recentParams.deleteAllFrame().x());
        recentParams.deleteAllFrame().setHeight(recentParams.unknownsItemHeight().px());
        Utils::drawText(painter, QPointF(recentParams.deleteAllFrame().x(), recentParams.deleteAllFrame().y()), Qt::AlignVCenter, title);

        recentParams.deleteAllFrame().setY(0);
        recentParams.deleteAllFrame().setHeight(recentParams.unknownsItemHeight().px());

        painter.restore();
    }

    QRect AddContactFrame()
    {
        auto recentParams = GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
        return recentParams.addContactFrame();
    }

    QRect RemoveContactFrame()
    {
        auto recentParams = GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
        return recentParams.removeContactFrame();
    }

    QRect DeleteAllFrame()
    {
        auto recentParams = GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
        return recentParams.deleteAllFrame();
    }
}
