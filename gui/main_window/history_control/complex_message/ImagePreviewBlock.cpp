#include "stdafx.h"

#include "../../../core_dispatcher.h"
#include "../../../main_window/MainWindow.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../KnownFileTypes.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlockLayout.h"
#include "Selection.h"
#include "Style.h"

#include "ImagePreviewBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ImagePreviewBlock::ImagePreviewBlock(ComplexMessageItem *parent, const QString& aimId, const QString &imageUri, const QString &imageType)
    : GenericBlock(
          parent,
          imageUri,
          (MenuFlags)(MenuFlagFileCopyable | MenuFlagLinkCopyable | MenuFlagOpenInBrowser),
          true)
    , AimId_(aimId)
    , ImageUri_(imageUri)
    , ImageType_(imageType)
    , Layout_(nullptr)
    , PreviewDownloadSeq_(-1)
    , PressedOverPreview_(false)
    , FullImageDownloadSeq_(-1)
    , IsSelected_(false)
    , ActionButton_(nullptr)
    , CopyFile_(false)
    , IsGifPlaying_(false)
    , FileSize_(-1)
    , SnapId_(0)
    , SnapMetainfoRequestId_(-1)
    , IsPausedByUser_(false)
    , MaxPreviewWidth_(0)
{
    assert(!ImageUri_.isEmpty());
    assert(!ImageType_.isEmpty());

    if (ImageUri_.endsWith(QChar::Space) || ImageUri_.endsWith(QChar::LineFeed))
        ImageUri_.truncate(ImageUri_.length() - 1);

    Layout_ = new ImagePreviewBlockLayout();
    setLayout(Layout_);
}

ImagePreviewBlock::~ImagePreviewBlock()
{
}

void ImagePreviewBlock::clearSelection()
{
    if (!IsSelected_)
    {
        return;
    }

    IsSelected_ = false;

    update();
}

bool ImagePreviewBlock::hasActionButton() const
{
    return ActionButton_;
}

QPoint ImagePreviewBlock::getActionButtonLogicalCenter() const
{
    assert(ActionButton_);

    return ActionButton_->getLogicalCenter();
}

QSize ImagePreviewBlock::getActionButtonSize() const
{
    assert(ActionButton_);

    return ActionButton_->sizeHint();
}

IItemBlockLayout* ImagePreviewBlock::getBlockLayout() const
{
    return Layout_;
}

QSize ImagePreviewBlock::getPreviewSize() const
{
    assert (!Preview_.isNull());

    return Preview_.size();
}

QString ImagePreviewBlock::getSelectedText(bool isFullSelect) const
{
    assert(!ImageUri_.isEmpty());

    if (IsSelected_)
    {
        return getSourceText();
    }

    return QString();
}

void ImagePreviewBlock::hideActionButton()
{
    assert(ActionButton_);

    ActionButton_->setVisible(false);
}

bool ImagePreviewBlock::hasPreview() const
{
    return !Preview_.isNull();
}

bool ImagePreviewBlock::hasRightStatusPadding() const
{
    return true;
}

bool ImagePreviewBlock::isBubbleRequired() const
{
    return false;
}

bool ImagePreviewBlock::isSelected() const
{
    return IsSelected_;
}

void ImagePreviewBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);

    if (isVisible && (PreviewDownloadSeq_ > 0))
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewDownloadSeq_);
    }

    if (isGifPreview())
    {
        onGifVisibilityChanged(isVisible);
    }
}

void ImagePreviewBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType /*selection*/)
{
    const QRect globalWidgetRect(
        mapToGlobal(rect().topLeft()),
        mapToGlobal(rect().bottomRight()));

    auto selectionArea(globalWidgetRect);
    selectionArea.setTop(from.y());
    selectionArea.setBottom(to.y());
    selectionArea = selectionArea.normalized();

    const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);
    assert(selectionOverlap.height() >= 0);

    const auto widgetHeight = std::max(globalWidgetRect.height(), 1);
    const auto overlappedHeight = selectionOverlap.height();
    const auto overlapRatePercents = ((overlappedHeight * 100) / widgetHeight);
    assert(overlapRatePercents >= 0);

    const auto isSelected = (overlapRatePercents > 45);

    if (isSelected != IsSelected_)
    {
        IsSelected_ = isSelected;

        update();
    }
}

void ImagePreviewBlock::setMaxPreviewWidth(int width)
{
    MaxPreviewWidth_ = width;
}

int ImagePreviewBlock::getMaxPreviewWidth() const
{
    return MaxPreviewWidth_;
}

void ImagePreviewBlock::showActionButton(const QRect &pos)
{
    assert(hasActionButton());

    if (!pos.isEmpty())
    {
        assert(pos.size() == ActionButton_->sizeHint());
        ActionButton_->setGeometry(pos);
    }

    const auto isActionButtonVisible = (!isGifPreview() || !isGifPlaying());
    if (isActionButtonVisible)
    {
        ActionButton_->setVisible(true);
    }
}

void ImagePreviewBlock::drawBlock(QPainter &p)
{
    const auto &imageRect = Layout_->getPreviewRect();

    const auto updateClippingPath = (PreviewClippingPathRect_ != imageRect);
    if (updateClippingPath)
    {
        PreviewClippingPathRect_ = imageRect;
        PreviewClippingPath_= evaluateClippingPath(imageRect);
    }

    p.setClipPath(PreviewClippingPath_);

    if (Preview_.isNull())
    {
        drawEmptyBubble(p, imageRect);
    }
    else
    {
        p.drawPixmap(imageRect, Preview_);
    }

    const auto isDownloading = (FullImageDownloadSeq_ != -1);
    if (isDownloading)
    {
        p.fillRect(imageRect, MessageStyle::getImageShadeBrush());
    }

    if (IsSelected_)
    {
        const QBrush brush(Utils::getSelectionColor());
        p.fillRect(imageRect, brush);
    }

    GenericBlock::drawBlock(p);
}

void ImagePreviewBlock::drawEmptyBubble(QPainter &p, const QRect &bubbleRect)
{
    auto bodyBrush = Ui::MessageStyle::getImagePlaceholderBrush();

    p.setBrush(bodyBrush);

    p.drawRoundedRect(
        bubbleRect,
        MessageStyle::getBorderRadius(),
        MessageStyle::getBorderRadius());
}

void ImagePreviewBlock::initialize()
{
    GenericBlock::initialize();

    connectSignals();

    requestSnapMetainfo();

    assert(PreviewDownloadSeq_ == -1);
    PreviewDownloadSeq_ = Ui::GetDispatcher()->downloadImage(
        ImageUri_,
        getChatAimid(),
        QString(),
        true,
        Style::getImageWidthMax(),
        0);
}

void ImagePreviewBlock::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();

    const auto mousePos = event->pos();

    if (isOverImage(mousePos))
    {
        setCursor(Qt::PointingHandCursor);

        return GenericBlock::mouseMoveEvent(event);
    }

    setCursor(Qt::ArrowCursor);

    return GenericBlock::mouseMoveEvent(event);
}

void ImagePreviewBlock::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return;
    }

    if (isOverImage(event->pos()))
    {
        event->accept();
        PressedOverPreview_ = true;
    }

    return GenericBlock::mousePressEvent(event);
}

void ImagePreviewBlock::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return;
    }

    const auto pressedOverPreview = PressedOverPreview_;
    PressedOverPreview_ = false;

    if (!pressedOverPreview)
    {
        return;
    }

    const auto releasedOverImage = isOverImage(event->pos());
    if (!releasedOverImage)
    {
        return;
    }

    if (isFullImageDownloading())
    {
        return;
    }

    event->accept();

    onLeftMouseClick(event->globalPos());

    return GenericBlock::mouseReleaseEvent(event);
}

void ImagePreviewBlock::onMenuCopyFile()
{
    CopyFile_ = true;

    if (isFullImageDownloading())
    {
        return;
    }

    QUrl urlParser(ImageUri_);

    auto filename = urlParser.fileName();
    if (filename.isEmpty())
    {
        static const QRegularExpression re("[{}-]");

        filename = QUuid::createUuid().toString();
        filename.remove(re);
    }

    const auto dstPath = (QDir::temp().absolutePath() + "/" + filename);
    downloadFullImage(dstPath);
}

void ImagePreviewBlock::onMenuCopyLink()
{
    assert(!ImageUri_.isEmpty());

    QApplication::clipboard()->setText(ImageUri_);
}

void ImagePreviewBlock::onMenuSaveFileAs()
{
    assert(!ImageUri_.isEmpty());

    QUrl urlParser(ImageUri_);

    QString dir;
    QString file;
    if (!Utils::saveAs(urlParser.fileName(), Out file, Out dir))
    {
        return;
    }

    assert(!dir.isEmpty());
    assert(!file.isEmpty());

    const auto addTrailingSlash = (!dir.endsWith('\\') && !dir.endsWith('/'));
    if (addTrailingSlash)
    {
        dir += "/";
    }

    dir += file;

    if (isFullImageDownloading())
    {
        SaveAs_ = dir;
        return;
    }

    downloadFullImage(dir);
}

void ImagePreviewBlock::onMenuOpenInBrowser()
{
    QDesktopServices::openUrl(ImageUri_);
}

void ImagePreviewBlock::onRestoreResources()
{
    if (isGifPreview() && isFullImageDownloaded())
    {
        __TRACE(
            "resman",
            "restoring gif fs block\n"
            __LOGP(local, FullImageLocalPath_)
            __LOGP(uri, ImageUri_));

        playGif(FullImageLocalPath_);
    }

    GenericBlock::onRestoreResources();
}

void ImagePreviewBlock::onUnloadResources()
{
    if (GifImage_)
    {
        __TRACE(
            "resman",
            "unloading gif image\n"
            __LOGP(local, FullImageLocalPath_)
            __LOGP(uri, ImageUri_));

        GifImage_.reset();

        IsGifPlaying_ = false;
    }

    GenericBlock::onUnloadResources();
}

void ImagePreviewBlock::showEvent(QShowEvent*)
{
    if (isGifPreview() && isFullImageDownloaded() && !IsPausedByUser_)
    {
        playGif(FullImageLocalPath_);
    }
}

bool ImagePreviewBlock::drag()
{
    return Utils::dragUrl(this, Preview_, DownloadUri_);
}

void ImagePreviewBlock::connectSignals()
{
    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloaded,
        this,
        &ImagePreviewBlock::onImageDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloadingProgress,
        this,
        &ImagePreviewBlock::onImageDownloadingProgress,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageMetaDownloaded,
        this,
        &ImagePreviewBlock::onImageMetaDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloadError,
        this,
        &ImagePreviewBlock::onImageDownloadError,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        GetDispatcher(),
        &core_dispatcher::snapMetainfoDownloaded,
        this,
        &ImagePreviewBlock::onSnapMetainfoDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
}

void ImagePreviewBlock::downloadFullImage(const QString &destination)
{
    assert(FullImageDownloadSeq_ == -1);

    const auto &imageUri = (DownloadUri_.isEmpty() ? ImageUri_ : DownloadUri_);
    assert(!imageUri.isEmpty());

    FullImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(imageUri, getChatAimid(), destination, false, 0, 0);

    if (!shouldDisplayProgressAnimation())
    {
        return;
    }

    if (ActionButton_)
    {
        ActionButton_->startAnimation();
    }

    notifyBlockContentsChanged();
}

QPainterPath ImagePreviewBlock::evaluateClippingPath(const QRect &imageRect) const
{
    assert(!imageRect.isEmpty());

    return Utils::renderMessageBubble(imageRect, Ui::MessageStyle::getBorderRadius(), false);
}

void ImagePreviewBlock::initializeActionButton()
{
    assert(!Preview_.isNull());
    assert(!ImageType_.isEmpty());

    if (ActionButton_)
    {
        return;
    }

    const auto &resourceSet =
        [&]
        {
            if (isGifPreview())
            {
                return ActionButtonWidget::ResourceSet::Gif_;
            }

            if (isVideoPreview())
            {
                return ActionButtonWidget::ResourceSet::Play_;
            }

            return ActionButtonWidget::ResourceSet::DownloadMediaFile_;
        }();

    __TRACE(
        "gif_preview",
        "initialized action button\n"
        "    uri=<" << ImageUri_ << ">\n"
        "    image_type=<" << ImageType_ << ">\n"
        "    is_gif=<" << logutils::yn(isGifPreview()) << ">\n"
        "    is_video=<" << logutils::yn(isVideoPreview()) << ">");

    ActionButton_ = new ActionButtonWidget(resourceSet, this);

    QObject::connect(
        ActionButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            schedulePreviewerOpening(globalClickCoords);

            if (isGifPreview())
            {
                onGifLeftMouseClick();
                return;
            }

            if (isFullImageDownloading())
            {
                return;
            }

            downloadFullImage(QString());
        });

    QObject::connect(
        ActionButton_,
        &ActionButtonWidget::stopClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            Q_UNUSED(globalClickCoords);

            const auto isFullImageDownloading = (FullImageDownloadSeq_ != -1);
            if (isFullImageDownloading)
            {
                GetDispatcher()->cancelImageDownloading(FullImageDownloadSeq_);
                FullImageDownloadSeq_ = -1;
            }

            ActionButton_->stopAnimation();

            notifyBlockContentsChanged();
        });
}

bool ImagePreviewBlock::isFullImageDownloaded() const
{
    return !FullImageLocalPath_.isEmpty();
}

bool ImagePreviewBlock::isFullImageDownloading() const
{
    assert(FullImageDownloadSeq_ >= -1);

    return (FullImageDownloadSeq_ > 0);
}

bool ImagePreviewBlock::isGifPlaying() const
{
    assert(isGifPreview());

    return IsGifPlaying_;
}

bool ImagePreviewBlock::isGifPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    return ((ImageType_ == "gif") || (ImageType_ == "snap/gif"));
}

bool ImagePreviewBlock::isOverImage(const QPoint &pos) const
{
    if (Preview_.isNull())
    {
        return false;
    }

    const auto &previewRect = Layout_->getPreviewRect();

    return  previewRect.contains(pos);
}

bool ImagePreviewBlock::isVideoPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    if (ImageType_ == "snap/video")
    {
        return true;
    }

    return History::IsVideoExtension(ImageType_);
}

void ImagePreviewBlock::onFullImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!image.isNull());
    assert(!localPath.isEmpty());

    FullImageLocalPath_ = localPath;

    stopDownloadingAnimation();

    if (!QFile::exists(localPath))
    {
        assert(!"file is missing");
        return;
    }

    if (!OpenPreviewer_.isNull())
    {
        openPreviewer(image, FullImageLocalPath_);

        OpenPreviewer_ = QPoint();
    }

    if (!SaveAs_.isEmpty())
    {
        QFile::copy(localPath, SaveAs_);
        SaveAs_ = QString();
    }

    if (CopyFile_)
    {
        CopyFile_ = false;
        Utils::copyFileToClipboard(localPath);
    }

    if (isGifPreview())
    {
        playGif(localPath);
    }
}

void ImagePreviewBlock::onGifLeftMouseClick()
{
    assert(isGifPreview());

    IsPausedByUser_ = false;

    if (!isFullImageDownloaded())
    {
        return;
    }

    if (isGifPlaying())
    {
        IsPausedByUser_ = true;

        pauseGif();

        return;
    }

    playGif(FullImageLocalPath_);
}

void ImagePreviewBlock::onGifPlaybackStatusChanged(const bool isPlaying)
{
    if (isPlaying && ActionButton_)
    {
        hideActionButton();
    }

    if (!isPlaying && ActionButton_)
    {
        showActionButton(QRect());
    }
}

void ImagePreviewBlock::onGifVisibilityChanged(const bool isVisible)
{
    assert(isGifPreview());

    if (isVisible)
    {
        if (isFullImageDownloaded() && !IsPausedByUser_)
        {
            playGif(FullImageLocalPath_);
        }

        return;
    }

    if (isGifPlaying())
    {
        pauseGif();
    }
}

void ImagePreviewBlock::onLeftMouseClick(const QPoint &globalPos)
{
    schedulePreviewerOpening(globalPos);

    const auto hasSnapId = (SnapId_ > 0);
    if (hasSnapId)
    {
        GetDispatcher()->read_snap(getSenderAimid(), SnapId_, false);
    }

    if (isGifPreview())
    {
        onGifLeftMouseClick();
        return;
    }

    downloadFullImage(QString());
}

void ImagePreviewBlock::onPreviewImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!image.isNull());
    assert(Preview_.isNull());

    PreviewLocalPath_ = localPath;

    Preview_ = image;

    Utils::check_pixel_ratio(Preview_);

    initializeActionButton();

    notifyBlockContentsChanged();

    preloadFullImageIfNeeded();
}

void ImagePreviewBlock::openPreviewer(QPixmap /*image*/, const QString &localPath)
{
    assert(!localPath.isEmpty());
    assert(!isGifPreview());

    Utils::InterConnector::instance().getMainWindow()->openGallery(AimId_, Data::Image(getId(), ImageUri_, false), localPath);
}

void ImagePreviewBlock::playGif(const QString &localPath)
{
    assert(isGifPreview());
    assert(!localPath.isEmpty());

    const auto isFileExists = QFile::exists(localPath);
    assert(isFileExists);

    if (isGifPlaying() || !isFileExists)
    {
        return;
    }

    if (GifImage_)
    {
        const auto &previewRect = Layout_->getPreviewRect();
        if (previewRect.isEmpty())
        {
            return;
        }

        if (IsPausedByUser_)
        {
            return;
        }

        IsGifPlaying_ = true;

        onGifPlaybackStatusChanged(IsGifPlaying_);

        const auto gifSize = Utils::unscale_value(previewRect.size());
        GifImage_->setScaledSize(gifSize);

        GifImage_->start();

        return;
    }

    std::unique_ptr<Utils::LoadMovieFromFileTask> task(
        new Utils::LoadMovieFromFileTask(localPath));

    QObject::connect(
        task.get(),
        &Utils::LoadMovieFromFileTask::loadedSignal,
        this,
        [this]
        (QSharedPointer<QMovie> movie)
        {
            assert(movie);
            assert(!GifImage_);

            GifImage_ = movie;

            QObject::connect(
                GifImage_.data(),
                &QMovie::frameChanged,
                this,
                &ImagePreviewBlock::onGifFrameUpdated);
        }
    );

    task->run();

    playGif(localPath);
}

void ImagePreviewBlock::preloadFullImageIfNeeded()
{
    if (!isGifPreview())
    {
        return;
    }

    if (isFullImageDownloading())
    {
        return;
    }

    if (isFullImageDownloaded() && !isGifPlaying())
    {
        playGif(FullImageLocalPath_);
        return;
    }

    downloadFullImage(QString());
}

void ImagePreviewBlock::requestSnapMetainfo()
{
    assert(SnapMetainfoRequestId_ == -1);

    const auto fileSharingId = extractIdFromFileSharingUri(ImageUri_);
    if (fileSharingId.isEmpty())
    {
        return;
    }

    const auto fileSharingType = extractContentTypeFromFileSharingId(fileSharingId);

    if (!is_snap_file_sharing_content_type(fileSharingType))
    {
        return;
    }

    SnapMetainfoRequestId_ = GetDispatcher()->download_snap_metainfo(getChatAimid(), fileSharingId);
}

void ImagePreviewBlock::schedulePreviewerOpening(const QPoint &globalPos)
{
    if (isGifPreview())
    {
        assert(OpenPreviewer_.isNull());
        return;
    }

    OpenPreviewer_ = globalPos;
}

bool ImagePreviewBlock::shouldDisplayProgressAnimation() const
{
    assert(FileSize_ >= -1);

    if (FileSize_ < 0)
    {
        return true;
    }

    if (FileSize_ > Style::getMinFileSize4ProgressBar())
    {
        return true;
    }

    return false;
}

void ImagePreviewBlock::stopDownloadingAnimation()
{
    if (isGifPreview() || isVideoPreview())
    {
        assert(ActionButton_);
        ActionButton_->stopAnimation();
    }
    else
    {
        delete ActionButton_;
        ActionButton_ = nullptr;
    }
}

void ImagePreviewBlock::pauseGif()
{
    assert(isGifPreview());

    if (IsGifPlaying_)
    {
        IsGifPlaying_ = false;

        onGifPlaybackStatusChanged(IsGifPlaying_);
    }

    if (!GifImage_)
    {
        return;
    }

    GifImage_->setPaused(true);
}

void ImagePreviewBlock::onGifFrameUpdated(int /*frameNumber*/)
{
    assert(GifImage_);

    const auto frame = GifImage_->currentPixmap();
    Preview_ = frame;

    update();
}

void ImagePreviewBlock::onImageDownloadError(qint64 seq, QString rawUri)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {
        getParentComplexMessage()->replaceBlockWithSourceText(this);

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        FullImageDownloadSeq_ = -1;

        update();

        stopDownloadingAnimation();

        const auto shouldOpenPreviewer = (!OpenPreviewer_.isNull() && !PreviewLocalPath_.isEmpty());
        if (shouldOpenPreviewer)
        {
            openPreviewer(Preview_, PreviewLocalPath_);

            OpenPreviewer_ = QPoint();
        }
    }
}

void ImagePreviewBlock::onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {
        assert(!image.isNull());

        onPreviewImageDownloaded(image, localPath);

        PreviewDownloadSeq_ = -1;

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        assert(!image.isNull());
        assert(!localPath.isEmpty());

        onFullImageDownloaded(image, localPath);

        FullImageDownloadSeq_ = -1;

        update();
    }
}

void ImagePreviewBlock::onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred)
{
    assert(seq > 0);
    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    if (seq != FullImageDownloadSeq_)
    {
        return;
    }

    if (!ActionButton_)
    {
        return;
    }

    const auto normalizedPercents = ((double)pctTransferred / 100.0);

    ActionButton_->setProgress(normalizedPercents);

    const auto progressText = HistoryControl::formatProgressText(bytesTotal, bytesTransferred);
    assert(!progressText.isEmpty());

    ActionButton_->setProgressText(progressText);

    notifyBlockContentsChanged();
}

void ImagePreviewBlock::onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (!isPreviewSeq)
    {
        return;
    }

    DownloadUri_ = meta.getDownloadUri();

    assert(FileSize_ == -1);
    FileSize_ = meta.getFileSize();
}

void ImagePreviewBlock::onSnapMetainfoDownloaded(int64_t _seq, bool _success, uint64_t _snap_id)
{
    assert(_seq > 0);
    assert(!_success || (_snap_id > 0));

    if (SnapMetainfoRequestId_ != _seq)
    {
        return;
    }

    if (_success)
    {
        assert(SnapId_ == 0);
        SnapId_ = _snap_id;
    }

    SnapMetainfoRequestId_ = -1;
}

UI_COMPLEX_MESSAGE_NS_END