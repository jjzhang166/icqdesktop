#include "stdafx.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/Text2DocConverter.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "Common.h"
#include "RecentsItemRenderer.h"
#include "RecentsModel.h"

#include "../../gui_settings.h"

namespace
{

	using namespace ContactList;

	namespace L
	{
		const auto itemHeight = dip(60);
        const auto serviceItemHeight = dip(25);
		const auto itemHorPadding = dip(16);
        const auto favoritesStatusPadding = dip(4);

		const auto avatarX = dip(16);
		const auto avatarY = dip(6);
		const auto avatarW = dip(48);
		const auto avatarH = dip(48);

		const auto nameFontSize = dip(17);
		const auto nameHeight = dip(22);
		const auto nameX = avatarX + avatarW + dip(13);
		const auto nameY = dip(8);
		const auto nameRightPadding = dip(12);
        const auto getNameStylesheet = []{
            return QString("font: %1 %2px \"%3\"; color: #282828; background-color: transparent")
                .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
                .arg(nameFontSize.px())
                .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI));
        };
		const auto messageFontSize = dip(14);
		const auto messageHeight = dip(24);
		const auto messageX = nameX;
		const auto messageY = dip(30);
		const auto getMessageStylesheet = []{
            return QString("font: %1 %2px \"%3\"; color: #696969; background-color: transparent")
                .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
                .arg(messageFontSize.px())
                .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI));
        };

		const auto timeFontSize = dip(12);
		const auto timeY = avatarY + dip(20);
		const auto timeFont = dif(Utils::FontsFamily::SEGOE_UI, timeFontSize.px());
		const QColor timeFontColor(0x69, 0x69, 0x69);

		const auto deliveryStateY = avatarY + dip(11);
		const auto deliveryStateRightPadding = dip(4);

		const auto unreadsFont = dif(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 13);
		const auto unreadsPadding = dip(6);
		const auto unreadsMinimumExtent = dip(20);
		const auto unreadsY = dip(34);
		const auto unreadsLeftPadding = dip(12);
        const auto dragOverlayPadding = dip(8);
        const auto dragOverlayBorderWidth = dip(2);
        const auto dragOverlayBorderRadius = dip(8);
        const auto dragOverlayVerPadding = dip(1);
	}

	void RenderAvatar(QPainter &painter, const RecentItemVisualData &visData);

	void RenderContactMessage(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx);

	void RenderContactName(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx);

	int RenderDeliveryState(QPainter &painter, const DeliveryState state, const int rightBorderPx);

	void RenderMouseState(QPainter &painter, const RecentItemVisualData &visData, bool fromAlert, bool _shortView);

	int RenderTime(QPainter &painter, const QDateTime &time, const DeliveryState state, bool fromAlert, bool _shortView);

	int RenderUnread(QPainter &painter, const int unreads, bool muted);

}

namespace ContactList
{
	RecentItemVisualData::RecentItemVisualData(
		const QString &aimId,
		const QPixmap &avatar,
		const QString &state,
		const QString &status,
		const bool isHovered,
		const bool isSelected,
        const bool isTyping,
		const QString &contactName,
		const DeliveryState deliveryState,
		const bool haveLastSeen,
		const QDateTime &lastSeen,
		const int unreadsCounter,
		const bool muted,
        const QString &senderNick)
		: VisualDataBase(aimId, avatar, state, status, isHovered, isSelected, contactName, haveLastSeen, lastSeen, false, false)
		, DeliveryState_(deliveryState)
		, UnreadsCounter_(unreadsCounter)
		, Muted_(muted)
        , senderNick_(senderNick)
        , IsTyping_(isTyping)
	{
		assert(deliveryState >= DeliveryState::Min);
		assert(deliveryState <= DeliveryState::Max);
		assert(unreadsCounter >= 0);
	}

	bool RecentItemVisualData::HasAvatar() const
	{
		return !Avatar_.isNull();
	}

	void RenderRecentsItem(QPainter &painter, const RecentItemVisualData &item, bool fromAlert)
	{
		painter.save();

		painter.setBrush(Qt::NoBrush);
		painter.setPen(Qt::NoPen);
		painter.setRenderHint(QPainter::Antialiasing);

        bool short_view = false;

		RenderMouseState(painter, item, fromAlert, short_view);

		RenderAvatar(painter, item);

		//auto rightBorderPx = RenderTime(painter, item.LastSeen_, item.DeliveryState_, fromAlert, short_view);

		//if (!fromAlert)
			//rightBorderPx = RenderDeliveryState(painter, item.DeliveryState_, rightBorderPx);


        int rightBorderPx = (ItemWidth(fromAlert, false, short_view) - L::itemHorPadding).px();
		RenderContactName(painter, item, rightBorderPx);

		if (!fromAlert)
			rightBorderPx = RenderUnread(painter, item.UnreadsCounter_, item.Muted_);

		RenderContactMessage(painter, item, rightBorderPx);

		painter.restore();
	}

    void RenderRecentsDragOverlay(QPainter &painter)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(0, 0, ItemWidth(false, false, false).px(), L::itemHeight.px(), QBrush(QColor(255, 255, 255, 255 * 0.9)));
        painter.setBrush(QBrush(QColor(255, 255, 255, 0)));;
        QPen pen;
        pen.setColor(QColor(0x57,0x9e,0x1c));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(L::dragOverlayBorderWidth.px());
        painter.setPen(pen);
        painter.drawRoundedRect(L::dragOverlayPadding.px(), L::dragOverlayVerPadding.px(), ItemWidth(false, false, false).px() - L::itemHorPadding.px(), L::itemHeight.px() - L::dragOverlayVerPadding.px(), L::dragOverlayBorderRadius.px(), L::dragOverlayBorderRadius.px());

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (ItemWidth(false, false, false).px() / 2) - (p.width() / 2. / ratio);
        int y = (L::itemHeight.px() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);

        painter.restore();
    }

    void RenderServiceItem(QPainter &painter, const QString& text, bool renderState)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        QPen pen;
        pen.setColor(QColor(0x57,0x9e,0x1c));
        painter.setPen(pen);
        QFont f = Utils::appFont(Utils::FontsFamily::SEGOE_UI_BOLD, Utils::scale_value(12));
        painter.setFont(f);
        Utils::drawText(painter, QPointF(L::itemHorPadding.px(), L::serviceItemHeight.px() / 2), Qt::AlignVCenter, text);

        if (renderState)
        {
            QPixmap p(Utils::parse_image_name(Logic::GetRecentsModel()->isFavoritesVisible() ? ":/resources/cl_group_close_100.png" : ":/resources/cl_group_open_100.png"));
            Utils::check_pixel_ratio(p);
            double ratio = Utils::scale_bitmap(1);
            QFontMetrics m(f);
            int x = L::itemHorPadding.px() + m.width(text) + L::favoritesStatusPadding.px();
            int y = L::serviceItemHeight.px() / 2 - (p.height() / 2. / ratio) + Utils::scale_value(1);
            painter.drawPixmap(x, y, p);
        }

        painter.restore();
    }
}

namespace
{

	void RenderAvatar(QPainter &painter, const RecentItemVisualData &visData)
	{
		if (!visData.HasAvatar())
		{
			return;
		}

		painter.drawPixmap(L::avatarX.px(), L::avatarY.px(), L::avatarW.px(), L::avatarH.px(), visData.Avatar_);
	}

	void RenderContactMessage(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx)
	{
		if (visData.Status_.isEmpty())
		{
			return;
		}

		static auto textControl = CreateTextBrowser("message", L::getMessageStylesheet(), L::messageHeight.px());

		const auto maxWidth = (rightBorderPx - L::messageX.px());
		textControl->setFixedWidth(maxWidth);
        textControl->setWordWrapMode(QTextOption::WrapMode::NoWrap);

        auto messageTextMaxWidth = maxWidth;

        const auto font = textControl->font();

        auto &doc = *textControl->document();
        doc.clear();

        auto cursor = textControl->textCursor();

        const auto &senderNick = visData.senderNick_;
        const auto showLastMessage = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
        if (!senderNick.isEmpty() && !visData.IsTyping_ && showLastMessage)
        {
            static const QString fix(": ");

            QString fixedNick;
            fixedNick.reserve(senderNick.size() + fix.length());
            fixedNick.append(senderNick);
            fixedNick.append(fix);

            const auto charFormat = cursor.charFormat();

            auto boldCharFormat = charFormat;
            boldCharFormat.setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, Utils::scale_value(14)));

            cursor.setCharFormat(boldCharFormat);

            Logic::Text2Doc(
                fixedNick,
                cursor,
                Logic::Text2DocHtmlMode::Pass,
                false
            );

            cursor.setCharFormat(charFormat);

            messageTextMaxWidth -= doc.idealWidth();
        }

        const auto text = visData.Status_.trimmed().replace("\n", " ").remove("\r");

        QFontMetrics m(font);
		const auto elidedText = m.elidedText(text, Qt::ElideRight, messageTextMaxWidth);

		Logic::Text2Doc(elidedText, cursor, Logic::Text2DocHtmlMode::Pass, false);

		Logic::FormatDocument(doc, L::messageHeight.px());

        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();
            qreal correction = ((realHeight > 17) ? 0 : 4);

            textControl->render(&painter, QPoint(L::messageX.px(), L::messageY.px() + correction));
        }
        else
        {
            textControl->render(&painter, QPoint(L::messageX.px(), L::messageY.px()));
        }
	}

	void RenderContactName(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx)
	{
		assert(!visData.ContactName_.isEmpty());

        static auto textControl = CreateTextBrowser("name", L::getNameStylesheet(), L::nameHeight.px() + (platform::is_apple()?1:0) );

		const auto maxWidth = (rightBorderPx - L::nameX.px() - L::nameRightPadding.px());
		textControl->setFixedWidth(maxWidth);

		QFontMetrics m(textControl->font());
		const auto elidedString = m.elidedText(visData.ContactName_, Qt::ElideRight, maxWidth);

		auto &doc = *textControl->document();
		doc.clear();
		QTextCursor cursor = textControl->textCursor();
		Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
		Logic::FormatDocument(doc, L::nameHeight.px());

        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();
            qreal correction = (realHeight > 21)?-2:2;

//                                    qDebug() << "text " << elidedString << " height " << realHeight << " d " << correction;


            textControl->render(&painter, QPoint(L::nameX.px(), L::nameY.px() + correction));
        }
        else
        {
            textControl->render(&painter, QPoint(L::nameX.px(), L::nameY.px()));
        }
	}

	int RenderDeliveryState(QPainter &painter, const DeliveryState state, const int rightBorderPx)
	{
		assert(state >= DeliveryState::Min);
		assert(state <= DeliveryState::Max);

		if (state == DeliveryState::NotDelivered)
		{
			return rightBorderPx;
		}

		Themes::IThemePixmapSptr pixmap;

		if (state == DeliveryState::Sending)
		{
			static const auto sendingMark = Themes::GetPixmap(Themes::PixmapResourceId::ContactListSendingMark);
			pixmap = sendingMark;
		}

		if (state == DeliveryState::DeliveredToClient)
		{
			static const auto readMark = Themes::GetPixmap(Themes::PixmapResourceId::ContactListReadMark);
			pixmap = readMark;
		}

		if (state == DeliveryState::DeliveredToServer)
		{
			static const auto deliveredMark = Themes::GetPixmap(Themes::PixmapResourceId::ContactListDeliveredMark);
			pixmap = deliveredMark;
		}

		if (!pixmap)
		{
			assert(!"unexpected delivery state");

			return rightBorderPx;
		}

		auto macOffset = Utils::scale_bitmap(1) == 2 ? 7 : 0;
		const auto deliveryStateX = (rightBorderPx - L::deliveryStateRightPadding.px() - pixmap->GetWidth()/Utils::scale_bitmap(1) - macOffset);
		pixmap->Draw(painter, deliveryStateX, L::deliveryStateY.px());

		return deliveryStateX;
	}

	void RenderMouseState(QPainter &painter, const RecentItemVisualData &visData, bool fromAlert, bool _shortView)
	{
		if (!visData.IsHovered_ && !visData.IsSelected_)
		{
			return;
		}

		painter.save();

		if (visData.IsHovered_)
		{
			static QBrush hoverBrush(fromAlert ? QColor(220, 220, 220, 0.7 * 255) : QColor(220, 220, 220, 0.4 * 255));
			painter.setBrush(hoverBrush);
		}
		else
		{
			static QBrush selectedBrush(QColor(202, 230, 179, 0.7 * 255));
			painter.setBrush(selectedBrush);
		}

		painter.drawRect(0, 0, ItemWidth(fromAlert, false, _shortView).px(), L::itemHeight.px());

		painter.restore();
	}

	int RenderTime(QPainter &painter, const QDateTime &time, const DeliveryState state, bool fromAlert, bool _shortView)
	{
		const auto timeXRight = (ItemWidth(fromAlert, false, _shortView) - L::itemHorPadding);
		assert(timeXRight.px() > 0);

		const auto hasLastSeen = (state != DeliveryState::Sending && time.isValid() && (time != QDateTime()));
		if (!hasLastSeen)
		{
			return timeXRight.px();
		}

		const auto timeStr = ContactList::FormatTime(time);
		if (timeStr.isEmpty())
		{
			return timeXRight.px();
		}

		static QFontMetrics m(L::timeFont.font());
		const auto leftBearing = m.leftBearing(timeStr[0]);
		const auto rightBearing = m.rightBearing(timeStr[timeStr.length() - 1]);
		const auto timeWidth = (m.tightBoundingRect(timeStr).width() + leftBearing + rightBearing);
		const auto timeX = (timeXRight.px() - timeWidth);

		painter.save();
		painter.setFont(L::timeFont.font());
		painter.setPen(L::timeFontColor);
		painter.drawText(timeX, L::timeY.px(), timeStr);
		painter.restore();

		return timeX;
	}

	int RenderUnread(QPainter &painter, const int unreads, bool muted)
	{
		assert(unreads >= 0);

		if (muted)
		{
			Themes::IThemePixmapSptr pixmap = Themes::GetPixmap(unreads == 0 ? Themes::PixmapResourceId::ContentMuteNotify : Themes::PixmapResourceId::ContentMuteNotifyNew);
			auto mutedX = (ItemWidth(false, false, false).px() - L::itemHorPadding.px()) - pixmap->GetWidth();
			pixmap->Draw(painter, mutedX, L::unreadsY.px());
			return ((ItemWidth(false, false, false).px() - L::itemHorPadding.px()) - pixmap->GetWidth());
		}

		if (unreads <= 0)
		{
			return ItemWidth(false, false, false).px() - L::itemHorPadding.px();
		}

		const auto text = (unreads > 99) ? QString("99+") : QVariant(unreads).toString();

		static QFontMetrics m(L::unreadsFont.font());

		const auto unreadsRect = m.tightBoundingRect(text);
		const auto firstChar = text[0];
		const auto lastChar = text[text.size() - 1];
		const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
		const auto unreadsHeight = unreadsRect.height();

		auto balloonWidth = unreadsWidth;
		const auto isLongText = (text.length() > 1);
		if (isLongText)
		{
			balloonWidth += (L::unreadsPadding.px() * 2);
		}
		else
		{
			balloonWidth = L::unreadsMinimumExtent.px();
		}

		const auto ballonHeight = L::unreadsMinimumExtent.px();

		const auto unreadsX = ((ItemWidth(false, false, false).px() - L::itemHorPadding.px()) - balloonWidth);
		const auto balloonRadius = (ballonHeight / 2);

		painter.setBrush(QColor("#579e1c"));
		painter.drawRoundedRect(unreadsX, L::unreadsY.px(), balloonWidth, ballonHeight, balloonRadius, balloonRadius);

		painter.setFont(L::unreadsFont.font());
		painter.setPen(Qt::white);
		const auto textX = (unreadsX + (balloonWidth - unreadsWidth) / 2);
        const auto textY = (L::unreadsY.px() + (ballonHeight + unreadsHeight) / 2) + (platform::is_apple()?1:0);
		painter.drawText(textX, textY, text);

		return (unreadsX - L::unreadsLeftPadding.px());
	}
}