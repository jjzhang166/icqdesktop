#include "stdafx.h"
#include "FlowLayout.h"


namespace Ui
{
	FlowLayout::FlowLayout(QWidget *_parent, int _margin, int _h_spacing, int _v_spacing)
		: QLayout(_parent), h_space_(_h_spacing), v_space_(_v_spacing)
	{
		setContentsMargins(_margin, _margin, _margin, _margin);
	}

	FlowLayout::FlowLayout(int _margin, int _h_spacing, int _v_spacing)
		: h_space_(_h_spacing), v_space_(_v_spacing)
	{
		setContentsMargins(_margin, _margin, _margin, _margin);
	}

	FlowLayout::~FlowLayout()
	{
		QLayoutItem *item;
		while ((item = takeAt(0)))
			delete item;
	}

	void FlowLayout::addItem(QLayoutItem *_item)
	{
		item_list_.append(_item);
	}

	int FlowLayout::horizontalSpacing() const
	{
		if (h_space_ >= 0) {
			return h_space_;
		} else {
			return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
		}
	}

	int FlowLayout::verticalSpacing() const
	{
		if (v_space_ >= 0) {
			return v_space_;
		} else {
			return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
		}
	}

	int FlowLayout::count() const
	{
		return item_list_.size();
	}

	QLayoutItem *FlowLayout::itemAt(int _index) const
	{
		return item_list_.value(_index);
	}

	QLayoutItem *FlowLayout::takeAt(int _index)
	{
		if (_index >= 0 && _index < item_list_.size())
			return item_list_.takeAt(_index);
		else
			return 0;
	}

	Qt::Orientations FlowLayout::expandingDirections() const
	{
		return 0;
	}

	bool FlowLayout::hasHeightForWidth() const
	{
		return true;
	}

	int FlowLayout::heightForWidth(int _width) const
	{
		int height = doLayout(QRect(0, 0, _width, 0), true);
		return height;
	}

	void FlowLayout::setGeometry(const QRect& _rect)
	{
		QLayout::setGeometry(_rect);
		doLayout(_rect, false);
	}

	QSize FlowLayout::sizeHint() const
	{
		return minimumSize();
	}

	QSize FlowLayout::minimumSize() const
	{
		QSize size;
		QLayoutItem *item;
		foreach (item, item_list_)
			size = size.expandedTo(item->minimumSize());

		size += QSize(2*margin(), 2*margin());
		return size;
	}

	int FlowLayout::doLayout(const QRect& _rect, bool _testOnly) const
	{
		int left, top, right, bottom;
		getContentsMargins(&left, &top, &right, &bottom);
		QRect effectiveRect = _rect.adjusted(+left, +top, -right, -bottom);
		int x = effectiveRect.x();
		int y = effectiveRect.y();
		int lineHeight = 0;

		QLayoutItem *item;
		foreach (item, item_list_) {
			QWidget *wid = item->widget();
			int spaceX = horizontalSpacing();
			if (spaceX == -1)
				spaceX = wid->style()->layoutSpacing(
				QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
			int spaceY = verticalSpacing();
			if (spaceY == -1)
				spaceY = wid->style()->layoutSpacing(
				QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
			int nextX = x + item->sizeHint().width() + spaceX;
			if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
				x = effectiveRect.x();
				y = y + lineHeight + spaceY;
				nextX = x + item->sizeHint().width() + spaceX;
				lineHeight = 0;
			}

			if (!_testOnly)
				item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

			x = nextX;
			lineHeight = qMax(lineHeight, item->sizeHint().height());
		}
		return y + lineHeight - _rect.y() + bottom;
	}

	int FlowLayout::smartSpacing(QStyle::PixelMetric _pm) const
	{
		QObject *parent = this->parent();
		if (!parent) {
			return -1;
		} else if (parent->isWidgetType()) {
			QWidget *pw = static_cast<QWidget *>(parent);
			return pw->style()->pixelMetric(_pm, 0, pw);
		} else {
			return static_cast<QLayout *>(parent)->spacing();
		}
	}
}