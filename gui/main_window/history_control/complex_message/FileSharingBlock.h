#pragma once

#include "../../../namespaces.h"
#include "../../../types/link_metadata.h"

#include "FileSharingBlockBase.h"

THEMES_NS_BEGIN

class IThemePixmap;

typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;

THEMES_NS_END

UI_NS_BEGIN

class ActionButtonWidget;
class TextEmojiWidget;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

class IFileSharingBlockLayout;

class FileSharingBlock final : public FileSharingBlockBase
{
    Q_OBJECT

public:
    FileSharingBlock(
        ComplexMessageItem *parent,
        const QString &link,
        const core::file_sharing_content_type type);

    virtual ~FileSharingBlock() override;

    QFont getAuthorNickFont() const;

    QSize getCtrlButtonSize() const;

    QSize getFailedSnapSizeMax() const;

    QSize getOriginalPreviewSize() const;

    QString getShowInDirLinkText() const;

    bool isAuthorVisible() const;

    bool isFailedSnap() const;

    bool isPreviewReady() const;

    virtual bool isSharingEnabled() const override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    void setAuthorNickGeometry(const QRect &rect);

    void setCtrlButtonGeometry(const QRect &rect);

protected:
    virtual void drawBlock(QPainter &p) override;

    virtual void enterEvent(QEvent*) override;

    virtual void initializeFileSharingBlock() override;

    virtual void leaveEvent(QEvent*) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void onMetainfoDownloaded() override;

    virtual bool drag() override;

private:
    void applyClippingPath(QPainter &p, const QRect &previewRect);

    void changeGifPlaybackStatus(const bool isPlaying);

    void connectGeneralSignals(const bool isConnected);

    void connectImageSignals(const bool isConnected);

    void createAuthorNickControl(const QString &_authorName);

    void drawFailedSnap(QPainter &p, const QRect &previewRect);

    void drawPlainFileBlock(QPainter &p, const QRect &frameRect);

    void drawPlainFileBubble(QPainter &p, const QRect &bubbleRect);

    void drawPlainFileFrame(QPainter &p, const QRect &frameRect);

    void drawPlainFileName(QPainter &p, const QRect &filenameRect);

    void drawPlainFileShowInDirLink(QPainter &p, const QRect &linkRect);

    void drawPlainFileSizeAndProgress(QPainter &p, const QString &text, const QRect &fileSizeRect);

    void drawPreview(QPainter &p, const QRect &previewRect);

    void drawPreviewableBlock(QPainter &p, const QRect &previewRect, const QRect &authorAvatarRect, const QRect &authorNickRect);

    void drawSnapAuthor(QPainter &p, const QRect &authorAvatarRect, const QRect &authorNameRect, const QRect &previewRect);

    void initPlainFile();

    void initPreview();

    bool isGifPlaying() const;

    void loadSnapAuthorAvatar(const QString &_uin, const QString &_nick);

    virtual void onDownloadingStarted() override;

    virtual void onDownloadingStopped() override;

    virtual void onDownloaded() override;

    virtual void onDownloading(const int64_t _bytesTransferred, const int64_t _bytesTotal) override;

    virtual void onDownloadingFailed() override;

    void onGifImageVisibilityChanged(const bool isVisible);

    virtual void onLocalCopyInfoReady(const bool isCopyExists) override;

    void onLeftMouseClick(const QPoint &globalPos);

    virtual void onPreviewMetainfoDownloaded(const QString &miniPreviewUri, const QString &fullPreviewUri) override;

    virtual void onRestoreResources() override;

    virtual void onUnloadResources() override;

    void parseLink();

    void pauseGif();

    void playGif(const QString &localPath);

    void requestPreview(const QString &uri);

    void requestSnapMetainfo();

    void sendGenericMetainfoRequests();

    void showFileInDir();

    void showImagePreviewer(const QString &localPath);

    bool shouldDisplayProgressAnimation() const;

    void updateFileTypeIcon();

    void unloadGif();

    QSharedPointer<QMovie> GifImage_;

    QString Id_;

    bool IsBodyPressed_;

    bool IsPausedByUser_;

    bool IsShowInDirLinkPressed_;

    QRect LastContentRect_;

    bool OpenPreviewer_;

    QSize OriginalPreviewSize_;

    QPainterPath PreviewClippingPath_;

    QPixmap Preview_;

    int64_t PreviewRequestId_;

    ActionButtonWidget *CtrlButton_;

    Themes::IThemePixmapSptr SnapExpiredImage_;

    QPixmapSCptr SnapAuthorAvatar_;

    QString SnapAuthorNick_;

    TextEmojiWidget *SnapAuthorNickCtrl_;

    QString SnapAuthorUin_;

    uint64_t SnapId_;

    int64_t SnapMetainfoRequestId_;

    bool IsFailedSnap_;

private Q_SLOTS:
    void onGifFrameUpdated(int frameNumber);

    void onImageDownloadError(qint64 seq, QString rawUri);

    void onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath);

    void onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred);

    void onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta);

    void onSnapAuthorAvatarChanged(QString aimId);

    void onSnapMetainfoDownloaded(int64_t _seq, bool success, uint64_t _snap_id, int64_t _expire_utc, QString _author_uin, QString _author_name);

};

UI_COMPLEX_MESSAGE_NS_END