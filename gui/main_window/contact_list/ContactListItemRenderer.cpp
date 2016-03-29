#include "stdafx.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/Text2DocConverter.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "Common.h"
#include "ContactListItemRenderer.h"
#include "ContactList.h"

namespace
{
    using namespace ContactList;

    namespace L
    {
        const auto itemH = dip(44);
        const auto itemPadding = dip(16);
        const auto checkboxWidth = dip(20);
        const auto remove_size = dip(20);

        const auto itemLeftBorder = itemPadding;

        DipPixels GetAvatarX(bool _isWithCheckBox)
        {
            return _isWithCheckBox ? checkboxWidth + dip(24) + dip(10) : itemLeftBorder;
        }

        const auto avatarY = dip(6);
        const auto avatarW = dip(32);
        const auto avatarH = dip(32);

        const auto timeFontSize = dip(12);
        const auto timeY = dip(27);
        const auto timeFont = dif(Utils::FontsFamily::SEGOE_UI, 12);
        const QColor timeFontColor(0x69, 0x69, 0x69);

        const auto onlineSignLeftPadding = dip(12);
        const auto onlineSignSize = dip(8);
        const auto onlineSignY = dip(18);
        const QColor onlineSignColor("#579e1c");

        DipPixels GetContactNameX(bool _isWithCheckBox)
        {
            return (GetAvatarX(_isWithCheckBox) + avatarW + dip(12));
        }

        const auto contactNameRightPadding = dip(12);
        const auto contactNameFontSize = dip(16);
        const auto contactNameHeight = dip(22);
        const auto contactNameCenterY = dip(10);
        const auto contactNameTopY = dip(2);

        QString getContactNameStylesheet(const QString& color)
        { 
            return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
                .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
                .arg(contactNameFontSize.px())
                .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI))
                .arg(color);
        };

        DipPixels GetStatusX(bool _isWithCheckBox)
        {
            return GetContactNameX(_isWithCheckBox);
        }

        const auto statusY = dip(21);
        const auto statusFontSize = dip(14);
        const auto statusHeight = dip(20);
        const auto getStatusStylesheet = []{
            return QString("font: %1 %2px \"%3\"; color: #696969; background-color: transparent")
                .arg(Utils::appFontWeightQss(Utils::FontsFamily::SEGOE_UI))
                .arg(statusFontSize.px())
                .arg(Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI));
        };

        DipPixels GetAddContactX(bool _isWithCheckBox)
        {
            return GetContactNameX(_isWithCheckBox);
        }

        const auto addContactY = dip(30);
        const auto addContactFont = dif(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 16);
        const auto emptyIgnoreListFont = dif(Utils::FontsFamily::SEGOE_UI, 16);
        const QColor addContactColor("#579e1c");
        const QColor emptyIgnoreListColor("#000000");
        const QColor addContactColorHovered("#60aa23");

        DipPixels GetGroupX(bool _isWithCheckBox)
        {
            return GetAddContactX(_isWithCheckBox);
        }
        const auto groupY = dip(17);
        const auto groupFont = dif(Utils::FontsFamily::SEGOE_UI, 12);
        const QColor groupColor("#579e1c");

        const auto dragOverlayPadding = dip(8);
        const auto dragOverlayBorderWidth = dip(2);
        const auto dragOverlayBorderRadius = dip(8);
        const auto dragOverlayVerPadding = dip(1);
    }

    void RenderAvatar(QPainter &painter, const ContactListVisualData &visData, bool _isWithCheckBox);

    void RenderCheckbox(QPainter &painter, const ContactListVisualData &visData);

    void RenderContactName(QPainter &painter, const ContactListVisualData &visData, const int y, const int rightBorderPx, bool _isWithCheckBox);

    int RenderDate(QPainter &painter, const QDateTime &date, int _regim, const ContactListVisualData &item, bool _shortView);

    void RenderMouseState(QPainter &painter, const bool isHovered, const bool isSelected, bool _isWithCheckBox, bool _shortView);

    int RenderOnline(QPainter &painter, int _regim, const ContactListVisualData &item, bool _shortView);

    int RenderRemove(QPainter &painter, int _regim, bool _shortView);

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, bool _isWithCheckBox);
}

namespace ContactList
{

    ContactListVisualData::ContactListVisualData(
        const QString &aimId,
        const QPixmap &avatar,
        const QString &state,
        const QString &status,
        const bool isHovered,
        const bool isSelected,
        const QString &contactName,
        const bool haveLastSeen,
        const QDateTime &lastSeen,
        bool isWithCheckBox,
        bool isChatMember)
        : VisualDataBase(aimId, avatar, state, status, isHovered, isSelected, contactName, haveLastSeen, lastSeen, isWithCheckBox, isChatMember)
    {
    }

    void RenderServiceContact(QPainter &painter, const bool _isHovered, const bool _isActive, int _regim, QString _name, Data::ContactType _type)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);

        RenderMouseState(painter, _isHovered, _isActive, _isWithCheckBox, false);
        if (_type == Data::VIEW_ALL_MEMBERS)
        {
            auto x = DipPixels(ItemWidth(false, _isWithCheckBox, false) - L::itemPadding);
            QRect rect(L::GetAvatarX(_isWithCheckBox).px(), L::avatarY.px(), x.px(), Utils::scale_value(1));
            painter.fillRect(rect, QBrush(QColor("#dadada")));
        }

        static const auto hoveredIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContactHovered);
        static const auto plainIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContact);

        const auto &icon = (_isHovered ? hoveredIcon : plainIcon);
        if (_type == Data::ContactType::ADD_CONTACT)
            icon->Draw(painter, L::GetAvatarX(_isWithCheckBox).px(), L::avatarY.px(), L::avatarW.px(), L::avatarH.px());

        if (_type == Data::EMPTY_IGNORE_LIST)
        {
            painter.setPen(L::emptyIgnoreListColor);
            painter.setFont(L::emptyIgnoreListFont.font());
        }
        else
        {
            painter.setPen(_isHovered ? L::addContactColorHovered : L::addContactColor);
            painter.setFont(L::addContactFont.font());
        }

        painter.translate(L::GetAddContactX(_isWithCheckBox).px(), L::addContactY.px());

        painter.drawText(0, 0, _name);

        painter.restore();
    }

    void RenderContactItem(QPainter &painter, const ContactListVisualData &item, int _regim, bool _shortView)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        painter.save();

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(painter, item.IsHovered_, item.IsSelected_, _isWithCheckBox, _shortView);

        if (_isWithCheckBox)
        {
            RenderCheckbox(painter, item);
        }

        RenderAvatar(painter, item, _isWithCheckBox);

        auto rightBorderPx = 0;

        if (Logic::is_delete_members(_regim) && item.IsHovered_)
        {
            rightBorderPx = RenderRemove(painter, _regim, _shortView);
        }

        if (item.IsOnline())
        {
            rightBorderPx = RenderOnline(painter, _regim, item, _shortView);
        }
        else
        {
            rightBorderPx = RenderDate(painter, item.LastSeen_, _regim, item, _shortView);
        }

        if (item.Status_.isEmpty())
        {
            RenderContactName(painter, item, L::contactNameCenterY.px(), rightBorderPx, _isWithCheckBox);
        }
        else
        {
            RenderContactName(painter, item, L::contactNameTopY.px(), rightBorderPx, _isWithCheckBox);
            RenderStatus(painter, item.Status_, rightBorderPx, _isWithCheckBox);
        }

        painter.restore();
    }

    void RenderGroupItem(QPainter &painter, const QString &groupName)
    {
        painter.save();

        painter.setPen(L::groupColor);
        painter.setBrush(Qt::NoBrush);
        painter.setFont(L::groupFont.font());

        painter.translate(L::GetGroupX(false).px(), L::groupY.px());
        painter.drawText(0, 0, groupName);

        painter.restore();
    }

    void RenderContactsDragOverlay(QPainter &painter)
    {
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(0, 0, ItemWidth(false, false, false).px(), L::itemH.px(), QBrush(QColor(255, 255, 255, 255 * 0.9)));
        painter.setBrush(QBrush(QColor(255, 255, 255, 0)));
        QPen pen;
        pen.setColor(QColor(0x57,0x9e,0x1c));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(L::dragOverlayBorderWidth.px());
        painter.setPen(pen);
        painter.drawRoundedRect(L::dragOverlayPadding.px(), L::dragOverlayVerPadding.px(), ItemWidth(false, false, false).px() - L::itemPadding.px(), L::itemH.px() - L::dragOverlayVerPadding.px(), L::dragOverlayBorderRadius.px(), L::dragOverlayBorderRadius.px());

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (ItemWidth(false, false, false).px() / 2) - (p.width() / 2. / ratio);
        int y = (L::itemH.px() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);

        painter.restore();
    }
    
    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView)
    {
        return DipPixels(ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding).px() - DipPixels(L::onlineSignSize + L::remove_size).px() / 2;
    }
}

namespace
{
    void RenderAvatar(QPainter &painter, const ContactListVisualData &visData, bool _isWithCheckBox)
    {
        if (visData.Avatar_.isNull())
        {
            return;
        }

        painter.drawPixmap(L::GetAvatarX(_isWithCheckBox).px(), L::avatarY.px(), L::avatarW.px(), L::avatarH.px(), visData.Avatar_);
    }

    void RenderCheckbox(QPainter &painter, const ContactListVisualData &visData)
    {		
        QString img = visData.isChatMember_ ?
            ":/resources/contr_clear_125.png"
            : (visData.isCheckedBox_ 
            ? ":/resources/widgets/content_check_100.png"
            : ":/resources/widgets/content_uncheck_100.png");
        QPixmap checkbox = QPixmap(img).scaled(L::checkboxWidth.px(),L::checkboxWidth.px(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

        
        painter.drawPixmap((L::GetAvatarX(true) - L::checkboxWidth - dip(10)).px(), L::avatarH.px() / 2.0 + L::avatarY.px() - L::checkboxWidth.px() / 2, L::checkboxWidth.px(), L::checkboxWidth.px(), checkbox);
    }

    void RenderContactName(QPainter &painter, const ContactListVisualData &visData, const int y, const int rightBorderPx, bool _isWithCheckBox)
    {
        assert(y > 0);
        assert(rightBorderPx > 0);
        assert(!visData.ContactName_.isEmpty());

        QString color = _isWithCheckBox && (visData.isChatMember_ || visData.isCheckedBox_) ? "#579e1c": "#282828";

        static auto textControl = CreateTextBrowser("name", L::getContactNameStylesheet(color), L::contactNameHeight.px());
        textControl.get()->setStyleSheet(L::getContactNameStylesheet(color));

        const auto maxWidth = (rightBorderPx - L::GetContactNameX(_isWithCheckBox).px() - L::contactNameRightPadding.px());
        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());

        const auto elidedString = m.elidedText(visData.ContactName_, Qt::ElideRight, maxWidth);

        auto &doc = *textControl->document();
        doc.clear();

        QTextCursor cursor = textControl->textCursor();
        Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, L::contactNameHeight.px());

        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();
            qreal correction = (realHeight > 20)?0:2;
            
//            qDebug() << "text " << elidedString << " height " << realHeight << " d " << correction;
            
            textControl->render(&painter, QPoint(L::GetContactNameX(_isWithCheckBox).px(), y + correction));
        }
        else
        {
            textControl->render(&painter, QPoint(L::GetContactNameX(_isWithCheckBox).px(), y));
        }
    }

    int RenderDate(QPainter &painter, const QDateTime &ts, int _regim, const ContactListVisualData &item, bool _shortView)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS; 
        const auto timeXRight = ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding;

        if (!ts.isValid())
        {
            return timeXRight.px();
        }

        const auto timeStr = FormatTime(ts);
        if (timeStr.isEmpty())
        {
            return timeXRight.px();
        }

        static QFontMetrics m(L::timeFont.font());
        const auto leftBearing = m.leftBearing(timeStr[0]);
        const auto rightBearing = m.rightBearing(timeStr[timeStr.length() - 1]);
        const auto timeWidth = (m.tightBoundingRect(timeStr).width() + leftBearing + rightBearing);
        const auto timeX = (timeXRight.px() - timeWidth);

        if ((!_isWithCheckBox && !Logic::is_delete_members(_regim) )
            || (Logic::is_delete_members(_regim) && !item.IsHovered_))
        {
            painter.save();
            painter.setFont(L::timeFont.font());
            painter.setPen(L::timeFontColor);
            painter.drawText(timeX, L::timeY.px(), timeStr);
            painter.restore();
        }

        return timeX;
    }

    void RenderMouseState(QPainter &painter, const bool isHovered, const bool isSelected, bool _isWithCheckBox, bool _shortView)
    {
        if (!isHovered && !isSelected)
        {
            return;
        }

        painter.save();

        if (isHovered)
        {
            static QBrush hoverBrush(QColor(220, 220, 220, 0.4 * 255));
            painter.setBrush(hoverBrush);
        }
        if (isSelected)
        {
            static QBrush selectedBrush(QColor(202, 230, 179, 0.7 * 255));
            painter.setBrush(selectedBrush);
        }

        painter.drawRect(0, 0, ItemWidth(false, _isWithCheckBox, _shortView).px(), L::itemH.px());

        painter.restore();
    }

    int RenderOnline(QPainter &painter, int _regim, const ContactListVisualData &item, bool _shortView)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS; 
        if ((!_isWithCheckBox && !Logic::is_delete_members(_regim) )
            || (Logic::is_delete_members(_regim) && !item.IsHovered_))
        {
            painter.save();

            painter.setBrush(L::onlineSignColor);
            painter.drawEllipse(DipPixels(ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding - L::onlineSignSize).px(), L::onlineSignY.px(), L::onlineSignSize.px(), L::onlineSignSize.px());

            painter.restore();
        }        

        const auto xPos = (DipPixels(ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding - L::onlineSignSize).px() - L::onlineSignLeftPadding.px());
        assert(xPos > L::itemLeftBorder.px());
        return xPos;
    }

    int RenderRemove(QPainter &painter, int, bool _shortView)
    {
        QPixmap remove_img;
        if (platform::is_apple())
            remove_img = QPixmap(Utils::ScaleStyle(":/resources/contr_clear_100.png", Utils::get_scale_coefficient()));
        else
            remove_img = QPixmap(":/resources/contr_clear_100.png").scaled(L::remove_size.px(), L::remove_size.px(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        bool _isWithCheckBox = false;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(GetXOfRemoveImg(_isWithCheckBox, _shortView), L::onlineSignY.px() + L::onlineSignSize.px() / 2 - L::remove_size.px() / 2,
            L::remove_size.px(), L::remove_size.px(), remove_img);
        
        painter.restore();

        const auto xPos = (DipPixels(ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding - L::remove_size).px() - L::onlineSignLeftPadding.px());
        assert(xPos > L::itemLeftBorder.px());
        return xPos;
    }

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, bool _isWithCheckBox)
    {
        assert(!status.isEmpty());

        static auto textControl = CreateTextBrowser("status", L::getStatusStylesheet(), L::statusHeight.px());

        const auto maxWidth = (rightBorderPx - L::GetStatusX(_isWithCheckBox).px());
        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());
        const auto elidedString = m.elidedText(status, Qt::ElideRight, maxWidth);

        auto &doc = *textControl->document();
        doc.clear();
        QTextCursor cursor = textControl->textCursor();
        Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, L::statusHeight.px());

        textControl->render(&painter, QPoint(L::GetStatusX(_isWithCheckBox).px(), L::statusY.px()));
    }
}