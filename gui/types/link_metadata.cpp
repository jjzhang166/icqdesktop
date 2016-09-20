#include "stdafx.h"

#include "link_metadata.h"

DATA_NS_BEGIN

LinkMetadata::LinkMetadata()
    : FileSize_(-1)
{
}

LinkMetadata::LinkMetadata(
    const QString &title,
    const QString &description,
    const QString &siteName,
    const QString &contentType,
    const QSize &previewSize,
    const QString &downloadUri,
    const int64_t fileSize)
    : Title_(title)
    , Description_(description)
    , SiteName_(siteName)
    , ContentType_(contentType)
    , PreviewSize_(previewSize)
    , DownloadUri_(downloadUri)
    , FileSize_(fileSize)
{
    assert(PreviewSize_.width() >= 0);
    assert(PreviewSize_.height() >= 0);
    assert(FileSize_ >= -1);
}

LinkMetadata::~LinkMetadata()
{
}

const QString& LinkMetadata::getTitle() const
{
    return Title_;
}

const QString& LinkMetadata::getDescription() const
{
    return Description_;
}

const QString& LinkMetadata::getDownloadUri() const
{
    return DownloadUri_;
}

const QString& LinkMetadata::getSiteName() const
{
    return SiteName_;
}

const QString LinkMetadata::getContentType() const
{
    return ContentType_;
}

const QSize& LinkMetadata::getPreviewSize() const
{
    return PreviewSize_;
}

int64_t LinkMetadata::getFileSize() const
{
    return FileSize_;
}

DATA_NS_END