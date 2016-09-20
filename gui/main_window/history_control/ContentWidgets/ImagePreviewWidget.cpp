#include "stdafx.h"

#include "../../../core_dispatcher.h"

#include "../../../main_window/MainWindow.h"

#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../../../previewer/GalleryWidget.h"

#include "../../MainPage.h"

#include "ImagePreviewWidget.h"

namespace
{
    int32_t getPreviewHeight();

    QString uri2Filename(const QString &uri);
}

namespace HistoryControl
{

    enum class ImagePreviewWidget::State
    {
        Min,

        Initial,
        Loaded,
        Error,

        Max,
    };

	ImagePreviewWidget::ImagePreviewWidget(QWidget *parent, const bool isOutgoing, const QString &uri, const QString &text, const bool previewsEnabled, QString _aimId)
		: PreviewContentWidget(parent, isOutgoing, text, previewsEnabled, _aimId)
        , State_(State::Initial)
        , Uri_(uri.trimmed())
        , LeftButtonPressed_(false)
        , CopyFile_(false)
        , FullImageDownloadSeq_(-1)
        , PreviewImageDownloadSeq_(-1)
	{
        if (build::is_debug())
        {
            QUrl uri(Uri_);
            assert(uri.isValid());
            assert(!uri.isLocalFile());
            assert(!uri.isRelative());
        }

        setMouseTracking(true);

        connectCoreSignals();
	}

	ImagePreviewWidget::~ImagePreviewWidget()
	{
	}

    void ImagePreviewWidget::leaveEvent(QEvent *e)
    {
        PreviewContentWidget::leaveEvent(e);

        LeftButtonPressed_ = false;
    }

    void ImagePreviewWidget::mouseMoveEvent(QMouseEvent *e)
    {
        PreviewContentWidget::mouseMoveEvent(e);

        setCursor(
            isOverPicture(e->pos()) ?
                Qt::PointingHandCursor :
                Qt::ArrowCursor
        );
    }

    void ImagePreviewWidget::mousePressEvent(QMouseEvent *e)
    {
        PreviewContentWidget::mousePressEvent(e);

        const auto clickedOnPicture = (
            isLeftButtonClick(e) &&
            isOverPicture(e->pos())
        );

        LeftButtonPressed_ |= clickedOnPicture;
    }

    void ImagePreviewWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        PreviewContentWidget::mouseReleaseEvent(e);

        const auto clickedOnPicture = (
            isLeftButtonClick(e) &&
            getPreviewScaledRect().contains(e->pos())
        );

        if (clickedOnPicture && LeftButtonPressed_)
        {
            onLeftMouseClick(e->pos());
        }

        LeftButtonPressed_ = false;
    }

    bool ImagePreviewWidget::drag()
    {
        return Utils::dragUrl(this, getPreview(), Uri_);;
    }

    void ImagePreviewWidget::onImageDownloadError(qint64 seq, QString rawUri)
    {
        assert(seq > 0);
        assert(!rawUri.isEmpty());

        const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
        const auto isPreviewSeq = (seq == PreviewImageDownloadSeq_);

        const auto skipSeq = (!isFullImageSeq && !isPreviewSeq);
        if (skipSeq)
        {
            return;
        }

        __WARN(
            "preview",
            "loading failed\n"
            "    uri=<" << Uri_ << ">\n"
            "    raw_uri=<" << rawUri << ">");

        if (isFullImageSeq)
        {
            FullImageDownloadSeq_ = -1;
            return;
        }

        assert(isPreviewSeq);

        State_ = State::Error;
        emit stateChanged();

        setTextVisible(true);

        update();
    }

    void ImagePreviewWidget::onImageDownloaded(int64_t seq, QString uri, QPixmap image, QString local)
    {
        const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
        const auto isPreviewSeq = (seq == PreviewImageDownloadSeq_);

        const auto skipSeq = (!isFullImageSeq && !isPreviewSeq);
        if (skipSeq)
        {
            return;
        }

        if (isFullImageSeq)
        {
            FullImageDownloadSeq_ = -1;

            onFullImageDownloaded(uri, image, local);

            update();

            return;
        }

        assert(isPreviewSeq);

        PreviewImageDownloadSeq_ = -1;

        onPreviewDownloaded(uri, image, local);

        update();
    }

	bool ImagePreviewWidget::isBlockElement() const
	{
		return false;
	}

	bool ImagePreviewWidget::canUnload() const
	{
		return true;
	}

	QString ImagePreviewWidget::toLogString() const
	{
        QString result;
        result.reserve(512);

        QTextStream fmt(&result);

        fmt << "    widget=<image_preview>\n";

        if (isOutgoing())
        {
            fmt <<  "    type=<outgoing>\n";
        }
        else
        {
            fmt << "    type=<incoming>\n";
        }

        fmt << "    uri=<" << Uri_ << ">\n"
               "    text=<" << getText() << ">";

        return result;
	}

	QString ImagePreviewWidget::toString() const
	{
        return Text_;
	}

    QString ImagePreviewWidget::toLink() const
    {
        return Uri_;
    }

    void ImagePreviewWidget::copyFile()
    {
        const auto isAlreadyDownloaded = !FullFileLocalPath_.isEmpty();
        if (isAlreadyDownloaded)
        {
            Utils::copyFileToClipboard(FullFileLocalPath_);
            return;
        }

        CopyFile_ = true;

        const auto alreadyDownloading = (FullImageDownloadSeq_ != -1);
        if (alreadyDownloading)
        {
            return;
        }

        auto filename = uri2Filename(Uri_);
        if (filename.isEmpty())
        {
            static const QRegularExpression re("[{}-]");

            filename = QUuid::createUuid().toString();
            filename.remove(re);
        }

        const auto dstPath = (QDir::temp().absolutePath() + "/" + filename);

        FullImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(Uri_, getContact(), dstPath, false, 0, 0);
    }

    void ImagePreviewWidget::saveAs()
    {
        const auto filename = uri2Filename(Uri_);

        QString dir;
        QString file;
        if (!Utils::saveAs(filename, Out file, Out dir))
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

        const auto isAlreadyDownloading = (FullImageDownloadSeq_ != -1);
        if (isAlreadyDownloading)
        {
            SaveAs_ = dir;
            return;
        }

        FullImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(Uri_, getContact(), dir, false, 0, 0);
    }

    void ImagePreviewWidget::initialize()
    {
        PreviewContentWidget::initialize();

        if (!PreviewsEnabled_)
        {
            setTextVisible(true);
            return;
        }

        const auto hasText = !Text_.isEmpty();
        const auto uriOnly = (hasText && (Text_.trimmed() == Uri_));
        if (!uriOnly)
        {
            setTextVisible(true);
        }

        PreviewImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(Uri_, getContact(), QString(), true, 0, getPreviewHeight());
    }

    bool ImagePreviewWidget::isPlaceholderVisible() const
    {
        return ((State_ != State::Error) && PreviewsEnabled_);
    }

    bool ImagePreviewWidget::isPreloaderVisible() const
    {
        return false;
    }

    bool ImagePreviewWidget::isLeftButtonClick(QMouseEvent *e)
    {
        assert(e);

        const auto isLeftButtonFlagSet = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (e->button() == Qt::LeftButton));

        return isLeftButton;
    }

    void ImagePreviewWidget::connectCoreSignals()
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloaded,
            this,
            &ImagePreviewWidget::onImageDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloadError,
            this,
            &ImagePreviewWidget::onImageDownloadError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
    }

    bool ImagePreviewWidget::isOverPicture(const QPoint &p) const
    {
        return getPreviewScaledRect().contains(p);
    }

    void ImagePreviewWidget::onFullImageDownloaded(QString uri, QPixmap image, QString local)
    {
        assert(!uri.isEmpty());
        assert(!image.isNull());
        assert(!local.isEmpty());

        assert(FullFileLocalPath_.isEmpty());
        FullFileLocalPath_ = local;

        if (CopyFile_)
        {
            CopyFile_ = false;
            copyFile();
        }

        if (!SaveAs_.isEmpty())
        {
            QFile::copy(local, SaveAs_);
            SaveAs_ = QString();
        }

        if (!OpenPreviewer_.isNull())
        {
            openPreviewer(OpenPreviewer_);
            OpenPreviewer_ = QPoint();
        }
    }

    void ImagePreviewWidget::onLeftMouseClick(const QPoint &click)
    {
        if (!FullFileLocalPath_.isEmpty())
        {
            openPreviewer(click);
            OpenPreviewer_ = QPoint();
            return;
        }

        OpenPreviewer_ = click;

        const auto downloadInProgress = (FullImageDownloadSeq_ != -1);
        if (downloadInProgress)
        {
            return;
        }

        const auto procId = Ui::GetDispatcher()->downloadImage(Uri_, getContact(), QString(), false, 0, 0);
        FullImageDownloadSeq_ = procId;
    }

    void ImagePreviewWidget::onPreviewDownloaded(QString uri, QPixmap image, QString local)
    {
        assert(!uri.isEmpty());
        assert(!image.isNull());

        if (image.isNull())
        {
            return;
        }

        State_ = State::Loaded;
        setPreview(image);
        emit stateChanged();
    }

    void ImagePreviewWidget::openPreviewer(const QPoint &click)
    {
        assert(!click.isNull());

        Ui::MainPage::instance()->cancelSelection();

        if (!QFile::exists(FullFileLocalPath_))
        {
            assert(!"the file not exists");
            return;
        }

        Utils::InterConnector::instance().getMainWindow()->openGallery(aimId_, Data::Image(0, Uri_, false), FullFileLocalPath_);
    }

    bool ImagePreviewWidget::isPreviewVisible() const
    {
        return (State_ == State::Loaded);
    }

    bool ImagePreviewWidget::isImageBubbleVisible() const
    {
        if (!PreviewsEnabled_)
        {
            return false;
        }
        return (State_ != State::Error);
    }

    void ImagePreviewWidget::onVisibilityChanged(const bool isVisible)
    {
        PreviewContentWidget::onVisibilityChanged(isVisible);

        const auto hasPreviewSeq = (PreviewImageDownloadSeq_ > 0);
        if (hasPreviewSeq)
        {
            Ui::GetDispatcher()->raiseDownloadPriority(PreviewImageDownloadSeq_);
        }
    }
}

namespace
{
    int32_t getPreviewHeight()
    {
        return Utils::scale_value(240);
    }

    QString uri2Filename(const QString &uri)
    {
        assert(!uri.isEmpty());

        const auto slashPos = uri.lastIndexOf("/");
        const auto hasSlash = (slashPos != -1);
        if (hasSlash)
        {
            return uri.mid(slashPos + 1, uri.length());
        }

        return QString();
    }
}