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

        void setItemWidth(int width);

        void setLeftMargin(int margin);

        void setRightMargin(int margin);

        void setRegim(int _regim);

        void setRenderRole(bool render);

	private:
        bool StateBlocked_;
        bool renderRole_;
        int regim_;
        int width_;
        int leftMargin_;
        int rightMargin_;
        QModelIndex DragIndex_;
	};
}