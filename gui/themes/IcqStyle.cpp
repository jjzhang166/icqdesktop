#include "stdafx.h"

#include "IcqStyle.h"

namespace Themes
{

	IcqStyle::IcqStyle()
	{
	}

	int IcqStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
	{
		return QProxyStyle::styleHint(hint, option, widget, returnData);
	}

}