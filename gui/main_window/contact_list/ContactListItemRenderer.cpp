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
        const auto role_offset = dip(24);
        const auto role_ver_offset = dip(4);

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
        const auto onlineSignSize = dip(4);
        const auto onlineSignY = dip(18);
        const QColor onlineSignColor("#579e1c");

        DipPixels GetContactNameX(bool _isWithCheckBox, int margin)
        {
            return (GetAvatarX(_isWithCheckBox) + avatarW + dip(12) + DipPixels(Utils::unscale_value(margin)));
        }

        const auto contactNameRightPadding = dip(12);
        const auto contactNameFontSize = dip(16);
        const auto contactNameHeight = dip(24);
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

        DipPixels GetStatusX(bool _isWithCheckBox, int margin)
        {
            return GetContactNameX(_isWithCheckBox, margin);
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

        DipPixels GetAddContactX(bool _isWithCheckBox, int margin)
        {
            return GetContactNameX(_isWithCheckBox, margin);
        }

        const auto addContactY = dip(30);
        const auto addContactFont = dif(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 16);
        const auto emptyIgnoreListFont = dif(Utils::FontsFamily::SEGOE_UI, 16);
        const QColor addContactColor("#579e1c");
        const QColor emptyIgnoreListColor("#000000");
        const QColor addContactColorHovered("#60aa23");

        DipPixels GetGroupX(bool _isWithCheckBox, int margin)
        {
            return GetAddContactX(_isWithCheckBox, margin);
        }
        const auto groupY = dip(17);
        const auto groupFont = dif(Utils::FontsFamily::SEGOE_UI, 12);
        const QColor groupColor("#579e1c");

        const auto dragOverlayPadding = dip(8);
        const auto dragOverlayBorderWidth = dip(2);
        const auto dragOverlayBorderRadius = dip(8);
        const auto dragOverlayVerPadding = dip(1);

        const auto official_hor_padding = dip(6);
        const auto official_ver_padding = dip(4);
    }

    void RenderAvatar(QPainter &painter, const ContactListVisualData &visData, bool _isWithCheckBox);

    void RenderCheckbox(QPainter &painter, const ContactListVisualData &visData);

    void RenderRole(QPainter &painter, const int y, const int leftMargin, const bool withCheckbox, const QString& role);

    void RenderContactName(QPainter &painter, const ContactListVisualData &visData, const int y, const int rightBorderPx, bool _isWithCheckBox);

    int RenderDate(QPainter &painter, const QDateTime &date, int _regim, const ContactListVisualData &item, bool _shortView, int width, int rightMargin);

    void RenderMouseState(QPainter &painter, const bool isHovered, const bool isSelected, bool _isWithCheckBox, bool _shortView, int width);

    int RenderRemove(QPainter &painter, int _regim, bool _shortView, int width);

    int RenderMore(QPainter &painter, int _regim, bool _shortView, int width);

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, bool _isWithCheckBox, const int leftMargin);

    struct StringInfo
    {

    };

    //std::unordered_map<QString, 
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
        bool isChatMember,
        bool isOfficial,
        int width,
        int leftMargin,
        int rightMargin,
        const QString& role)
        : VisualDataBase(aimId, avatar, state, status, isHovered, isSelected, contactName, haveLastSeen, lastSeen, isWithCheckBox, isChatMember, isOfficial, false, QPixmap())
        , width_(width)
        , leftMargin_(leftMargin)
        , rightMargin_(rightMargin)
        , role_(role)
    {
    }

    void RenderServiceContact(QPainter &painter, const bool _isHovered, const bool _isActive, int _regim, QString _name, Data::ContactType _type, int leftMargin)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);

        RenderMouseState(painter, _isHovered, _isActive, _isWithCheckBox, false, -1);
        if (_type == Data::VIEW_ALL_MEMBERS)
        {
            auto x = DipPixels(ItemWidth(false, _isWithCheckBox, false) - L::itemPadding);
            QRect rect(L::GetAvatarX(_isWithCheckBox).px() + leftMargin, L::avatarY.px(), x.px(), Utils::scale_value(1));
            painter.fillRect(rect, QBrush(QColor("#dadada")));
        }

        static const auto hoveredIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContactHovered);
        static const auto plainIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContact);

        const auto &icon = (_isHovered ? hoveredIcon : plainIcon);
        if (_type == Data::ContactType::ADD_CONTACT)
            icon->Draw(painter, L::GetAvatarX(_isWithCheckBox).px() + leftMargin, L::avatarY.px(), L::avatarW.px(), L::avatarH.px());

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

        painter.translate(L::GetAddContactX(_isWithCheckBox, leftMargin).px(), L::addContactY.px());

        painter.drawText(0, 0, _name);

        painter.restore();
    }

    void RenderContactItem(QPainter &painter, ContactListVisualData item, int _regim, bool _shortView)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        painter.save();

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(painter, item.IsHovered_, item.IsSelected_, _isWithCheckBox, _shortView, item.width_);

        if (_isWithCheckBox)
        {
            RenderCheckbox(painter, item);
        }

        RenderAvatar(painter, item, _isWithCheckBox);

        auto rightBorderPx = 0;

        if (item.IsHovered_)
        {
            if (Logic::is_delete_members_regim(_regim))
            {
                rightBorderPx = RenderRemove(painter, _regim, _shortView, item.width_);
            }
            else if (Logic::is_admin_members_regim(_regim))
            {
                rightBorderPx = RenderMore(painter, _regim, _shortView, item.width_);
            }
        }


        if (!item.IsOnline())
        {
            rightBorderPx = RenderDate(painter, item.LastSeen_, _regim, item, _shortView, item.width_, item.rightMargin_);
        }
        else
        {
            rightBorderPx = item.width_ == -1 ? ItemWidth(false, _isWithCheckBox, _shortView).px() : item.width_;
            rightBorderPx -= item.rightMargin_;
        }

        if (item.HasStatus())
        {
            if (item.role_ == "admin" || item.role_ == "moder")
            {
                RenderRole(painter, L::contactNameTopY.px(), item.leftMargin_, _isWithCheckBox, item.role_);
                item.leftMargin_ += L::role_offset.px();
            }

            RenderContactName(painter, item, L::contactNameTopY.px(), rightBorderPx, _isWithCheckBox);
            RenderStatus(painter, item.GetStatus(), rightBorderPx, _isWithCheckBox, item.leftMargin_);
        }
        else
        {
            if (item.role_ == "admin" || item.role_ == "moder")
            {
                RenderRole(painter, L::contactNameCenterY.px(), item.leftMargin_, _isWithCheckBox, item.role_);
                item.leftMargin_ += L::role_offset.px();
            }

            RenderContactName(painter, item, L::contactNameCenterY.px(), rightBorderPx, _isWithCheckBox);
        }

        painter.restore();
    }

    void RenderGroupItem(QPainter &painter, const QString &groupName, int leftMargin)
    {
        painter.save();

        painter.setPen(L::groupColor);
        painter.setBrush(Qt::NoBrush);
        painter.setFont(L::groupFont.font());

        painter.translate(L::GetGroupX(false, leftMargin).px(), L::groupY.px());
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

    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView, int width)
    {
        if (width != -1)
            return DipPixels(DipPixels(Utils::unscale_value(width)) - L::itemPadding).px() - DipPixels(L::onlineSignSize + L::remove_size).px();

        return DipPixels(ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding).px() - DipPixels(L::onlineSignSize + L::remove_size).px();
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

        painter.drawPixmap(L::GetAvatarX(_isWithCheckBox).px() + visData.leftMargin_, L::avatarY.px(), L::avatarW.px(), L::avatarH.px(), visData.Avatar_);
    }

    void RenderCheckbox(QPainter &painter, const ContactListVisualData &visData)
    {
        QString img = visData.isChatMember_ ?
            ":/resources/dialog_closechat_100.png"
            : (visData.isCheckedBox_
            ? ":/resources/widgets/content_check_100.png"
            : ":/resources/widgets/content_uncheck_100.png");
        QPixmap checkbox = Utils::parse_image_name(img);

        painter.drawPixmap((L::GetAvatarX(true) - L::checkboxWidth - dip(10)).px(), L::avatarH.px() / 2.0 + L::avatarY.px() - L::checkboxWidth.px() / 2, L::checkboxWidth.px(), L::checkboxWidth.px(), checkbox);
    }

    void RenderRole(QPainter &painter, const int y, const int leftMargin, const bool withCheckbox, const QString& role)
    {
        QPixmap rolePixmap;
        if (role == "admin")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_ultraadmin_100.png"));
        else if (role == "moder")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_king_100.png"));
        else
            return;
        Utils::check_pixel_ratio(rolePixmap);
        painter.drawPixmap(L::GetContactNameX(withCheckbox, leftMargin).px(), y + L::role_ver_offset.px(), rolePixmap);
    }

    void RenderContactName(QPainter &painter, const ContactListVisualData &visData, const int y, const int rightBorderPx, bool _isWithCheckBox)
    {
        assert(y > 0);
        assert(rightBorderPx > 0);
        assert(!visData.ContactName_.isEmpty());

        QString color = _isWithCheckBox && (visData.isChatMember_ || visData.isCheckedBox_) ? "#579e1c": "#282828";

        static auto textControl = CreateTextBrowser("name", L::getContactNameStylesheet(color), L::contactNameHeight.px());
        textControl.get()->setStyleSheet(L::getContactNameStylesheet(color));

        QPixmap official_mark;
        if (visData.isOfficial_)
        {
            official_mark = QPixmap(Utils::parse_image_name(":/resources/cl_badges_official_100.png"));
            Utils::check_pixel_ratio(official_mark);
        }

        int maxWidth = (rightBorderPx - L::GetContactNameX(_isWithCheckBox, visData.leftMargin_).px() - L::contactNameRightPadding.px());
        if (!official_mark.isNull())
            maxWidth -= official_mark.width();

        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());

        const auto elidedString = m.elidedText(visData.ContactName_, Qt::ElideRight, maxWidth);

        auto &doc = *textControl->document();
        doc.clear();

        QTextCursor cursor = textControl->textCursor();
        Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, L::contactNameHeight.px());

        qreal correction = 0;
        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();
            correction = (realHeight > 20)?0:2;
            textControl->render(&painter, QPoint(L::GetContactNameX(_isWithCheckBox, visData.leftMargin_).px(), y + correction));
        }
        else
        {
            textControl->render(&painter, QPoint(L::GetContactNameX(_isWithCheckBox, visData.leftMargin_).px(), y));
        }

        int pX = L::GetContactNameX(_isWithCheckBox, visData.leftMargin_).px() + m.width(elidedString) + L::official_hor_padding.px();
        int pY = y + L::official_ver_padding.px();
        painter.drawPixmap(pX, pY, official_mark);
    }

    int RenderDate(QPainter &painter, const QDateTime &ts, int _regim, const ContactListVisualData &item, bool _shortView, int width, int rightMargin)
    {
        bool _isWithCheckBox = _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        auto timeXRight = width == -1 ? ItemWidth(false, _isWithCheckBox, _shortView) - L::itemPadding : DipPixels(Utils::unscale_value(width));
        timeXRight = timeXRight - DipPixels(Utils::unscale_value(rightMargin));

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

        if ((!_isWithCheckBox && !Logic::is_delete_members_regim(_regim) && !Logic::is_admin_members_regim(_regim))
            || (Logic::is_delete_members_regim(_regim) && !item.IsHovered_)
            || (!Logic::is_admin_members_regim(_regim) && !item.IsHovered_))
        {
            painter.save();
            painter.setFont(L::timeFont.font());
            painter.setPen(L::timeFontColor);
            painter.drawText(timeX, L::timeY.px(), timeStr);
            painter.restore();
        }

        return timeX;
    }

    void RenderMouseState(QPainter &painter, const bool isHovered, const bool isSelected, bool _isWithCheckBox, bool _shortView, int width)
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

        painter.drawRect(0, 0, width == -1 ? ItemWidth(false, _isWithCheckBox, _shortView).px() : width, L::itemH.px());

        painter.restore();
    }

    int RenderRemove(QPainter &painter, int, bool _shortView, int width)
    {
        QPixmap remove_img;
        remove_img = Utils::parse_image_name(":/resources/contr_clear_100.png");
        bool _isWithCheckBox = false;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(GetXOfRemoveImg(_isWithCheckBox, _shortView, width), L::onlineSignY.px() + L::onlineSignSize.px() / 2 - L::remove_size.px() / 2,
            L::remove_size.px(), L::remove_size.px(), remove_img);

        painter.restore();

        const auto xPos = (DipPixels((width != -1 ? DipPixels(Utils::unscale_value(width)) : ItemWidth(false, _isWithCheckBox, _shortView)) - L::itemPadding - L::remove_size).px() - L::onlineSignLeftPadding.px());
        assert(xPos > L::itemLeftBorder.px());
        return xPos;
    }

    int RenderMore(QPainter &painter, int, bool _shortView, int width)
    {
        QPixmap remove_img;
        remove_img = Utils::parse_image_name(":/resources/contr_options_100.png");
        bool _isWithCheckBox = false;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(GetXOfRemoveImg(_isWithCheckBox, _shortView, width), L::onlineSignY.px() + L::onlineSignSize.px() / 2 - L::remove_size.px() / 2,
            L::remove_size.px(), L::remove_size.px(), remove_img);

        painter.restore();

        const auto xPos = (DipPixels((width != -1 ? DipPixels(Utils::unscale_value(width)) : ItemWidth(false, _isWithCheckBox, _shortView)) - L::itemPadding - L::remove_size).px() - L::onlineSignLeftPadding.px());
        assert(xPos > L::itemLeftBorder.px());
        return xPos;
    }

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, bool _isWithCheckBox, const int leftMargin)
    {
        assert(!status.isEmpty());

        static auto textControl = CreateTextBrowser("status", L::getStatusStylesheet(), L::statusHeight.px());

        const auto maxWidth = (rightBorderPx - L::GetStatusX(_isWithCheckBox, leftMargin).px());
        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());
        const auto elidedString = m.elidedText(status, Qt::ElideRight, maxWidth);

        auto &doc = *textControl->document();
        doc.clear();
        QTextCursor cursor = textControl->textCursor();
        Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, L::statusHeight.px());

        textControl->render(&painter, QPoint(L::GetStatusX(_isWithCheckBox, leftMargin).px(), L::statusY.px()));
    }
}