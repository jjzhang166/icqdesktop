#include "stdafx.h"

#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "SearchModel.h"
#include "SearchMembersModel.h"
#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "ContactListModel.h"
#include "ChatMembersModel.h"
#include "ContactList.h"

namespace Logic
{
    ContactListItemDelegate::ContactListItemDelegate(QObject* parent, int _regim)
        : QItemDelegate(parent)
        , StateBlocked_(false)
        , renderRole_(false)
    {
        viewParams_.regim_ = _regim;
    }

    ContactListItemDelegate::~ContactListItemDelegate()
    {
    }

    void ContactListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const auto searchMemberModel = qobject_cast<const Logic::SearchMembersModel*>(index.model());
        const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(index.model());

        bool isGroup = false;
        QString displayName, status, state;
        QString aimId;
        bool haveLastSeen = false, isChecked = false, isChatMember = false, isOfficial = false;
        QDateTime lastSeen;

        bool hasMouseOver = true;
        if (platform::is_apple())
        {
            if (const auto customModel = qobject_cast<const Logic::CustomAbstractListModel *>(index.model()))
                hasMouseOver = customModel->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver);
        }

        Data::Contact* contact_in_cl = NULL;
        QString role;

        if (membersModel || searchMemberModel)
        {
            auto cont = index.data(Qt::DisplayRole).value<Data::ChatMemberInfo*>();
            displayName = cont->getFriendly();
            if (renderRole_)
                role = cont->Role_;

            aimId = cont->AimId_;
            auto contact_item = Logic::getContactListModel()->getContactItem(aimId);
            contact_in_cl = contact_item == NULL ? NULL : contact_item->Get();
        }
        else
        {
            contact_in_cl = index.data(Qt::DisplayRole).value<Data::Contact*>();
        }

        if (contact_in_cl)
        {
            isGroup = (contact_in_cl->GetType() == Data::GROUP);
            displayName = contact_in_cl->GetDisplayName();
            aimId = contact_in_cl->AimId_;
            status = contact_in_cl->StatusMsg_;
            state = contact_in_cl->State_;
            haveLastSeen = contact_in_cl->HaveLastSeen_;
            lastSeen = contact_in_cl->LastSeen_;
            isChecked = contact_in_cl->IsChecked_;
            isOfficial = contact_in_cl->IsOfficial_;

        }

        auto chatMembersModel = Logic::getChatMembersModel();
        if (!!chatMembersModel)
        {
            isChatMember = chatMembersModel->isContactInChat(aimId);
        }

        const bool isSelected = ((option.state & QStyle::State_Selected) && !isGroup) && !StateBlocked_;
        const bool isHovered =  ((option.state & QStyle::State_MouseOver) && !isGroup) && !StateBlocked_ && !isSelected && hasMouseOver;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(option.rect.topLeft());

        if (isGroup)
        {
            ContactList::RenderGroupItem(*painter, displayName, viewParams_);
        }
        else
        {
            const auto isMultichat = Logic::getContactListModel()->isChat(aimId);
            const auto isFilled = !isMultichat;
            auto isDefault = false;

            const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimId, displayName, Utils::scale_bitmap(ContactList::GetContactListParams().avatarH().px())
                , isMultichat ? QString() : state, isFilled, isDefault, false /* _regenerate */ , ContactList::GetContactListParams().isCL());
            const ContactList::VisualDataBase visData(aimId, *avatar, state, status, isHovered, isSelected, displayName, haveLastSeen, lastSeen
                , isChecked, isChatMember, isOfficial, false /* draw last read */, QPixmap() /* last seen avatar*/, role, 0 /* unread count */);

            ContactList::ViewParams viewParams(viewParams_.regim_, membersModel && membersModel->isShortView_, viewParams_.fixedWidth_, viewParams_.leftMargin_, viewParams_.rightMargin_);
            ContactList::RenderContactItem(*painter, visData, viewParams);
        }

        if (index == DragIndex_)
        {
            ContactList::RenderContactsDragOverlay(*painter);
        }

        painter->restore();
    }

    QSize ContactListItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(index.model());
        const auto width = viewParams_.fixedWidth_ == -1 ? ContactList::GetContactListParams().itemWidth().px() : viewParams_.fixedWidth_;
        if (!membersModel)
        {
            const auto cont = index.data(Qt::DisplayRole).value<Data::Contact*>();
            if (!cont)
            {
                return QSize(width, ContactList::ContactItemHeight());
            }

            const auto isGroup = (cont->GetType() == Data::GROUP);
            if (isGroup)
            {
                return QSize(width, ContactList::GroupItemHeight());
            }
        }
        return QSize(width, ContactList::ContactItemHeight());
    }

    void ContactListItemDelegate::blockState(bool value)
    {
        StateBlocked_= value;
    }

    void ContactListItemDelegate::setDragIndex(const QModelIndex& index)
    {
        DragIndex_ = index;
    }

    void ContactListItemDelegate::setItemWidth(int width)
    {
        viewParams_.fixedWidth_ = width;
    }

    void ContactListItemDelegate::setLeftMargin(int margin)
    {
        viewParams_.leftMargin_ = margin;
    }

    void ContactListItemDelegate::setRightMargin(int margin)
    {
        viewParams_.rightMargin_ = margin;
    }

    void ContactListItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

    void ContactListItemDelegate::setRenderRole(bool render)
    {
        renderRole_ = render;
    }
}
