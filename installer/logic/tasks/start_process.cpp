#include "stdafx.h"
#include "start_process.h"
#include "../tools.h"

namespace installer
{
	namespace logic
	{
		installer::error start_process()
		{
			QString icq_exe = QString("\"") + get_icq_exe() + QString("\"");
						
			if (!QProcess::startDetached(icq_exe))
				installer::error(errorcode::start_exe, QString("start process: ") + icq_exe);

			return installer::error();
		}
	}
}
