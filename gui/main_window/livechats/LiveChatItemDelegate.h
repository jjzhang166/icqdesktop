#pragma once

namespace Logic
{
	class LiveChatItemDelegate : public QItemDelegate
	{
	public:
		LiveChatItemDelegate(QWidget* parent);

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

	private:
        QWidget* parent_;
        mutable QMap<int, int> height_;
	};
}