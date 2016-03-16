#pragma once

namespace Themes
{
	class IcqStyle : public QProxyStyle
	{
	public:
		IcqStyle();

		virtual int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override;

	private:

	};
}