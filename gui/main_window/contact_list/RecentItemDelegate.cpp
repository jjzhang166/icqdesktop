#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModelDLG.h"

#include "ContactList.h"
#include "../history_control/MessagesModel.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "RecentsItemRenderer.h"

#include "../../gui_settings.h"

namespace Logic
{
	RecentItemDelegate::RecentItemDelegate(QObject* parent)
		: AbstractItemDelegateWithRegim(parent)
		, StateBlocked_(false)
	{
	}

    QString getLastSeenText(const QString& AimId_)
    {
        QString slastSeen = QT_TRANSLATE_NOOP("contact_list", "Seen ");
        QString slastSeenSuffix = "";
        const auto lastSeen = Logic::getContactListModel()->getLastSeen(AimId_);
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
        return slastSeen;
    }

    ContactList::DeliveryState getDeliveryState(const Data::DlgState& dlg, bool isMultichat)
    {
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
        return deliveryState;
    }

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const Data::DlgState& dlg, bool dragOverlay) const
	{
        if (dlg.AimId_ == "favorites" || dlg.AimId_ == "recents")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());

            ContactList::RenderServiceItem(*painter, dlg.GetText(), dlg.AimId_ == "favorites", dlg.AimId_ == "recents" /* drawLine */, viewParams_);

            painter->restore();
            return;
        }
        else if (dlg.AimId_ == "unknowns")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());
            
            ContactList::RenderUnknownsHeader(*painter, QT_TRANSLATE_NOOP("contact_list", "Unknown contacts"), Logic::getUnknownsModel()->totalUnreads(), viewParams_);
            
            painter->restore();
            return;
        }
        else if (dlg.AimId_ == "contacts" || dlg.AimId_ == "all messages")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());

            auto text = dlg.GetText();
            ContactList::RenderServiceItem(*painter, text, dlg.AimId_ == "favorites" /* renderState */, dlg.AimId_ == "recents" /* drawLine */, viewParams_);

            painter->restore();
            return;
        }

		const auto isMultichat = Logic::getContactListModel()->isChat(dlg.AimId_);
		const auto state = isMultichat ? QString() : Logic::getContactListModel()->getState(dlg.AimId_);
        const auto slastSeen = isMultichat ? QString() : getLastSeenText(dlg.AimId_);

		const auto isFilled = !isMultichat;
		
        bool isDefaultAvatar = false;
		const auto avatar = GetAvatarStorage()->GetRounded(dlg.AimId_, QString(), Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).avatarH().px())
            , state, isFilled, isDefaultAvatar, false, ContactList::GetRecentsParams(viewParams_.regim_).isCL());

        const auto hasMouseOver = (platform::is_apple() ? Logic::getRecentsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
		const auto isSelected = (option.state & QStyle::State_Selected) && !StateBlocked_;
        const auto isHovered = (option.state & QStyle::State_MouseOver) && !StateBlocked_ && !isSelected && hasMouseOver;

		const auto displayName = Logic::getContactListModel()->getDisplayName(dlg.AimId_);
        const auto deliveryState = getDeliveryState(dlg, isMultichat);

        const auto showLastMessage = viewParams_.regim_ == ::Logic::MembersWidgetRegim::FROM_ALERT
            || viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH
            || Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);

        auto status = (
            showLastMessage ?
                dlg.GetText() :
                (isMultichat ? "online" : (Utils::stateEqualsOnline(state) ? state : slastSeen))
        );

        bool isTyping = false;

        if (viewParams_.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
            for (auto iter_typing = typings_.rbegin(); iter_typing != typings_.rend(); ++iter_typing)
            {
                if (iter_typing->aimId_ == dlg.AimId_)
                {
                    isTyping = true;

                    status.clear();

                    if (isMultichat)
                    {
                        status += iter_typing->getChatterName() + " ";
                    }

                    status += QT_TRANSLATE_NOOP("contact_list", "typing...");

                    break;
                }
            }

        const auto isOfficial = dlg.Official_ || Logic::getContactListModel()->isOfficial(dlg.AimId_);
        auto isDrawLastRead = false;

        QPixmap lastReadAvatar;

        bool isOutgoing = dlg.Outgoing_;
        bool isLastRead = (dlg.LastMsgId_ >= 0 && dlg.TheirsLastRead_ > 0 && dlg.LastMsgId_ <= dlg.TheirsLastRead_);

        if (!isMultichat && isOutgoing && isLastRead && !Logic::GetMessagesModel()->isHasPending(dlg.AimId_))
        {
            lastReadAvatar = *GetAvatarStorage()->GetRounded(dlg.AimId_, QString(), Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).getLastReadAvatarSize())
                , QString(), isFilled, isDefaultAvatar, false, ContactList::GetRecentsParams(viewParams_.regim_).isCL());
            isDrawLastRead = true;
        }

		ContactList::RecentItemVisualData visData(
			dlg.AimId_,
			*avatar,
			state,
            status,
			isHovered,
			isSelected,
            displayName.isEmpty() ? dlg.AimId_ : displayName,
			true /* haveLastSeen */,
			QDateTime::fromTime_t(dlg.Time_),
			(int)dlg.UnreadCount_, 
            Logic::getContactListModel()->isMuted(dlg.AimId_),
            dlg.senderNick_,
            isOfficial,
            isDrawLastRead,
            lastReadAvatar,
            isTyping,
			deliveryState,
            dlg.SearchTerm_,
            dlg.HasLastMsgId(),
            dlg.SearchedMsgId_);

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		painter->setRenderHint(QPainter::TextAntialiasing);
		painter->setRenderHint(QPainter::SmoothPixmapTransform);
		painter->translate(option.rect.topLeft());

		ContactList::RenderRecentsItem(*painter, visData, viewParams_);

        if (dragOverlay)
            ContactList::RenderRecentsDragOverlay(*painter, viewParams_);

		painter->restore();
	}

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		Data::DlgState dlg = index.data(Qt::DisplayRole).value<Data::DlgState>();
		return paint(painter, option, dlg, index == DragIndex_);
	}

	QSize RecentItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& i) const
	{
        auto width = ContactList::GetRecentsParams(viewParams_.regim_).itemWidth().px();

        if (viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (Logic::getSearchModelDLG()->isServiceItem(i.row()))
            {
                return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).serviceItemHeight().px());
            }
        }
        else
        {
            if (Logic::getRecentsModel()->isServiceItem(i))
            {
                if (Logic::getRecentsModel()->isUnknownsButton(i))
                    return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).unknownsItemHeight().px());
                else
                    return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).serviceItemHeight().px());
            }
        }
		return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).itemHeight().px());
	}

	QSize RecentItemDelegate::sizeHintForAlert() const
	{
		return QSize(ContactList::GetRecentsParams(viewParams_.regim_).itemWidthAlert().px(), ContactList::GetRecentsParams(viewParams_.regim_).itemHeight().px());
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

    void RecentItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool RecentItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void RecentItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }

    void RecentItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

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
}
