#include "stdafx.h"

#include "../../core_dispatcher.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"

#include "Common.h"
#include "ContactListItemRenderer.h"
#include "ContactList.h"

namespace
{
    using namespace ContactList;

    void RenderCheckbox(QPainter &painter, const int x, const VisualDataBase &visData, ContactListParams& _contactListPx);

    void RenderRole(QPainter &painter, const int x, const int y, const QString& role, ContactListParams& _contactListPx);

    int RenderDate(QPainter &painter, const QDateTime &date, const VisualDataBase &item, ContactListParams& _contactListPx, const ViewParams& viewParams_);

    int RenderRemove(QPainter &painter, ContactListParams& _contactListPx, const ViewParams& viewParams_);

    void RenderApprove(QPainter &painter, int& rightBorder, ContactListParams& _contactListPxr);

    int RenderMore(QPainter &painter, ContactListParams& _contactListPx, const ViewParams& viewParams_);

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, ContactListParams& _contactListPx);
}

namespace ContactList
{
    void RenderServiceContact(QPainter &painter, const bool _isHovered, const bool _isActive
        , QString _name, Data::ContactType _type, int leftMargin, const ViewParams& viewParams_)
    {
        auto contactListPx = GetContactListParams();
        contactListPx.setLeftMargin(leftMargin);

        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::NoBrush);

        RenderMouseState(painter, _isHovered, _isActive, contactListPx, viewParams_);
        if (_type == Data::VIEW_ALL_MEMBERS)
        {
            auto x = DipPixels(ItemWidth(viewParams_) - contactListPx.itemPadding());
            QRect rect(contactListPx.avatarX().px() + leftMargin, contactListPx.avatarY().px(), x.px(), Utils::scale_value(1));
            painter.fillRect(rect, QBrush(QColor("#dadada")));
        }

        static const auto hoveredIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContactHovered);
        static const auto plainIcon = Themes::GetPixmap(Themes::PixmapResourceId::ContactListAddContact);

        const auto &icon = plainIcon;
        if (_type == Data::ContactType::ADD_CONTACT)
            icon->Draw(painter, contactListPx.avatarX().px() + leftMargin, contactListPx.avatarY().px(), contactListPx.avatarW().px(), contactListPx.avatarH().px());

        if (_type == Data::EMPTY_IGNORE_LIST)
        {
            painter.setPen(contactListPx.emptyIgnoreListColor());
            painter.setFont(contactListPx.emptyIgnoreListFont().font());
        }
        else
        {
            painter.setPen(Ui::CommonStyle::getLinkColor());
            painter.setFont(contactListPx.addContactFont().font());
        }

        painter.translate(contactListPx.GetAddContactX().px(), contactListPx.addContactY().px());
        painter.drawText(0, 0, _name);
        painter.restore();

        contactListPx.resetParams();
    }

    void RenderContactItem(QPainter &painter, VisualDataBase item, ViewParams _viewParams)
    {
        const auto regim = _viewParams.regim_;

        auto contactListPx = GetContactListParams();
        contactListPx.setLeftMargin(_viewParams.leftMargin_);
        contactListPx.setWithCheckBox(regim == Logic::MembersWidgetRegim::SELECT_MEMBERS);

        painter.save();

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(painter, item.IsHovered_, item.IsSelected_, contactListPx, _viewParams);

        if (regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
        {
            RenderCheckbox(painter, (contactListPx.avatarX() - contactListPx.checkboxWidth() - dip(10)).px(), item, contactListPx);
        }

        RenderAvatar(painter, contactListPx.avatarX().px() + _viewParams.leftMargin_, item.Avatar_, contactListPx);

        auto rightBorderPx = 0;

        if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
        {
            rightBorderPx = RenderRemove(painter, contactListPx, _viewParams);
            RenderApprove(painter, rightBorderPx, contactListPx);
            RenderContactName(painter, item, contactListPx.contactNameCenterY().px(), rightBorderPx, _viewParams, contactListPx);
            painter.restore();
            return;
        }
        else if (item.IsHovered_)
        {
            if (Logic::is_delete_members_regim(regim))
            {
                rightBorderPx = RenderRemove(painter, contactListPx, _viewParams);
            }
            else if (Logic::is_admin_members_regim(regim))
            {
                rightBorderPx = RenderMore(painter, contactListPx, _viewParams);
            }
        }


        if (!item.IsOnline())
        {
            rightBorderPx = RenderDate(painter, item.LastSeen_, item, contactListPx, _viewParams);
        }
        else
        {
            rightBorderPx = CorrectItemWidth(ItemWidth(_viewParams).px(), _viewParams.fixedWidth_);
            rightBorderPx -= _viewParams.rightMargin_;
        }

        if (item.HasStatus())
        {
            if (item.role_ == "admin" || item.role_ == "moder")
            {
                RenderRole(painter, contactListPx.GetContactNameX().px(), contactListPx.contactNameTopY().px(), item.role_, contactListPx);
                _viewParams.leftMargin_ += contactListPx.role_offset().px();
            }

            RenderContactName(painter, item, contactListPx.contactNameTopY().px(), rightBorderPx, _viewParams, contactListPx);
            RenderStatus(painter, item.GetStatus(), rightBorderPx, contactListPx);
        }
        else
        {
            if (item.role_ == "admin" || item.role_ == "moder")
            {
                RenderRole(painter, contactListPx.GetContactNameX().px(), contactListPx.contactNameCenterY().px(), item.role_, contactListPx);
                _viewParams.leftMargin_ += contactListPx.role_offset().px();
            }

            RenderContactName(painter, item, contactListPx.contactNameCenterY().px(), rightBorderPx, _viewParams, contactListPx);
        }

        painter.restore();
        contactListPx.resetParams();
    }

    void RenderGroupItem(QPainter &painter, const QString &groupName, const ViewParams& viewParams_)
    {
        auto contactListPx = GetContactListParams();
        contactListPx.setLeftMargin(viewParams_.leftMargin_);
        
        painter.save();
        painter.setPen(contactListPx.groupColor());
        painter.setBrush(Qt::NoBrush);
        painter.setFont(contactListPx.groupFont().font());

        painter.translate(contactListPx.GetGroupX().px(), contactListPx.groupY().px());
        painter.drawText(0, 0, groupName);

        painter.restore();
        contactListPx.resetParams();
    }

    void RenderContactsDragOverlay(QPainter &painter)
    {
        auto contactListPx = GetContactListParams();
        painter.save();

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(0, 0, ItemWidth(false, false, false).px(), contactListPx.itemHeight().px(), QBrush(QColor(255, 255, 255, 255 * 0.9)));
        painter.setBrush(QBrush(QColor(255, 255, 255, 0)));
        QPen pen (QColor(0x57, 0x9e, 0x1c), contactListPx.dragOverlayBorderWidth().px(), Qt::DashLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawRoundedRect(
            contactListPx.dragOverlayPadding().px(),
            contactListPx.dragOverlayVerPadding().px(),
            ItemWidth(false, false, false).px() - contactListPx.itemPadding().px() - Utils::scale_value(1),
            contactListPx.itemHeight().px() - contactListPx.dragOverlayVerPadding().px(),
            contactListPx.dragOverlayBorderRadius().px(),
            contactListPx.dragOverlayBorderRadius().px()
        );

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (ItemWidth(false, false, false).px() / 2) - (p.width() / 2. / ratio);
        int y = (contactListPx.itemHeight().px() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);

        painter.restore();
    }

    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView, int width)
    {
        auto contactListPx = GetContactListParams();
        return CorrectItemWidth(ItemWidth(false, _isWithCheckBox, _shortView).px(), width) - contactListPx.itemPadding().px() - DipPixels(contactListPx.onlineSignSize() + contactListPx.remove_size()).px();
    }
}

namespace
{
    void RenderCheckbox(QPainter &painter, const int x, const VisualDataBase &visData, ContactListParams& _contactListPx)
    {
        const auto img = visData.isChatMember_ ?
            ":/resources/dialog_closechat_100.png"
            : (visData.isCheckedBox_
            ? ":/resources/widgets/content_check_100.png"
            : ":/resources/widgets/content_uncheck_100.png");
        const auto checkbox = Utils::parse_image_name(img);

        painter.drawPixmap(x, _contactListPx.getItemMiddleY() - _contactListPx.checkboxWidth().px() / 2, _contactListPx.checkboxWidth().px(), _contactListPx.checkboxWidth().px(), checkbox);
    }

    void RenderRole(QPainter &painter, const int x, const int y, const QString& role, ContactListParams& _contactListPx)
    {
        QPixmap rolePixmap;
        if (role == "admin")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_ultraadmin_100.png"));
        else if (role == "moder")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_king_100.png"));
        else
            return;
        Utils::check_pixel_ratio(rolePixmap);

        painter.drawPixmap(x, y + _contactListPx.role_ver_offset().px(), rolePixmap);
    }

    int RenderDate(QPainter &painter, const QDateTime &ts, const VisualDataBase &item, ContactListParams& _contactListPx, const ViewParams& _viewParams)
    {
        const auto regim = _viewParams.regim_;
        const auto isWithCheckBox = regim == Logic::MembersWidgetRegim::SELECT_MEMBERS;
        const auto timeXRight = CorrectItemWidth(ItemWidth(_viewParams).px() - _contactListPx.itemPadding().px(), _viewParams.fixedWidth_)
            - _viewParams.rightMargin_ - _contactListPx.itemPadding().px();

        if (!ts.isValid())
        {
            return timeXRight;
        }

        const auto timeStr = FormatTime(ts);
        if (timeStr.isEmpty())
        {
            return timeXRight;
        }

        static QFontMetrics m(_contactListPx.timeFont().font());
        const auto leftBearing = m.leftBearing(timeStr[0]);
        const auto rightBearing = m.rightBearing(timeStr[timeStr.length() - 1]);
        const auto timeWidth = (m.tightBoundingRect(timeStr).width() + leftBearing + rightBearing);
        const auto timeX = timeXRight - timeWidth;

        if ((!isWithCheckBox && !Logic::is_delete_members_regim(regim) && !Logic::is_admin_members_regim(regim))
            || (Logic::is_delete_members_regim(regim) && !item.IsHovered_)
            || (!Logic::is_admin_members_regim(regim) && !item.IsHovered_))
        {
            painter.save();
            painter.setFont(_contactListPx.timeFont().font());
            painter.setPen(_contactListPx.timeFontColor());
            painter.drawText(timeX, _contactListPx.timeY().px(), timeStr);
            painter.restore();
        }

        return timeX;
    }

    int RenderRemove(QPainter &painter, ContactListParams& _contactListPx, const ViewParams& _viewParams)
    {
        const auto _shortView = _viewParams.shortView_;
        const auto width = _viewParams.fixedWidth_;
        const auto remove_img = Utils::parse_image_name(":/resources/contr_clear_100.png");
        const auto isWithCheckBox = false;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(GetXOfRemoveImg(isWithCheckBox, _shortView, width), _contactListPx.onlineSignY().px() + _contactListPx.onlineSignSize().px() / 2 - _contactListPx.remove_size().px() / 2,
            _contactListPx.remove_size().px(), _contactListPx.remove_size().px(), remove_img);

        painter.restore();

        const auto xPos = CorrectItemWidth(ItemWidth(_viewParams).px(), width) - _contactListPx.itemPadding().px() - _contactListPx.remove_size().px() - _contactListPx.onlineSignLeftPadding().px();
        assert(xPos > _contactListPx.itemLeftBorder().px());
        return xPos;
    }

    void RenderApprove(QPainter &painter, int& rightBorder, ContactListParams& _contactListPx)
    {
        QPixmap approve_img = Utils::parse_image_name(":/resources/cl_addcontact_100.png");
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(rightBorder - _contactListPx.approve_size().px(), _contactListPx.onlineSignY().px() + _contactListPx.onlineSignSize().px() / 2 - _contactListPx.approve_size().px() / 2,
            _contactListPx.approve_size().px(), _contactListPx.approve_size().px(), approve_img);

        painter.restore();

        rightBorder -= _contactListPx.approve_size().px();
    }

    int RenderMore(QPainter &painter, ContactListParams& _contactListPx, const ViewParams& _viewParams)
    {
        const auto width = _viewParams.fixedWidth_;
        const auto remove_img = Utils::parse_image_name(":/resources/contr_options_100.png");
        const auto isWithCheckBox = false;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.drawPixmap(GetXOfRemoveImg(isWithCheckBox, _viewParams.shortView_, width), _contactListPx.onlineSignY().px() + _contactListPx.onlineSignSize().px() / 2 - _contactListPx.remove_size().px() / 2,
            _contactListPx.remove_size().px(), _contactListPx.remove_size().px(), remove_img);

        painter.restore();

        const auto xPos = CorrectItemWidth(ItemWidth(_viewParams).px(), width) - _contactListPx.itemPadding().px() - _contactListPx.remove_size().px()
            - _contactListPx.onlineSignLeftPadding().px();
        assert(xPos > _contactListPx.itemLeftBorder().px());
        return xPos;
    }

    void RenderStatus(QPainter &painter, const QString &status, const int rightBorderPx, ContactListParams& _contactListPx)
    {
        assert(!status.isEmpty());

        static auto textControl = CreateTextBrowser("status", _contactListPx.getStatusStylesheet(), _contactListPx.statusHeight().px());

        const auto maxWidth = rightBorderPx - _contactListPx.GetStatusX().px();
        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());
        const auto elidedString = m.elidedText(status, Qt::ElideRight, maxWidth);

        auto &doc = *textControl->document();
        doc.clear();
        QTextCursor cursor = textControl->textCursor();
        Logic::Text2Doc(elidedString, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, _contactListPx.statusHeight().px());

        textControl->render(&painter, QPoint(_contactListPx.GetStatusX().px(), _contactListPx.statusY().px()));
    }
}
