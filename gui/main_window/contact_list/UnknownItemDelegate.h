#pragma once

#include "../../types/message.h"
#include "Common.h"

namespace Ui
{
    class ContactListItem;
}

namespace Logic
{
    class UnknownItemDelegate : public QItemDelegate
    {
    public:

        UnknownItemDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlgState, bool _fromAlert = false, bool _dragOverlay = false) const;
        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        QSize sizeHintForAlert() const;
        void blockState(bool _value);
        
        void setDragIndex(const QModelIndex& _index);

        bool isInAddContactFrame(const QPoint& _p) const;
        bool isInRemoveContactFrame(const QPoint& _p) const;
        bool isInDeleteAllFrame(const QPoint& _p) const;
        
        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;
        void setFixedWidth(int _newWidth);

    private:
        struct ItemKey
        {
            const bool isSelected_;
            const bool isHovered_;
            const int unreadDigitsNumber_;

            ItemKey(const bool _isSelected, const bool _isHovered, const int _unreadDigitsNumber);

            bool operator < (const ItemKey& _key) const;
        };

        bool stateBlocked_;
        QModelIndex dragIndex_;
        ContactList::ViewParams viewParams_;
    };
}