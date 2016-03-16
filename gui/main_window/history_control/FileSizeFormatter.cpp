#include "stdafx.h"

#include "FileSizeFormatter.h"

namespace HistoryControl
{

	QString FormatFileSize(const int64_t size)
	{
		assert(size >= 0);

		const auto KiB = 1024;
		const auto MiB = 1024 * KiB;
		const auto GiB = 1024 * MiB;

		if (size >= GiB)
		{
			const auto gibSize = ((double)size / (double)GiB);

			return QString("%1 GB").arg(gibSize, 0, 'f', 1);
		}

		if (size >= MiB)
		{
			const auto mibSize = ((double)size / (double)MiB);

			return QString("%1 MB").arg(mibSize, 0, 'f', 1);
		}

		if (size >= KiB)
		{
			const auto kibSize = ((double)size / (double)KiB);

			return QString("%1 KB").arg(kibSize, 0, 'f', 1);
		}

		return QString("%1 B").arg(size);
	}

}