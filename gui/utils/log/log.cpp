#include "stdafx.h"

#include "log.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

using namespace Ui;

namespace
{
	void log(const QString &type, const QString &area, const QString &text);
}

namespace Log
{
	void trace(const QString& area, const QString& text)
	{
		static const QString type("trace");
		log(type, area, text);
	}

	void info(const QString& area, const QString& text)
	{
		static const QString type("info");
		log(type, area, text);
	}

	void warn(const QString& area, const QString& text)
	{
		static const QString type("warn");
		log(type, area, text);
	}

	void error(const QString& area, const QString& text)
	{
		static const QString type("error");
		log(type, area, text);
	}

}

namespace
{
	void log(const QString &type, const QString &area, const QString &text)
	{
		assert(!type.isEmpty());
		assert(!area.isEmpty());
		assert(!text.isEmpty());

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("type", type);
		collection.set_value_as_qstring("area", area);
		collection.set_value_as_string("text", text.toUtf8().data(), text.toUtf8().size());

		Ui::GetDispatcher()->post_message_to_core("log", collection.get());
	}
}