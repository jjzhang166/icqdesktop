#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "MessageStyle.h"

namespace Ui
{
	HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
		: QWidget(parent)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , Selected_(false)
	{
	}

    void HistoryControlPageItem::clearSelection()
    {
        if (Selected_)
        {
            update();
        }

        Selected_ = false;
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::select()
    {
        if (!Selected_)
        {
            update();
        }

        Selected_ = true;
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
        {
            return;
        }

        HasTopMargin_ = value;

        updateGeometry();
    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;

        updateGeometry();
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {
        
    }

    themes::themePtr HistoryControlPageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(aimId_);
    }

    bool HistoryControlPageItem::setLastRead(const bool /*_isLastRead*/)
    {
        return false;
    }

    void HistoryControlPageItem::drawLastReadAvatar(QPainter& _p, const QString& _aimid, const QString& _friendly, const int _rightPadding)
    {
        const QRect rc = rect();

        const int size = Utils::scale_bitmap(MessageStyle::getLastReadAvatarSize());

        bool isDefault = false;
        QPixmap avatar = *Logic::GetAvatarStorage()->GetRounded(_aimid, _friendly, size, QString(), true, isDefault, false);

        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        const int margin = MessageStyle::getLastReadAvatarMargin();

        if (!avatar.isNull())
        {
            _p.drawPixmap(rc.right() - avatarSize - _rightPadding, rc.bottom() - avatarSize - margin, avatarSize, avatarSize, avatar);
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }
}