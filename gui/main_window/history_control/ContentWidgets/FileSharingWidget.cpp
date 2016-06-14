#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"
#include "../../../utils/log/log.h"
#include "../../../utils/profiling/auto_stop_watch.h"
#include "../../../utils/InterConnector.h"
#include "../../../gui_settings.h"
#include "../../../core_dispatcher.h"
#include "../../../themes/ThemePixmap.h"
#include "../../../themes/ResourceIds.h"
#include "../../../utils/LoadPixmapFromFileTask.h"
#include "../../../previewer/Previewer.h"

#include "../../MainPage.h"
#include "../HistoryControlPage.h"

#include "../KnownFileTypes.h"
#include "../FileSharingInfo.h"
#include "../FileSizeFormatter.h"
#include "FileSharingWidget.h"

#ifdef __APPLE__
#include "mac_support.h"
#endif

namespace
{
	const Themes::IThemePixmap& downloadButton();

	const Themes::IThemePixmap& downloadCancelButton();

	const QSizeF& getMaxPreviewSize();

	const QSizeF& getMinPreviewSize();

	const auto DOWNLOADING_BAR_BASE_ANGLE_MIN = 90;

	const auto DOWNLOADING_BAR_BASE_ANGLE_MAX = 450;

	const auto PROGRESS_PEN_WIDTH = 1;

    const auto RETRY_MAX = 5;

    const auto RETRY_INTERVAL_MS = 500;
}

namespace HistoryControl
{

	enum class FileSharingWidget::State
	{
		Min,

		Initial,

		PlainFile_Initial,
		PlainFile_MetainfoLoaded,
		PlainFile_CheckingLocalCopy,
		PlainFile_Downloading,
		PlainFile_Downloaded,
		PlainFile_Uploading,
		PlainFile_UploadError,
		PlainFile_Uploaded,

        ImageFile_Min,

		ImageFile_Initial,
		ImageFile_MetainfoLoaded,
		ImageFile_Downloading,
		ImageFile_Downloaded,
		ImageFile_Uploading,
		ImageFile_Uploaded,

        ImageFile_Max,

		Deleted,

		Max
	};

    enum class FileSharingWidget::PreviewState
    {
        Min,

        NoPreview,
        MiniPreviewLoaded,
        FullPreviewLoaded,

        Max
    };

    FileSharingWidget::Retry::Retry()
        : FileMetainfo_(false)
        , FileMetainfoRetryCount_(0)
        , FileDownload_(false)
        , FileDownloadRetryCount_(0)
        , PreviewDownload_(false)
        , PreviewDownloadRetryCount_(0)
    {
    }

    bool FileSharingWidget::Retry::HasRetryFlagSet() const
    {
        return (FileMetainfo_ || FileDownload_ || PreviewDownload_);
    }

    bool FileSharingWidget::Retry::ShouldRetry() const
    {
        return (ShouldRetryFileMetainfo() || ShouldRetryFileDownload() || ShouldRetryPreviewDownload());
    }

    bool FileSharingWidget::Retry::ShouldRetryFileDownload() const
    {
        assert(FileDownloadRetryCount_ >= 0);
        assert(FileDownloadRetryCount_ <= RETRY_MAX);

        return (FileDownload_ && (FileDownloadRetryCount_ < RETRY_MAX));
    }

    bool FileSharingWidget::Retry::ShouldRetryFileMetainfo() const
    {
        assert(FileMetainfoRetryCount_ >= 0);
        assert(FileMetainfoRetryCount_ <= RETRY_MAX);

        return (FileMetainfo_ && (FileMetainfoRetryCount_ < RETRY_MAX));
    }

    bool FileSharingWidget::Retry::ShouldRetryPreviewDownload() const
    {
        assert(PreviewDownloadRetryCount_ >= 0);
        assert(PreviewDownloadRetryCount_ <= RETRY_MAX);

        return (PreviewDownload_ && (PreviewDownloadRetryCount_ < RETRY_MAX));
    }

	void FileSharingWidget::setDownloadingBarBaseAngle(int _val)
	{
		assert(_val >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(_val <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		DownloadingBarBaseAngle_ = _val;

		update();
	}

	int FileSharingWidget::getDownloadingBarBaseAngle() const
	{
		assert(DownloadingBarBaseAngle_ >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(DownloadingBarBaseAngle_ <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		return DownloadingBarBaseAngle_;
	}

    FileSharingWidget::FileSharingWidget(const FileSharingInfoSptr& fsInfo, QString _aimId)
        : PreviewContentWidget(0, false, QString(), false, _aimId)
        , FsInfo_(fsInfo)
        , CopyFile_(false)
        , SaveAs_(false)
        , PreviewState_(PreviewState::NoPreview)
        , PreviewDownloadId_(-1)
        , FileMetainfoDownloadId_(-1)
        , PreviewMetainfoDownloadId_(-1)
        , FileDownloadId_(-1)
        , CheckLocalCopyExistenceId_(-1)
    {
    }

	FileSharingWidget::FileSharingWidget(
        QWidget *parent,
        const bool isOutgoing,
        const QString &contactUin,
        const FileSharingInfoSptr& fsInfo,
        const bool previewsEnabled)
		: PreviewContentWidget(parent, isOutgoing, QString(), previewsEnabled, contactUin)
		, ContactUin_(contactUin)
		, FsInfo_(fsInfo)
		, BytesTransferred_(0)
		, DownloadingBarBaseAngle_(DOWNLOADING_BAR_BASE_ANGLE_MIN)
		, BaseAngleAnimation_(nullptr)
        , CopyFile_(false)
        , SaveAs_(false)
        , PreviewState_(PreviewState::NoPreview)
        , PreviewDownloadId_(-1)
        , FileMetainfoDownloadId_(-1)
        , PreviewMetainfoDownloadId_(-1)
        , FileDownloadId_(-1)
        , CheckLocalCopyExistenceId_(-1)
	{
		assert(!ContactUin_.isEmpty());
		assert(FsInfo_);
		assert(isOutgoing == FsInfo_->IsOutgoing());

		Private_.State_ = State::Initial;

		Metainfo_.FileSize_ = 0;

        if (FsInfo_->HasSize())
        {
		    setPreviewGenuineSize(FsInfo_->GetSize());
        }
	}

	FileSharingWidget::~FileSharingWidget()
	{
	}

	void FileSharingWidget::initializeInternal()
	{
        PreviewContentWidget::initializeInternal();

		__TRACE(
            "fs",
            "initializing file sharing widget\n" <<
            FsInfo_->ToLogString()
        );

		setMouseTracking(true);

		setInitialWidgetSizeAndState();

        connectSignals();

        const auto isImageFileInitial = isState(State::ImageFile_Initial);
        const auto isPlainFileInitial = isState(State::PlainFile_Initial);
        assert(isImageFileInitial || isPlainFileInitial);

		if (FsInfo_->IsOutgoing())
		{
			const auto &uploadingProcessId = FsInfo_->GetUploadingProcessId();
			if (!uploadingProcessId.isEmpty())
			{
				assert(FsInfo_->IsOutgoing());
				assert(FsInfo_->HasLocalPath());

				resumeUploading();

				return;
			}
		}

        if (isImageFileInitial)
        {
            requestPreviewMetainfo();
        }

        requestFileMetainfo();

        invalidateSizes();
        update();
	}

	QString FileSharingWidget::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		fmt << "    widget=<file_sharing>\n";

		if (FsInfo_->IsOutgoing())
		{
			fmt << "    type=<outgoing>\n"
				   "    local_path=<" << FsInfo_->GetLocalPath() << ">";
		}
		else
		{
			fmt << "    type=<incoming>\n"
				   "    uri=<" << FsInfo_->GetUri() << ">";
		}

		return result;
	}

    QString FileSharingWidget::toRecentsString() const
    {
        return QT_TRANSLATE_NOOP("filesharing_widget", "File");
    }

	QString FileSharingWidget::toString() const
	{
		QString result;
		result.reserve(512);

		result = FsInfo_->GetUri();
		if (result.isEmpty())
        {
			result = FsInfo_->GetLocalPath();
        }

		return result;
	}

    QString FileSharingWidget::toLink() const
    {
        QString result;
        result.reserve(512);

        result = FsInfo_->GetUri();
        if (result.isEmpty())
        {
            result = FsInfo_->GetLocalPath();
        }

        return result;
    }

    void FileSharingWidget::copyFile()
    {
        if (DownloadedFileLocalPath_.isEmpty())
        {
            CopyFile_ = true;
            if (isImagePreview())
            {
                startDownloadingFullImage();
                return;
            }

            startDownloadingPlainFile();

            return;
        }

        Utils::copyFileToClipboard(DownloadedFileLocalPath_);
    }

    void FileSharingWidget::saveAs()
    {
        QString dir;
        QString file;
        if (!Utils::saveAs(Metainfo_.Filename_, Out file, Out dir))
        {
            return;
        }

        if (isImagePreview())
        {
            setState(State::ImageFile_Downloading);
        }
        else
        {
            setState(State::PlainFile_Downloading);
        }

        startDataTransferAnimation();

        const auto procId = Ui::GetDispatcher()->downloadSharedFile(
            ContactUin_,
            FsInfo_->GetUri(),
            dir,
            file,
            core::file_sharing_function::download_file);

        assert(FileDownloadId_ == -1);
        FileDownloadId_ = procId;

        SaveAs_ = true;
    }

    bool FileSharingWidget::haveContentMenu(QPoint) const
    {
        return true;
    }

	bool FileSharingWidget::canStartImageDownloading(const QPoint &mousePos) const
	{
        const auto isInRightState = isState(State::ImageFile_MetainfoLoaded);

		return (isInRightState && isOverPreview(mousePos));
	}

	void FileSharingWidget::checkLocalCopyExistence()
	{
		assert(isState(State::PlainFile_CheckingLocalCopy));

		const auto procId = (int)Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()),
            QString(),
			core::file_sharing_function::check_local_copy_exists);

        assert(CheckLocalCopyExistenceId_ == -1);
        CheckLocalCopyExistenceId_ = procId;
	}

    void FileSharingWidget::connectErrorSignal()
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingError,
            this,
            &FileSharingWidget::fileSharingError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
    }

	void FileSharingWidget::connectFileDownloadSignals()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileDownloaded,
            this,
            &FileSharingWidget::fileDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileDownloading,
            this,
            &FileSharingWidget::fileDownloading,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingLocalCopyCheckCompleted,
            this,
            &FileSharingWidget::fileLocalCopyChecked,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

	void FileSharingWidget::connectMetainfoSignal()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded,
            this,
            &FileSharingWidget::onFileMetainfo,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingPreviewMetainfoDownloaded,
            this,
            &FileSharingWidget::onPreviewMetainfo,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

	void FileSharingWidget::connectFileUploadingSignals()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingResult,
            this,
            &FileSharingWidget::fileSharingUploadingResult,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingProgress,
            this,
            &FileSharingWidget::fileSharingUploadingProgress,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

    void FileSharingWidget::connectSignals()
    {
        connectErrorSignal();

        connectFileDownloadSignals();

        connectFileUploadingSignals();

        connectMetainfoSignal();

        connectPreviewSignals();
    }

	void FileSharingWidget::convertToPlainFileView()
	{
		setState(State::PlainFile_MetainfoLoaded);
	}

	void FileSharingWidget::convertToUploadErrorView()
	{
		setState(State::PlainFile_UploadError);

		Metainfo_.Filename_ = "not found";
		Metainfo_.FileSize_ = 0;
		Metainfo_.FileSizeStr_.resize(0);

        update();
	}

    void FileSharingWidget::connectPreviewSignals()
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloaded,
            this,
            &FileSharingWidget::previewDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloadError,
            this,
            &FileSharingWidget::previewDownloadError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
    }

	bool FileSharingWidget::isBlockElement() const
	{
		return (isState(State::PlainFile_Initial) ||
				isState(State::PlainFile_MetainfoLoaded) ||
				isState(State::PlainFile_CheckingLocalCopy) ||
				isState(State::PlainFile_Downloading) ||
				isState(State::PlainFile_Downloaded) ||
				isState(State::PlainFile_Uploading) ||
				isState(State::PlainFile_Uploaded) ||
				isState(State::PlainFile_UploadError));
	}

    bool FileSharingWidget::canReplace() const
    {
        return false;
    }

    bool FileSharingWidget::canUnload() const
	{
		return (!isState(State::PlainFile_Initial) &&
				!isState(State::PlainFile_CheckingLocalCopy) &&
				!isState(State::PlainFile_Downloading) &&
				!isState(State::PlainFile_Uploading) &&
				!isState(State::ImageFile_Initial) &&
				!isState(State::ImageFile_Downloading));
	}

    bool FileSharingWidget::isPreloaderVisible() const
    {
        return !isDataTransferProgressVisible();
    }

	void FileSharingWidget::mouseMoveEvent(QMouseEvent *event)
	{
        const auto mousePos = event->pos();

		if (isControlButtonVisible())
		{
			if (isOverControlButton(mousePos) &&
				!isState(State::PlainFile_Downloaded) &&
				!isState(State::PlainFile_Uploaded) &&
				!isState(State::ImageFile_Uploaded))
			{
				setCursor(Qt::PointingHandCursor);
				return;
			}
		}

		if (isOpenDownloadsDirButtonVisible())
		{
			if (isOverOpenDownloadsDirButton(mousePos))
			{
				setCursor(Qt::PointingHandCursor);
				return;
			}
		}

        const auto isClickable = (
            canStartImageDownloading(mousePos) ||
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded)
        );
		if (isClickable)
		{
			setCursor(Qt::PointingHandCursor);
			return;
		}

		setCursor(Qt::ArrowCursor);
		event->ignore();
	}

	void FileSharingWidget::mouseReleaseEvent(QMouseEvent *event)
	{
        event->ignore();

        Ui::HistoryControlPage* page = Utils::InterConnector::instance().getHistoryPage(ContactUin_);
        if (page && page->touchScrollInProgress())
        {
            return;
        }

		const auto isLeftClick = (event->button() == Qt::LeftButton);
		if (!isLeftClick)
		{
			return;
		}

		const auto mousePos = event->pos();

        const auto isOpenDownloadsDirButtonClicked = (
            isOpenDownloadsDirButtonVisible() &&
            isOverOpenDownloadsDirButton(mousePos));
        if (isOpenDownloadsDirButtonClicked)
        {
            openDownloadsDir();
            return;
        }

		const auto clickedOnButton = (isControlButtonVisible() && isOverControlButton(mousePos));

        const auto isDownloading = (isState(State::PlainFile_Downloading) || isState(State::ImageFile_Downloading));
		const auto isStopDownloadButtonClicked = (clickedOnButton && isDownloading);
		if (isStopDownloadButtonClicked)
		{
			stopDownloading();
			return;
		}

        const auto isUploading = (isState(State::PlainFile_Uploading) || isState(State::ImageFile_Uploading));
		const auto isStopUploadButtonClicked = (clickedOnButton && isUploading);
		if (isStopUploadButtonClicked)
		{
			stopUploading();
			return;
		}

        if (isState(State::PlainFile_MetainfoLoaded))
        {
            startDownloadingPlainFile();
            return;
        }

        const auto fullImageReady = (
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded)
        );
		if (fullImageReady && isOverPreview(mousePos))
		{
            if (platform::is_apple())
            {
                auto path = DownloadedFileLocalPath_;
                if (path.isEmpty() && FsInfo_.get())
                {
                    path = FsInfo_->GetLocalPath();
                }
                assert(QFile::exists(path));
                openPreviewMac(path, event->globalX(), event->globalY());
            }
            else
            {
                openPreview();
            }

            return;
		}

        if (canStartImageDownloading(mousePos))
        {
            setState(State::ImageFile_Downloading);

            startDownloadingFullImage();
        }
	}

	const QRect& FileSharingWidget::getControlButtonRect() const
	{
		if (isState(State::PlainFile_MetainfoLoaded) ||
			isState(State::PlainFile_Downloading) ||
			isState(State::PlainFile_Downloaded) ||
			isState(State::PlainFile_Uploading) ||
			isState(State::PlainFile_Uploaded))
		{
			return getControlButtonPlainRect();
		}

		return getControlButtonPreviewRect();
	}

	const QRect& FileSharingWidget::getControlButtonPlainRect() const
	{
		if (Private_.ControlButtonPlainRect_.isEmpty())
		{
			auto x = Utils::scale_value(PROGRESS_PEN_WIDTH);
			auto y = Utils::scale_value(8);

			const auto &btn = downloadButton();

			Private_.ControlButtonPlainRect_ = QRect(x, y, btn.GetWidth(), btn.GetHeight());
		}

		return Private_.ControlButtonPlainRect_;
	}

	const QRect& FileSharingWidget::getControlButtonPreviewRect() const
	{
		assert(isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Uploading));

		if (Private_.ControlButtonPreviewRect_.isEmpty())
		{
			const auto &btn = downloadCancelButton();

			auto buttonRect = btn.GetRect();

			buttonRect.moveCenter(getPreviewScaledRect().center());

			Private_.ControlButtonPreviewRect_ = buttonRect;
		}

		return Private_.ControlButtonPreviewRect_;
	}

	bool FileSharingWidget::getLocalFileMetainfo()
	{
		assert(FsInfo_);

		const auto &localPath = FsInfo_->GetLocalPath();
		QFileInfo info(localPath);

		if (!info.exists())
		{
			return false;
		}

		assert(Metainfo_.Filename_.isEmpty());
		assert(Metainfo_.FileSize_ == 0);
		assert(Metainfo_.FileSizeStr_.isEmpty());

		Metainfo_.Filename_ = info.fileName();
		Metainfo_.FileSize_ = info.size();
		Metainfo_.FileSizeStr_ = FormatFileSize(Metainfo_.FileSize_);

		return true;
	}

	FileSharingWidget::State FileSharingWidget::getState() const
	{
		assert(Private_.State_ > State::Min);
		assert(Private_.State_ < State::Max);

		return Private_.State_;
	}

	bool FileSharingWidget::isControlButtonVisible() const
	{
		return isState(State::PlainFile_MetainfoLoaded) ||
			   isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Downloaded) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::PlainFile_Uploaded) ||
			   isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Uploading);
	}

	bool FileSharingWidget::isDataTransferProgressVisible() const
	{
		return isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Uploading);
	}

    bool FileSharingWidget::isImagePreview() const
    {
        return (
            (getState() > State::ImageFile_Min) &&
            (getState() < State::ImageFile_Max)
        );
    }

	bool FileSharingWidget::isFilenameAndSizeVisible() const
	{
		return isState(State::PlainFile_MetainfoLoaded) ||
			   isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Downloaded) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::PlainFile_Uploaded) ||
			   isState(State::PlainFile_CheckingLocalCopy);
	}

	bool FileSharingWidget::isOpenDownloadsDirButtonVisible() const
	{
		return isState(State::PlainFile_Downloaded);
	}

	bool FileSharingWidget::isOverControlButton(const QPoint &p) const
	{
		auto downloadButtonRect = getControlButtonRect();
		const auto margins = contentsMargins();
		downloadButtonRect.translate(margins.left(), margins.top());

		return downloadButtonRect.contains(p);
	}

	bool FileSharingWidget::isOverOpenDownloadsDirButton(const QPoint &p) const
	{
		assert(isOpenDownloadsDirButtonVisible());

		const auto margins = contentsMargins();
		auto controlRect = OpenDownloadsDirButtonRect_;
		controlRect.translate(margins.left(), margins.top());

		return controlRect.contains(p);
	}

    bool FileSharingWidget::isOverPreview(const QPoint &p) const
    {
        return getPreviewScaledRect().contains(p);
    }

	bool FileSharingWidget::isPreviewVisible() const
	{
        assert(PreviewState_ > PreviewState::Min);
        assert(PreviewState_ < PreviewState::Max);

		return (
            (PreviewState_ == PreviewState::MiniPreviewLoaded) ||
            (PreviewState_ == PreviewState::FullPreviewLoaded)
        );
	}

	bool FileSharingWidget::isState(const State state) const
	{
		return (Private_.State_ == state);
	}

	bool FileSharingWidget::loadPreviewFromLocalFile()
	{
		assert(isState(State::ImageFile_Initial));

		const auto fileExists = QFile::exists(FsInfo_->GetLocalPath());
        if (!fileExists)
        {
            return false;
        }

        auto task = new Utils::LoadPixmapFromFileTask(FsInfo_->GetLocalPath());

        QObject::connect(
            task,
            &Utils::LoadPixmapFromFileTask::loadedSignal,
            this,
            &FileSharingWidget::localPreviewLoaded);

        QThreadPool::globalInstance()->start(task);

		return true;
	}

	void FileSharingWidget::renderPreview(QPainter &p)
	{
        if (isImagePreview())
        {
            PreviewContentWidget::renderPreview(p);
        }

        if (isFilenameAndSizeVisible())
		{
			renderFilename(p);
			renderFileSizeAndProgress(p);
		}

		if (isControlButtonVisible())
		{
			renderControlButton(p);
		}

		if (isDataTransferProgressVisible())
		{
			renderDataTransferProgress(p);
		}

		if (isOpenDownloadsDirButtonVisible())
		{
			renderOpenDownloadsDirButton(p);
		}
	}

    void FileSharingWidget::resizeEvent(QResizeEvent *e)
    {
        PreviewContentWidget::resizeEvent(e);
    }

	void FileSharingWidget::openDownloadsDir() const
	{
#ifdef _WIN32
		assert(QFile::exists(DownloadedFileLocalPath_));

		const auto param = ("/select," + QDir::toNativeSeparators(DownloadedFileLocalPath_));
		const auto command = "explorer " + param;
		QProcess::startDetached(command);
#else

#ifdef __APPLE__
        MacSupport::openFinder(DownloadedFileLocalPath_);
#else
        QDir dir(DownloadedFileLocalPath_);
        dir.cdUp();
        QDesktopServices::openUrl(dir.absolutePath());
#endif

#endif //_WIN32
	}

    void FileSharingWidget::openPreviewMac(const QString &path, int x, int y)
    {
        Q_UNUSED(x);
        Q_UNUSED(y);

        assert(QFile::exists(path));

#ifdef __APPLE__
        MacSupport::showPreview(path, x, y);
#endif
    }

	void FileSharingWidget::openPreview()
	{
        Ui::MainPage::instance()->cancelSelection();

        if (Preview_.FullImg_.isNull())
        {
            Utils::loadPixmap(DownloadedFileLocalPath_, Out Preview_.FullImg_);
        }

        if (!Preview_.FullImg_.isNull())
		{
			Previewer::ShowPreview(Preview_.FullImg_);
		}
	}

	void FileSharingWidget::renderControlButton(QPainter &p)
	{
		assert(isControlButtonVisible());

		if (isState(State::PlainFile_Downloading) ||
			isState(State::PlainFile_Uploading) ||
			isState(State::ImageFile_Downloading) ||
			isState(State::ImageFile_Uploading))
		{
			downloadCancelButton().Draw(p, getControlButtonRect());
			return;
		}

		if (isState(State::PlainFile_Downloaded) ||
			isState(State::PlainFile_Uploaded))
		{
			if (!FileTypeIcon_)
			{
				FileTypeIcon_ = GetIconByFilename(Metainfo_.Filename_);
			}

			FileTypeIcon_->Draw(p, getControlButtonRect());

			return;
		}

		downloadButton().Draw(p, getControlButtonRect());
	}

	void FileSharingWidget::renderDataTransferProgress(QPainter &p)
	{
		assert(Metainfo_.FileSize_ >= 0);
		assert(BytesTransferred_ >= 0);
		assert(BytesTransferred_ <= Metainfo_.FileSize_);
		assert(DownloadingBarBaseAngle_ >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(DownloadingBarBaseAngle_ <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		p.save();

		auto percents = 1.0;
		if (Metainfo_.FileSize_ > 0)
		{
			percents = std::max(0.05, (double)BytesTransferred_ / (double)Metainfo_.FileSize_);
		}
		assert(percents <= (1.0 + std::numeric_limits<double>::epsilon()));

		const auto angle = (int)std::ceil(percents * 360 * 16);
		assert(angle <= (361 * 16));

		const auto penWidth = Utils::scale_value(PROGRESS_PEN_WIDTH);
		QPen pen(QBrush(Qt::black), penWidth);
		p.setPen(pen);

		const auto baseAngle = (DownloadingBarBaseAngle_ * 16);
		p.drawArc(getControlButtonRect(), -baseAngle, -angle);

		p.restore();
	}

	void FileSharingWidget::renderFilename(QPainter &p)
	{
		assert(!Metainfo_.Filename_.isEmpty());

		p.save();

        QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16));

		p.setFont(font);
		p.setPen(QColor(0x282828));

		auto x = downloadButton().GetWidth();
		x += Utils::scale_value(12);

		auto y = Utils::scale_value(29);

		p.drawText(x, y, Metainfo_.Filename_);

		p.restore();
	}

	void FileSharingWidget::renderFileSizeAndProgress(QPainter &p)
	{
		assert(Metainfo_.FileSize_ >= 0);

		if (Metainfo_.FileSizeStr_.isEmpty())
		{
			Metainfo_.FileSizeStr_ = FormatFileSize(Metainfo_.FileSize_);
		}

		p.save();

        QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(12));

		p.setFont(font);
		p.setPen(QColor(0x979797));

		auto x = downloadButton().GetWidth();
		x += Utils::scale_value(12);

		auto y = Utils::scale_value(47);

		QString text;
		if (isState(State::PlainFile_Downloading) || isState(State::PlainFile_Uploading))
		{
			text = FormatFileSize(BytesTransferred_);
			text += " of ";
		}

		text += Metainfo_.FileSizeStr_;

		p.drawText(x, y, text);

		FileSizeAndProgressStr_ = text;

		p.restore();
	}

	void FileSharingWidget::renderOpenDownloadsDirButton(QPainter &p)
	{
		assert(!FileSizeAndProgressStr_.isEmpty());

		p.save();

		QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(12));

		QFontMetrics m(font);
		const auto fileSizeAndProgressWidth = m.tightBoundingRect(FileSizeAndProgressStr_).width();

		auto x = downloadButton().GetWidth();
		x += Utils::scale_value(12);
		x += fileSizeAndProgressWidth;
		x += Utils::scale_value(16);

		auto y = Utils::scale_value(47);

        static const QString TEXT = QT_TRANSLATE_NOOP("filesharing_widget","Show in folder");

		auto controlRect = m.boundingRect(TEXT);
		controlRect.translate(x, y);
        controlRect.setWidth(controlRect.width() + Utils::scale_value(2));
		OpenDownloadsDirButtonRect_ = controlRect;

		p.setFont(font);
		p.setPen(QPen(0x579e1c));
		p.drawText(controlRect, TEXT);

		p.restore();
	}

	void FileSharingWidget::requestFileMetainfo()
	{
		assert(FsInfo_);
		assert(isState(State::PlainFile_Initial) || isState(State::ImageFile_Initial));

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()),
            QString(),
			core::file_sharing_function::download_file_metainfo);

        assert(FileMetainfoDownloadId_ == -1);
        FileMetainfoDownloadId_ = procId;
	}

	void FileSharingWidget::requestPreview()
	{
        if (PreviewState_ == PreviewState::FullPreviewLoaded)
        {
            assert(!"invalid preview state");
            return;
        }

		const auto &previewUri = (
            (PreviewState_ == PreviewState::NoPreview) ?
                Metainfo_.MiniPreviewUri_ :
                Metainfo_.FullPreviewUri_
        );

		if (previewUri.isEmpty())
		{
            assert(!"unexpected preview uri");
			return;
		}

        assert(PreviewDownloadId_ == -1);
		PreviewDownloadId_ = Ui::GetDispatcher()->downloadImage(previewUri, QString(), false);
	}

    void FileSharingWidget::requestPreviewMetainfo()
    {
        assert(FsInfo_);
        assert(isState(State::ImageFile_Initial));

        const auto procId = Ui::GetDispatcher()->downloadSharedFile(
            ContactUin_,
            FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()),
            QString(),
            core::file_sharing_function::download_preview_metainfo);

        assert(PreviewMetainfoDownloadId_ == -1);
        PreviewMetainfoDownloadId_ = procId;
    }

	void FileSharingWidget::resumeUploading()
	{
		assert(FsInfo_);
		assert(FsInfo_->IsOutgoing());
		assert(isState(State::PlainFile_Initial) || isState(State::ImageFile_Initial));

		if (!getLocalFileMetainfo())
		{
			convertToUploadErrorView();
			return;
		}

		if (isState(State::PlainFile_Initial))
		{
			setState(State::PlainFile_Uploading);

            startDataTransferAnimation();
		}
		else
		{
			if (!loadPreviewFromLocalFile())
            {
                convertToUploadErrorView();
                return;
            }
		}

		__INFO(
			"fs",
			"resuming file sharing upload\n"
			"	local_path=<" << FsInfo_->GetLocalPath() << ">\n"
			"	uploading_id=<" << FsInfo_->GetUploadingProcessId() << ">");
	}

    void FileSharingWidget::retryRequest()
    {
        if (Retry_.ShouldRetryFileMetainfo())
        {
            ++Retry_.FileMetainfoRetryCount_;

            FileMetainfoDownloadId_ = -1;

            requestFileMetainfo();
        }

        if (Retry_.ShouldRetryFileDownload())
        {
            ++Retry_.FileDownloadRetryCount_;

            FileDownloadId_ = -1;

            startDownloadingPlainFile();
        }

        if (Retry_.ShouldRetryPreviewDownload())
        {
            ++Retry_.PreviewDownloadRetryCount_;

            PreviewDownloadId_ = -1;

            requestPreview();
        }
    }

    bool FileSharingWidget::retryRequestLater()
    {
        assert(Retry_.HasRetryFlagSet());

        if (!Retry_.ShouldRetry())
        {
            return false;
        }

        QTimer::singleShot(
            RETRY_INTERVAL_MS,
            Qt::VeryCoarseTimer,
            this,
            &FileSharingWidget::retryRequest
        );

        return true;
    }

    void FileSharingWidget::setBlockSizePolicy()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedHeight(Utils::scale_value(64));
        emit forcedLayoutUpdatedSignal();
        updateGeometry();
        update();
    }

	void FileSharingWidget::setInitialWidgetSizeAndState()
	{
        if (!PreviewsEnabled_)
		{
			setState(State::PlainFile_Initial);
			return;
		}

		if (FsInfo_->HasSize())
		{
			setState(State::ImageFile_Initial);
			return;
		}

		if (!FsInfo_->HasLocalPath())
		{
			setState(State::PlainFile_Initial);
			return;
		}

		QFileInfo fileInfo(FsInfo_->GetLocalPath());
		const auto ext = fileInfo.suffix();
		if(ext.isEmpty() || !Utils::is_image_extension(ext))
		{
			setState(State::PlainFile_Initial);
			return;
		}

		setState(State::ImageFile_Initial);
	}

    void FileSharingWidget::setState(const State state)
	{
		assert(state > State::Min);
		assert(state < State::Max);
		assert(Private_.State_ > State::Min);
		assert(Private_.State_ < State::Max);

		const auto oldState = Private_.State_;
		Private_.State_ = state;
        emit stateChanged();

        if ((state == State::PlainFile_Initial) || (state == State::PlainFile_MetainfoLoaded))
        {
            setBlockSizePolicy();
        }

		switch (oldState)
		{
			case State::Initial:
				assert(isState(State::PlainFile_Initial) ||
					   isState(State::ImageFile_Initial));
				break;

			case State::PlainFile_Initial:
				break;

			case State::PlainFile_CheckingLocalCopy:
				assert((state == State::PlainFile_MetainfoLoaded) ||
					   (state == State::PlainFile_Downloaded));
				break;

			case State::PlainFile_MetainfoLoaded:
				assert(state == State::PlainFile_Downloading);
				break;

			case State::PlainFile_Downloading:
				assert((state == State::PlainFile_MetainfoLoaded) ||
					   (state == State::PlainFile_Downloaded) ||
                       (state == State::PlainFile_Downloading));
				break;

			case State::PlainFile_Uploading:
				assert(isState(State::PlainFile_Uploaded) ||
					   isState(State::PlainFile_UploadError) ||
					   isState(State::PlainFile_MetainfoLoaded) ||
					   isState(State::Deleted));
				break;

			case State::ImageFile_Initial:
				{
                    assert(isState(State::ImageFile_MetainfoLoaded) ||
						   isState(State::ImageFile_Uploading) ||
						   isState(State::ImageFile_Uploaded) ||
                           isState(State::ImageFile_Downloading));
				}
				break;

			case State::ImageFile_MetainfoLoaded:
				assert(isState(State::PlainFile_MetainfoLoaded) ||
                       isState(State::ImageFile_Downloading));
				break;

			case State::ImageFile_Downloading:
				{
					const auto isImageDownloaded = isState(State::ImageFile_Downloaded);
                    const auto isImageMetainfoLoaded = isState(State::ImageFile_MetainfoLoaded);
                    if (!isImageDownloaded && !isImageMetainfoLoaded)
                    {
                        assert(false);
                    }
					if (isImageDownloaded)
                    {
                        if (platform::is_apple())
                        {
                            auto path = DownloadedFileLocalPath_;
                            if (path.isEmpty() && FsInfo_.get())
                            {
                                path = FsInfo_->GetLocalPath();
                            }
                            assert(QFile::exists(path));
                            openPreviewMac(path, -1, -1);
                        }
                        else
                        {
                            openPreview();
                        }
					}
				}

				break;

			case State::ImageFile_Uploading:
				assert(isState(State::ImageFile_Uploaded) ||
					   isState(State::Deleted));
				break;

            case State::PlainFile_Downloaded:
                break;

			default:
				assert(!"unexpected internal state");
				break;
		}
	}

	void FileSharingWidget::startDownloadingPlainFile()
	{
		setState(State::PlainFile_Downloading);

		startDataTransferAnimation();

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(),
			core::file_sharing_function::download_file);

        assert(FileDownloadId_ == -1);
        FileDownloadId_ = procId;

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);
	}

	void FileSharingWidget::startDownloadingFullImage()
	{
		assert(isState(State::ImageFile_Downloading));

        // start animation

		startDataTransferAnimation();

        // start image downloading

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(
                settings_download_directory,
                Utils::DefaultDownloadsPath()
            ),
            QString(),
			core::file_sharing_function::download_file
        );

        assert(FileDownloadId_ == -1);
        FileDownloadId_ = procId;

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);
	}

	void FileSharingWidget::startDataTransferAnimation()
	{
		if (!BaseAngleAnimation_)
		{
			BaseAngleAnimation_ = new QPropertyAnimation(this, "DownloadingBarBaseAngle");
			BaseAngleAnimation_->setDuration(700);
			BaseAngleAnimation_->setLoopCount(-1);
			BaseAngleAnimation_->setStartValue(DOWNLOADING_BAR_BASE_ANGLE_MIN);
			BaseAngleAnimation_->setEndValue(DOWNLOADING_BAR_BASE_ANGLE_MAX);
		}

		stopDataTransferAnimation();

		BaseAngleAnimation_->start();
	}

	void FileSharingWidget::stopDataTransferAnimation()
	{
        if (!BaseAngleAnimation_)
        {
            return;
        }

        BaseAngleAnimation_->stop();
	}

	void FileSharingWidget::stopDownloading()
	{
		assert(isState(State::PlainFile_Downloading) ||
			   isState(State::ImageFile_Downloading));

		if (isState(State::PlainFile_Downloading))
		{
			setState(State::PlainFile_MetainfoLoaded);
		}

        if (isState(State::ImageFile_Downloading))
        {
            setState(State::ImageFile_MetainfoLoaded);
        }

		Ui::GetDispatcher()->abortSharedFileDownloading(
			ContactUin_,
			FsInfo_->GetUri(),
			getCurrentProcessId()
		);

		resetCurrentProcessId();

		BytesTransferred_ = 0;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_cancel);
	}

	void FileSharingWidget::stopUploading()
	{
		assert(isState(State::PlainFile_Uploading) ||
			   isState(State::ImageFile_Uploading));
		assert(!FsInfo_->GetUploadingProcessId().isEmpty());
		assert(FsInfo_->HasLocalPath());

		setState(State::Deleted);

		Ui::GetDispatcher()->abortSharedFileUploading(
			ContactUin_,
			FsInfo_->GetLocalPath(),
			FsInfo_->GetUploadingProcessId()
		);

		emit removeMe();

		BytesTransferred_ = 0;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_cancel);
	}

	void FileSharingWidget::onFileMetainfo(qint64 seq, QString filename, QString downloadUri, qint64 size)
	{
		assert(!filename.isEmpty());
		assert(!downloadUri.isEmpty());
		assert(size > 0);

		if (seq != FileMetainfoDownloadId_)
		{
			return;
		}

        const auto isImageFileInitial = isState(State::ImageFile_Initial);
        const auto isPlainFileInitial = isState(State::PlainFile_Initial);

        assert(isImageFileInitial || isPlainFileInitial);

		Metainfo_.Filename_ = filename;
		Metainfo_.FileSize_ = size;
		Metainfo_.DownloadUri_ = downloadUri;

        if (isPlainFileInitial)
        {
            setState(State::PlainFile_CheckingLocalCopy);
            checkLocalCopyExistence();
        }
        else if (isImageFileInitial)
        {
            setState(State::ImageFile_MetainfoLoaded);
        }

        FileMetainfoDownloadId_ = -1;
        Retry_.FileMetainfo_ = false;
        Retry_.FileMetainfoRetryCount_ = 0;

        invalidateSizes();
		update();
	}

    void FileSharingWidget::onPreviewMetainfo(qint64 seq, QString miniPreviewUri, QString fullPreviewUri)
    {
        assert(seq > 0);
        assert(!miniPreviewUri.isEmpty());
        assert(!fullPreviewUri.isEmpty());

        if (PreviewMetainfoDownloadId_ != seq)
        {
            return;
        }

        assert(isState(State::ImageFile_Initial) || isState(State::ImageFile_MetainfoLoaded));

        Metainfo_.MiniPreviewUri_ = miniPreviewUri;
        Metainfo_.FullPreviewUri_ = fullPreviewUri;

        requestPreview();
    }

	void FileSharingWidget::imageDownloaded(qint64 seq, QString uri, QPixmap preview, QString)
	{
		assert(!uri.isEmpty());

		if (!isCurrentProcessId(seq))
		{
			return;
		}

		const auto isDownloading = isState(State::ImageFile_Downloading);
		if (!isDownloading)
		{
            assert(!"unexpected widget state");
			return;
		}

		assert(!Metainfo_.DownloadUri_.isEmpty());
		assert(uri == Metainfo_.DownloadUri_);

        resetCurrentProcessId();

		if (preview.isNull())
		{
			return;
		}

		Preview_.FullImg_ = preview;
		setState(State::ImageFile_Downloaded);

        invalidateSizes();
		update();
	}

	void FileSharingWidget::fileDownloaded(qint64 seq, QString rawUri, QString localPath)
	{
		assert(!rawUri.isEmpty());
		assert(QFile::exists(localPath));

		if (seq != FileDownloadId_)
		{
			return;
		}

		DownloadedFileLocalPath_ = localPath;

        const auto isPlainFileDownloading = isState(State::PlainFile_Downloading);
		if (isPlainFileDownloading)
		{
			setState(State::PlainFile_Downloaded);
		}

		const auto isImageDownloading = isState(State::ImageFile_Downloading);
		if (isImageDownloading)
		{
            setState(State::ImageFile_Downloaded);
		}

        if (CopyFile_)
        {
            CopyFile_ = false;
            copyFile();
        }

        if (SaveAs_)
        {
            SaveAs_ = false;
        }

        FileDownloadId_ = -1;
        Retry_.FileDownload_ = false;
        Retry_.FileMetainfoRetryCount_ = 0;

        stopDataTransferAnimation();

        invalidateSizes();
        update();
	}

	void FileSharingWidget::fileDownloading(qint64 seq, QString uri, qint64 bytesDownloaded)
	{
		assert(bytesDownloaded >= 0);

		if (seq != FileDownloadId_)
		{
			return;
		}

		__DISABLE(
			__TRACE(
				"fs",
				"file downloading progress\n"
				<< "\tseq= " << seq << "\n"
				<< "\turi= " << uri << "\n"
				<< "\tbytes_downloaded= " << bytesDownloaded);
		);

		BytesTransferred_ = bytesDownloaded;

		update();
	}

	void FileSharingWidget::fileSharingError(qint64 seq, QString rawUri, qint32 errorCode)
	{
        assert(seq > 0);

        errorCode;

        const auto isFileMetainfoRequestFailed = (seq == FileMetainfoDownloadId_);
        const auto isPreviewMetainfoRequestFailed = (seq == PreviewMetainfoDownloadId_);
        const auto isFileRequestFailed = (seq == FileDownloadId_);
        const auto isLocalCopyCheckFailed = (seq == CheckLocalCopyExistenceId_);

        if (!isFileMetainfoRequestFailed &&
            !isPreviewMetainfoRequestFailed &&
            !isFileRequestFailed &&
            !isLocalCopyCheckFailed)
        {
            return;
        }

        if (isFileMetainfoRequestFailed)
        {
            Retry_.FileMetainfo_ = true;
        }

        if (isFileRequestFailed)
        {
            Retry_.FileDownload_ = true;
        }

        if (Retry_.HasRetryFlagSet() && retryRequestLater())
        {
            return;
        }

        stopDataTransferAnimation();

        convertToPlainFileView();

        invalidateSizes();
        update();
	}

	void FileSharingWidget::fileLocalCopyChecked(qint64 seq, bool exists, QString localPath)
	{
		if (seq != CheckLocalCopyExistenceId_)
		{
			return;
		}

		assert(isState(State::PlainFile_CheckingLocalCopy));

		if (exists)
		{
			assert(QFile::exists(localPath));
			DownloadedFileLocalPath_ = localPath;
			setState(State::PlainFile_Downloaded);
		}
		else
		{
			setState(State::PlainFile_MetainfoLoaded);
		}

		update();
	}

	void FileSharingWidget::fileSharingUploadingProgress(QString uploadingProcessId, qint64 bytesUploaded)
	{
		assert(bytesUploaded >= 0);
		assert(Metainfo_.FileSize_ >= 0);
		assert(!uploadingProcessId.isEmpty());

		const auto isMyProcessId = (uploadingProcessId == FsInfo_->GetUploadingProcessId());
		if (!isMyProcessId)
		{
			return;
		}

		BytesTransferred_ = bytesUploaded;

		__DISABLE(
			__TRACE(
				"fs",
				"uploading progress\n" <<
				"	transferred=<" << BytesTransferred_ << "/" << Metainfo_.FileSize_ << ">"
			);
		);

        update();
	}

    void FileSharingWidget::fileSharingUploadingResult(QString uploadingProcessId, bool success, QString link, bool tooLargeFile)
    {
        (void)success;

        const auto isMyProcessId = (uploadingProcessId == FsInfo_->GetUploadingProcessId());
        if (!isMyProcessId)
        {
            return;
        }

        __TRACE(
            "fs",
            "uploading finished\n" <<
            "    succeed=<" << logutils::yn(success) << ">\n"
            "    link=<" << link << ">"
        );

        FsInfo_->SetUri(link);

        if (isState(State::ImageFile_Uploading))
        {
            setState(State::ImageFile_Uploaded);
        }
        else
        {
            setState(State::PlainFile_Uploaded);
        }

        stopDataTransferAnimation();

        if (tooLargeFile)
        {
            emit removeMe();
            return;
        }

        update();
    }

    void FileSharingWidget::localPreviewLoaded(QPixmap pixmap)
    {
        startDataTransferAnimation();

        if (pixmap.isNull())
        {
            setState(State::PlainFile_Uploading);

            return;
        }

        Preview_.FullImg_ = pixmap;

        setPreview(pixmap);

        setState(State::ImageFile_Uploading);
    }

    void FileSharingWidget::previewDownloaded(qint64 seq, QString uri, QPixmap preview, QString)
    {
        if (PreviewDownloadId_ != seq)
        {
            return;
        }

        assert(PreviewState_ != PreviewState::FullPreviewLoaded);

        if (preview.isNull())
        {
            return;
        }

        const auto isPreviewEmpty = (PreviewState_ == PreviewState::NoPreview);
        if (isPreviewEmpty)
        {
            assert(Preview_.FullImg_.isNull());

            setPreview(preview);

            PreviewState_ = PreviewState::MiniPreviewLoaded;

            PreviewDownloadId_ = -1;
            Retry_.PreviewDownload_ = false;
            Retry_.PreviewDownloadRetryCount_ = 0;

            requestPreview();

            return;
        }

        assert(PreviewState_ == PreviewState::MiniPreviewLoaded);

        setPreview(preview);

        PreviewState_ = PreviewState::FullPreviewLoaded;

        PreviewDownloadId_ = -1;
        Retry_.PreviewDownload_ = false;
        Retry_.PreviewDownloadRetryCount_ = 0;
    }

    void FileSharingWidget::previewDownloadError(int64_t seq, QString rawUri)
    {
        assert(seq > 0);
        assert(!rawUri.isEmpty());

        const auto isMySeq = (seq == PreviewDownloadId_);
        if (!isMySeq)
        {
            return;
        }

        Retry_.PreviewDownload_ = true;

        if (retryRequestLater())
        {
            return;
        }

        stopDataTransferAnimation();

        convertToPlainFileView();

        invalidateSizes();
        update();
    }

}

namespace
{
	const Themes::IThemePixmap& downloadButton()
	{
		static const auto btn = Themes::GetPixmap(Themes::PixmapResourceId::FileSharingDownload);
		return *btn;
	}

	const Themes::IThemePixmap& downloadCancelButton()
	{
		static const auto btn = Themes::GetPixmap(Themes::PixmapResourceId::FileSharingPlainCancel);
		return *btn;
	}
}

