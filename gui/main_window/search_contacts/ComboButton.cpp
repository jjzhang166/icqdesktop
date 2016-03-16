#include "stdafx.h"
#include "ComboButton.h"

namespace Ui
{
	ComboButton::ComboButton(QWidget* _parent)
		:	QPushButton(_parent)
	{
		setCursor(QCursor(Qt::PointingHandCursor));

	}


	ComboButton::~ComboButton()
	{
	}

}