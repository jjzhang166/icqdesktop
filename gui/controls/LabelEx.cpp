#include "stdafx.h"
#include "LabelEx.h"

namespace Ui
{
	LabelEx::LabelEx(QWidget* parent)
		: QLabel(parent)
	{

	}

	void LabelEx::mouseReleaseEvent(QMouseEvent* event)
	{
		emit clicked();
		QLabel::mouseReleaseEvent(event);
	}
}