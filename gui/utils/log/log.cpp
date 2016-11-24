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

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_content_type arg)
{
    using namespace core;

    assert(arg > file_sharing_content_type::min);
    assert(arg < file_sharing_content_type::max);

    switch (arg)
    {
        case file_sharing_content_type::gif: return (oss << "gif");
        case file_sharing_content_type::image: return (oss << "image");
        case file_sharing_content_type::ptt: return (oss << "ptt");
        case file_sharing_content_type::snap_gif: return (oss << "snap_gif");
        case file_sharing_content_type::snap_image: return (oss << "snap_image");
        case file_sharing_content_type::snap_video: return (oss << "snap_video");
        case file_sharing_content_type::undefined: return (oss << "undefined");
        case file_sharing_content_type::video: return (oss << "video");
        default:
            assert(!"unexpected file sharing content type");
    }

    return (oss << "#unknown");
}

QTextStream& operator <<(QTextStream &oss, const core::file_sharing_function arg)
{
    using namespace core;

    assert(arg > file_sharing_function::min);
    assert(arg < file_sharing_function::max);

    switch(arg)
    {
        case file_sharing_function::check_local_copy_exists:
            oss << "check_local_copy_exists";
            break;

        case file_sharing_function::download_file:
            oss << "download_file";
            break;

        case file_sharing_function::download_file_metainfo:
            oss << "download_file_metainfo";
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << "download_preview_metainfo";
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
    }

    return oss;
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