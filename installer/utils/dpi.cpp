#include "stdafx.h"
#include "dpi.h"

namespace installer
{
	namespace ui
	{
		namespace dpi
		{
			double scale_coefficient = 1.0;

			bool init()
			{
				double dpi_count = QApplication::primaryScreen()->logicalDotsPerInchX();

				scale_coefficient = ((std::min)(dpi_count / (double)(96), double(2.0)));

				return true;
			}

			double scale(const double& _value)
			{
				static bool res = init();

				return (scale_coefficient * _value);
			}
		}
	}
}
