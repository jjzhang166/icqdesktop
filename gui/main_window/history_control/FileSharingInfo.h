#pragma once

namespace core
{
	class coll_helper;
}

namespace HistoryControl
{
	class FileSharingInfo
	{
	public:
		FileSharingInfo(const core::coll_helper &info);

		const QString& GetLocalPath();

		const QString& GetUploadingProcessId() const;

		QSize GetSize() const;

		const QString& GetUri() const;

		bool IsOutgoing() const;

		bool HasLocalPath() const;

		bool HasSize() const;

		QString ToLogString() const;

	private:
		bool IsOutgoing_;

		QString LocalPath_;

		std::unique_ptr<QSize> Size_;

		QString Uri_;

		QString UploadingProcessId_;
	};

	typedef std::shared_ptr<FileSharingInfo> FileSharingInfoSptr;
}