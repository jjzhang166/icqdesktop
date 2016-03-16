#include "stdafx.h"

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

#include "FileSharingInfo.h"

using namespace core;

namespace HistoryControl
{

	FileSharingInfo::FileSharingInfo(const coll_helper &info)
	{
		IsOutgoing_ = info.get_value_as_bool("outgoing");

		UploadingProcessId_ = info.get_value_as_string("uploading_id", "");

		if (info.is_value_exist("local_path"))
		{
			assert(!info.is_value_exist("uri"));
			assert(IsOutgoing_);
			LocalPath_ = info.get_value_as_string("local_path");
		}
		else
		{
			Uri_ = info.get_value_as_string("uri");
			assert(!Uri_.isEmpty());
		}

		if (!info.is_value_exist("content_type"))
		{
			return;
		}

#ifdef _DEBUG
		const auto content_type = (preview_content_type)info.get_value_as_int("content_type");
		assert(content_type >= preview_content_type::min);
		assert(content_type <= preview_content_type::max);
#endif //_DEBUG

		const auto width = info.get_value_as_int("width");
		const auto height = info.get_value_as_int("height");
		assert(width > 0);
		assert(height > 0);

		Size_.reset(new QSize(width, height));
	}

	const QString& FileSharingInfo::GetUri() const
	{
		assert(!Uri_.isEmpty());
		return Uri_;
	}

	const QString& FileSharingInfo::GetLocalPath()
	{
		assert(IsOutgoing());

		return LocalPath_;
	}

	QSize FileSharingInfo::GetSize() const
	{
		return Size_ ? *Size_ : QSize();
	}

	bool FileSharingInfo::IsOutgoing() const
	{
		return IsOutgoing_;
	}

	const QString& FileSharingInfo::GetUploadingProcessId() const
	{
		assert(IsOutgoing());

		return UploadingProcessId_;
	}

	bool FileSharingInfo::HasLocalPath() const
	{
		return !LocalPath_.isEmpty();
	}

	bool FileSharingInfo::HasSize() const
	{
		return !!Size_;
	}

	QString FileSharingInfo::ToLogString() const
	{
		QString logStr;
		logStr.reserve(512);

		logStr += "\toutgoing=<";
		logStr += logutils::yn(IsOutgoing_);
		logStr += ">\n";

		logStr += "\tlocal_path=<";
		logStr += LocalPath_;
		logStr += ">\n";

		logStr += "\tuploading_id=<";
		logStr += UploadingProcessId_;
		logStr += ">\n";

		logStr += "\turi=<";
		logStr += Uri_;
		logStr += ">";

		return logStr;
	}

}