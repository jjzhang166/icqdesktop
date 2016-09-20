#pragma once

namespace Logic
{
	class LiveChatItemDelegate : public QItemDelegate
	{
	public:
		LiveChatItemDelegate(QWidget* _parent);

		void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

		QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

        void blockState(bool _value);

	private:
        bool stateBlocked_;
        QWidget* parent_;
        mutable QMap<int, int> height_;
	};
}