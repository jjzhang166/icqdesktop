#pragma once

namespace core
{
    class coll_helper;
    enum class file_sharing_content_type;
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

        bool HasUri() const;

        void SetUri(const QString &uri);

        QString ToLogString() const;

        core::file_sharing_content_type getContentType() const;

        void setContentType(core::file_sharing_content_type type);

    private:
        bool IsOutgoing_;

        QString LocalPath_;

        std::unique_ptr<QSize> Size_;

        QString Uri_;

        QString UploadingProcessId_;

        core::file_sharing_content_type ContentType_;
    };

    typedef std::shared_ptr<FileSharingInfo> FileSharingInfoSptr;
}