#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "../history_control/MessagesModel.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "../../main_window/contact_list/RecentsItemRenderer.h"

#include "../../gui_settings.h"

namespace Logic
{
	RecentItemDelegate::ItemKey::ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber)
		: IsSelected(isSelected)
		, IsHovered(isHovered)
		, UnreadDigitsNumber(unreadDigitsNumber)
	{
		assert(unreadDigitsNumber >= 0);
		assert(unreadDigitsNumber <= 2);
	}

	bool RecentItemDelegate::ItemKey::operator < (const ItemKey &_key) const
	{
		if (IsSelected != _key.IsSelected)
		{
			return (IsSelected < _key.IsSelected);
		}

		if (IsHovered != _key.IsHovered)
		{
			return (IsHovered < _key.IsHovered);
		}

		if (UnreadDigitsNumber != _key.UnreadDigitsNumber)
		{
			return (UnreadDigitsNumber < _key.UnreadDigitsNumber);
		}

		return false;
	}

	RecentItemDelegate::RecentItemDelegate(QObject* parent)
		:	QItemDelegate(parent)
		,	StateBlocked_(false)
	{
	}

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const Data::DlgState& dlg, bool fromAlert, bool dragOverlay) const
	{
        if (dlg.AimId_ == "favorites" || dlg.AimId_ == "recents")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());

            ContactList::RenderServiceItem(*painter, dlg.GetText(), dlg.AimId_ == "favorites");

            painter->restore();
            return;
        }
		const auto isMultichat = Logic::GetContactListModel()->isChat(dlg.AimId_);
		const auto state = isMultichat ? QString() : Logic::GetContactListModel()->getState(dlg.AimId_);

        QString slastSeen = "";
        if (!isMultichat)
        {
            slastSeen = QT_TRANSLATE_NOOP("contact_list", "Seen ");
            QString slastSeenSuffix = "";
            const auto lastSeen = Logic::GetContactListModel()->getLastSeen(dlg.AimId_);
            if (lastSeen.isValid())
            {
                const auto current = QDateTime::currentDateTime();
                const auto days = lastSeen.daysTo(current);
                if (days == 0)
                    slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "today");
                else if (days == 1)
                    slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "yesterday");
                else
                    slastSeenSuffix += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
                if (lastSeen.date().year() == current.date().year())
                {
                    slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", " at ");
                    slastSeenSuffix += lastSeen.time().toString(Qt::SystemLocaleShortDate);
                }
            }
            if (!slastSeenSuffix.length())
                slastSeen = "";
            else
                slastSeen += slastSeenSuffix;
        }

		const auto isFilled = !isMultichat;
		bool isDefault = false;

		const auto avatar = GetAvatarStorage()->GetRounded(dlg.AimId_, QString(), Utils::scale_bitmap(Utils::scale_value(56)), state, isFilled, isDefault, false);

        const bool hasMouseOver = (platform::is_apple() ? Logic::GetRecentsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
		const bool isSelected = (option.state & QStyle::State_Selected) && !StateBlocked_;
        const bool isHovered = (option.state & QStyle::State_MouseOver) && !StateBlocked_ && !isSelected && hasMouseOver;

		const auto displayName = Logic::GetContactListModel()->getDisplayName(dlg.AimId_);

		auto deliveryState = ContactList::DeliveryState::NotDelivered;

		const auto isSending = ((dlg.LastMsgId_ == -1) && dlg.Outgoing_);
		if (isSending)
		{
			deliveryState = ContactList::DeliveryState::Sending;
		}

		const auto isDeliveredToServer = ((dlg.LastMsgId_ != -1) && dlg.Outgoing_);
		if (isDeliveredToServer)
		{
			deliveryState = ContactList::DeliveryState::DeliveredToServer;
		}

		const auto isDeliveredToClient = ((dlg.TheirsLastDelivered_ >= dlg.LastMsgId_) && isDeliveredToServer);
		if (isDeliveredToClient)
		{
			deliveryState = ContactList::DeliveryState::DeliveredToClient;
		}

        if (isDeliveredToServer && isMultichat)//force delivered to client status for mchats
        {
            deliveryState = ContactList::DeliveryState::DeliveredToClient;
        }

        const auto showLastMessage = (fromAlert || Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true));

        auto status = (
            showLastMessage ?
                dlg.GetText() :
                (isMultichat ? "online" : (Utils::state_equals_online(state) ? state : slastSeen))
        );

        bool isTyping = false;

        for (auto iter_typing = typings_.rbegin(); iter_typing != typings_.rend(); ++iter_typing)
        {
            if (iter_typing->aimId_ == dlg.AimId_)
            {
                isTyping = true;

                status.clear();

                QString chatterName = iter_typing->getChatterName();

                if (isMultichat)
                {
                    status += iter_typing->getChatterName() + " ";
                }

                status += QT_TRANSLATE_NOOP("contact_list", "typing...");

                break;
            }
        }


        bool isOfficial = dlg.Official_ || Logic::GetContactListModel()->isOfficial(dlg.AimId_);

        bool isDrawLastRead = false;

        QPixmap lastReadAvatar;

        bool isOutgoing = dlg.Outgoing_;
        bool isLastRead = (dlg.LastMsgId_ >= 0 && dlg.TheirsLastRead_ > 0 && dlg.LastMsgId_ <= dlg.TheirsLastRead_);

        if (!isMultichat && isOutgoing && isLastRead && !Logic::GetMessagesModel()->isHasPending(dlg.AimId_))
        {
            lastReadAvatar = *GetAvatarStorage()->GetRounded(dlg.AimId_, QString(), Utils::scale_bitmap(Utils::scale_value(16)), QString(), isFilled, isDefault, false);
            isDrawLastRead = true;
        }

		ContactList::RecentItemVisualData visData(
			dlg.AimId_,
			*avatar,
			state,
            status,
			isHovered,
			isSelected,
            isTyping,
			displayName,
			deliveryState,
			true,
			QDateTime::fromTime_t(dlg.Time_),
			(int)dlg.UnreadCount_, 
            Logic::GetContactListModel()->isMuted(dlg.AimId_),
            dlg.senderNick_,
            isOfficial,
            isDrawLastRead,
            lastReadAvatar);

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		painter->setRenderHint(QPainter::TextAntialiasing);
		painter->setRenderHint(QPainter::SmoothPixmapTransform);
		painter->translate(option.rect.topLeft());

		ContactList::RenderRecentsItem(*painter, visData, fromAlert);

        if (dragOverlay)
            ContactList::RenderRecentsDragOverlay(*painter);

		painter->restore();
	}

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		Data::DlgState dlg = index.data(Qt::DisplayRole).value<Data::DlgState>();
		return paint(painter, option, dlg, false, index == DragIndex_);
	}

	QSize RecentItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& i) const
	{
        if (Logic::GetRecentsModel()->isServiceItem(i))
            return QSize(Utils::scale_value(333), Utils::scale_value(25));

		return QSize(Utils::scale_value(333), Utils::scale_value(68));
	}

	QSize RecentItemDelegate::sizeHintForAlert() const
	{
		return QSize(Utils::scale_value(320), Utils::scale_value(68));
	}

	int RecentItemDelegate::itemSize() const
	{
		return Utils::scale_value(333);
	}

	void RecentItemDelegate::blockState(bool value)
	{
		StateBlocked_= value;
	}
    
    void RecentItemDelegate::addTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter == typings_.end())
        {
            typings_.push_back(_typing);
        }
    }

    void RecentItemDelegate::removeTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter != typings_.end())
        {
            typings_.erase(iter);
        }
    }

    void RecentItemDelegate::setDragIndex(const QModelIndex& index)
    {
        DragIndex_ = index;
    }
}