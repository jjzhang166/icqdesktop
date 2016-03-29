#include "stdafx.h"

#include "../../core_dispatcher.h"

#include "../../utils/log/log.h"
#include "../../utils/utils.h"

#include "ImagePreviewWidget.h"

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
	{
        if (build::is_debug())
        {
            QUrl uri(Uri_);
            assert(uri.isValid());
            assert(!uri.isLocalFile());
            assert(!uri.isRelative());
        }

        setMouseTracking(true);
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
            PreviewGenuineSize_.isValid() &&
            getPreviewScaledRect().contains(e->pos())
        );

        if (clickedOnPicture && LeftButtonPressed_)
        {
            QDesktopServices::openUrl(Uri_);
        }

        LeftButtonPressed_ = false;
    }

    void ImagePreviewWidget::imageDownloaded(qint64 seq, QString uri, QPixmap preview, QString local)
    {
        if (!isCurrentProcessId(seq))
        {
            return;
        }

        connectCoreSignals(false);
        resetCurrentProcessId();

        if (!preview.isNull())
        {
            State_ = State::Loaded;
            setPreview(preview);
            emit stateChanged();
        }
        else
        {
            __WARN(
                "preview",
                "loading failed\n"
                "    uri=<" << Uri_ << ">"
            );

            State_ = State::Error;
            emit stateChanged();

            setTextVisible(true);
        }

        Local_ = local;

        if (CopyFile_)
        {
            CopyFile_ = false;
            copyFile();
        }

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
        int pos = Uri_.lastIndexOf('.');
        QString ext;
        if (pos != -1)
            ext = Uri_.mid(pos, Uri_.length());

        if (!Local_.isEmpty() && Local_.endsWith(ext))
        {
            Utils::copyFileToClipboard(Local_);
        }
        else
        {
            CopyFile_ = true;
            connectCoreSignals(true);
            setCurrentProcessId(Ui::GetDispatcher()->downloadImagePreview(Uri_, QDir::temp().absolutePath() + "/" + QUuid::createUuid().toString() + ext));
        }
    }

    void ImagePreviewWidget::saveAs()
    {
        int slash = Uri_.lastIndexOf("/");
        QString filename = slash ? Uri_.mid(slash + 1, Uri_.length()) : QString();
        QString dir, file;
        if (Utils::saveAs(filename, file, dir))
        {
            if (!dir.endsWith('\\') && !dir.endsWith('/'))
                dir += "/";
            dir += file;

            connectCoreSignals(true);
            setCurrentProcessId(Ui::GetDispatcher()->downloadImagePreview(Uri_, dir));
        }
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

        connectCoreSignals(true);

        const auto procId = Ui::GetDispatcher()->downloadImagePreview(Uri_);

        setCurrentProcessId(procId);
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
            (e->button() == Qt::LeftButton)
        );

        return isLeftButton;
    }

    void ImagePreviewWidget::connectCoreSignals(const bool isConnected)
    {
        connectCoreSignal(
            SIGNAL(imageDownloaded(qint64, QString, QPixmap, QString)),
            SLOT(imageDownloaded(qint64, QString, QPixmap, QString)),
            isConnected
        );
    }

    bool ImagePreviewWidget::isOverPicture(const QPoint &p) const
    {
        if (!PreviewGenuineSize_.isValid())
        {
            return false;
        }

        return getPreviewScaledRect().contains(p);
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
}