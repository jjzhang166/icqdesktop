#pragma once

#include "../../../namespaces.h"

#include "GenericBlock.h"

CORE_NS_BEGIN

enum class file_sharing_content_type;

CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout;

class FileSharingBlockBase : public GenericBlock
{
    Q_OBJECT

public:
    FileSharingBlockBase(
        ComplexMessageItem *_parent,
        const QString &_link,
        const core::file_sharing_content_type _type);

    virtual ~FileSharingBlockBase() override;

    virtual void clearSelection() override;

    virtual QString formatRecentsText() const final override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    QString getProgressText() const;

    virtual QString getSelectedText() const override;

    virtual bool hasRightStatusPadding() const override;

    virtual void initialize() final override;

    virtual bool isBubbleRequired() const override;

    virtual bool isSelected() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual bool containsImage() const override;

protected:
    void checkExistingLocalCopy();

    const QString& getFileLocalPath() const;

    const QString& getFilename() const;

    IFileSharingBlockLayout* getFileSharingLayout() const;

    QString getFileSharingId() const;

    int64_t getFileSize() const;

    const QString& getLink() const;

    core::file_sharing_content_type getType() const;

    const QString& getDirectUri() const;

    virtual void initializeFileSharingBlock() = 0;

    bool isFileDownloaded() const;

    bool isFileDownloading() const;

    bool isGifImage() const;

    bool isImage() const;

    bool isPlainFile() const;

    bool isPreviewable() const;

    bool isSnap() const;

    bool isVideo() const;

    virtual void onDownloadingStarted() = 0;

    virtual void onDownloadingStopped() = 0;

    virtual void onDownloaded() = 0;

    virtual void onDownloading(const int64_t bytesTransferred, const int64_t bytesTotal) = 0;

    virtual void onDownloadingFailed() = 0;

    virtual void onLocalCopyInfoReady(const bool isCopyExists) = 0;

    virtual void onMenuCopyLink() final override;

    virtual void onMenuCopyFile() final override;

    virtual void onMenuSaveFileAs() final override;

    virtual void onMetainfoDownloaded() = 0;

    virtual void onPreviewMetainfoDownloaded(const QString &miniPreviewUri, const QString &fullPreviewUri) = 0;

    void requestMetainfo(const bool isPreview);

    void requestDirectUri(const QObject* _object, std::function<void(bool _res, const QString& _uri)> _callback);

    void setBlockLayout(IFileSharingBlockLayout *_layout);

    void setSelected(const bool isSelected);

    void startDownloading(const bool sendStats);

    void stopDownloading();

private:
    void connectSignals(const bool isConnected);

    int64_t BytesTransferred_;

    int64_t CheckLocalCopyRequestId_;

    bool CopyFile_;

    int64_t DownloadRequestId_;

    QString FileLocalPath_;

    int64_t FileMetaRequestId_;

    int64_t fileDirectLinkRequestId_;

    QString Filename_;

    int64_t FileSizeBytes_;

    bool IsSelected_;

    IFileSharingBlockLayout *Layout_;

    const QString Link_;

    int64_t PreviewMetaRequestId_;

    QString SaveAs_;

    const core::file_sharing_content_type Type_;

    QString directUri_;

private Q_SLOTS:
    void onFileDownloaded(qint64 seq, QString rawUri, QString localPath);

    void onFileDownloading(qint64 seq, QString rawUri, qint64 bytesTransferred, qint64 bytesTotal);

    void onFileMetainfoDownloaded(qint64 seq, QString filename, QString downloadUri, qint64 size);

    void onFileSharingError(qint64 seq, QString rawUri, qint32 errorCode);

    void onLocalCopyChecked(qint64 seq, bool success, QString localPath);

    void onPreviewMetainfoDownloadedSlot(qint64 seq, QString miniPreviewUri, QString fullPreviewUri);

};

UI_COMPLEX_MESSAGE_NS_END