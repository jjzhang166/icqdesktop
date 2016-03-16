#pragma once


namespace Ui
{
	class ContactListItem;
}

namespace Utils
{
    int scale_value(const int _px);
}

namespace Logic
{
	
    class SearchModel;

	class ContactListItemDelegate : public QItemDelegate
	{
	public:
		ContactListItemDelegate(QObject* parent, int _regim);

		virtual ~ContactListItemDelegate();

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

		void blockState(bool value);

        void setDragIndex(const QModelIndex& index);

	private:
        bool StateBlocked_;
        int regim_;
        QModelIndex DragIndex_;
	};
}