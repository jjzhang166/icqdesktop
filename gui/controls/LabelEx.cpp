#include "stdafx.h"
#include "LabelEx.h"

namespace Ui
{
	LabelEx::LabelEx(QWidget* _parent)
		: QLabel(_parent)
	{

	}

	void LabelEx::mouseReleaseEvent(QMouseEvent* _event)
	{
		emit clicked();
		QLabel::mouseReleaseEvent(_event);
	}
}