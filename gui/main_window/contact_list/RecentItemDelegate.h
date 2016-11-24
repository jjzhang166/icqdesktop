#pragma once

#include "../../types/message.h"
#include "../../types/typing.h"
#include "Common.h"

namespace Logic
{
	class RecentItemDelegate : public AbstractItemDelegateWithRegim
	{
	public:

		RecentItemDelegate(QObject* parent);

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
		void paint(QPainter *painter, const QStyleOptionViewItem &option, const Data::DlgState& dlgState, bool dragOverlay) const;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
		QSize sizeHintForAlert() const;

		void blockState(bool value);

        void addTyping(const TypingFires& _typing);
        void removeTyping(const TypingFires& _typing);

        void setDragIndex(const QModelIndex& index);

        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;

        virtual void setFixedWidth(int _newWidth) override;

        virtual void setRegim(int _regim) override;

	private:

        std::list<TypingFires> typings_;
        
		struct ItemKey
		{
			const bool IsSelected;

			const bool IsHovered;

			const int UnreadDigitsNumber;

			ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber);

			bool operator < (const ItemKey &_key) const;
		};

		bool StateBlocked_;
        QModelIndex DragIndex_;
        ContactList::ViewParams viewParams_;
	};
}
