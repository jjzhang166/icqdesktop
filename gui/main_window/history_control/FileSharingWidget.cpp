#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../utils/InterConnector.h"
#include "../../gui_settings.h"
#include "../../core_dispatcher.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../previewer/Previewer.h"

#include "../MainPage.h"
#include "HistoryControlPage.h"

#include "KnownFileTypes.h"
#include "FileSharingInfo.h"
#include "FileSizeFormatter.h"
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

	const auto PROGRESS_PEN_WIDTH = 2;

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
		ImageFile_MiniPreviewLoaded,
		ImageFile_FullPreviewLoaded,
		ImageFile_Downloading,
		ImageFile_Downloaded,
		ImageFile_Uploading,
		ImageFile_Uploaded,

        ImageFile_Max,

		Deleted,

		Max
	};

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
        , RetryCount_(0)
        , CopyFile_(false)
        , SaveAs_(false)
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
        , RetryCount_(0)
        , CopyFile_(false)
        , SaveAs_(false)
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

	void FileSharingWidget::initialize()
	{
        PreviewContentWidget::initialize();

		__TRACE("fs", "initializing file sharing widget\n" << FsInfo_->ToLogString());

		setMouseTracking(true);

		setInitialWidgetSizeAndState();

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

		connectMetainfoSignal(true);
		requestMetainfo();

        invalidateSizes();
        update();
	}

	QString FileSharingWidget::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		fmt << "	widget=<file_sharing>\n";

		if (FsInfo_->IsOutgoing())
		{
			fmt << "	type=<outgoing>\n"
				   "	local_path=<" << FsInfo_->GetLocalPath() << ">";
		}
		else
		{
			fmt << "	type=<incoming>\n"
				   "	uri=<" << FsInfo_->GetUri() << ">";
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
                startDownloadingFullImage();
            else
                startDownloadingPlainFile();
        }
        else
        {
            Utils::copyFileToClipboard(DownloadedFileLocalPath_);
        }
    }

    void FileSharingWidget::saveAs()
    {
        QString dir, file;
        if (Utils::saveAs(Metainfo_.Filename_, file, dir))
        {
            if (isImagePreview())
            {
                startDataTransferAnimation();
                connectMetainfoSignal(false);
                connectImageDownloadedSignal(false);
                resetCurrentProcessId();
                connectFileDownloadSignals(true);
                const auto procId = Ui::GetDispatcher()->downloadSharedFile(
                    ContactUin_,
                    FsInfo_->GetUri(),
                    dir, file,
                    core::file_sharing_function::download_file);

                setCurrentProcessId(procId);
            }
            else
            {
                setState(State::PlainFile_Downloading);
                startDataTransferAnimation();
                connectFileDownloadSignals(true);
                const auto procId = Ui::GetDispatcher()->downloadSharedFile(
                    ContactUin_,
                    FsInfo_->GetUri(),
                    dir, file,
                    core::file_sharing_function::download_file);

                setCurrentProcessId(procId);
            }

            SaveAs_ = true;
        }
    }

    bool FileSharingWidget::haveContentMenu(QPoint) const
    {
        return true;
    }

	bool FileSharingWidget::canStartImageDownloading(const QPoint &mousePos) const
	{
        const auto isInRightState = isState(State::ImageFile_FullPreviewLoaded) ||
                                    isState(State::ImageFile_MiniPreviewLoaded) ||
                                    isState(State::ImageFile_MetainfoLoaded);

		return (isInRightState && isOverPreview(mousePos));
	}

	void FileSharingWidget::checkLocalCopyExistence()
	{
		assert(isState(State::PlainFile_CheckingLocalCopy));

		connectFileDownloadSignals(true);

		const auto procId = (int)Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(),
			core::file_sharing_function::check_local_copy_exists);

        setCurrentProcessId(procId);
	}

	void FileSharingWidget::connectFileDownloadSignals(const bool isConnected)
	{
		connectCoreSignal(
			SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)),
			SLOT(fileDownloaded(qint64, QString, QString)),
			isConnected);

		connectCoreSignal(
			SIGNAL(fileSharingFileDownloading(qint64, QString, qint64)),
			SLOT(fileDownloading(qint64, QString, qint64)),
			isConnected);

		connectCoreSignal(
			SIGNAL(fileSharingDownloadError(qint64, QString, qint32)),
			SLOT(fileDownloadError(qint64, QString, qint32)),
			isConnected);

		connectCoreSignal(
			SIGNAL(fileSharingLocalCopyCheckCompleted(qint64, bool, QString)),
			SLOT(fileLocalCopyChecked(qint64, bool, QString)),
			isConnected);
	}

	void FileSharingWidget::connectImageDownloadedSignal(const bool isConnected)
	{
		connectCoreSignal(
			SIGNAL(imageDownloaded(qint64, QString, QPixmap, QString)),
			SLOT(imageDownloaded(qint64, QString, QPixmap, QString)),
			isConnected);
	}

	void FileSharingWidget::connectMetainfoSignal(const bool isConnected)
	{
		connectCoreSignal(
			SIGNAL(fileSharingMetadataDownloaded(qint64, QString, QString, QString, QString, QString, qint64, bool)),
			SLOT(setFileInfo(qint64, QString, QString, QString, QString, QString, qint64, bool)),
			isConnected);

        connectCoreSignal(
            SIGNAL(fileSharingDownloadError(qint64, QString, qint32)),
            SLOT(metaDownloadError(qint64, QString, qint32)),
            isConnected);
	}

	void FileSharingWidget::connectFileUploadingSignals(const bool isConnected)
	{
		connectCoreSignal(
			SIGNAL(fileSharingUploadingProgress(QString, qint64)),
			SLOT(fileSharingUploadingProgress(QString, qint64)),
			isConnected);
	}

	void FileSharingWidget::convertToPlainFileView()
	{
		setState(State::PlainFile_MetainfoLoaded);
	}

    void FileSharingWidget::convertToPlainImageView()
    {
        setState(State::ImageFile_MetainfoLoaded);
    }

	void FileSharingWidget::convertToUploadErrorView()
	{
		setState(State::PlainFile_UploadError);
		update();

		Metainfo_.Filename_ = "not found";
		Metainfo_.FileSize_ = 0;
		Metainfo_.FileSizeStr_.resize(0);
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

		if (canStartImageDownloading(mousePos))
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
            return;

		const auto isLeftClick = (event->button() == Qt::LeftButton);
		if (!isLeftClick)
		{
			return;
		}

		const auto mousePos = event->pos();

		const auto clickedOnButton = (isControlButtonVisible() && isOverControlButton(mousePos));

		const auto isDownloadButtonClicked = (clickedOnButton && isState(State::PlainFile_MetainfoLoaded));
		if (isDownloadButtonClicked)
		{
			startDownloadingPlainFile();
			return;
		}

		const auto isStopDownloadButtonClicked = (clickedOnButton && isState(State::PlainFile_Downloading));
		if (isStopDownloadButtonClicked)
		{
			stopDownloading();
			return;
		}

		const auto isStopUploadButtonClicked = (clickedOnButton && isState(State::PlainFile_Uploading));
		if (isStopUploadButtonClicked)
		{
			stopUploading();
			return;
		}

		const auto isOpenDownloadsDirButtonClicked = (isOpenDownloadsDirButtonVisible() &&
													  isOverOpenDownloadsDirButton(mousePos));
		if (isOpenDownloadsDirButtonClicked)
		{
			openDownloadsDir();
			return;
		}

		if (isState(State::ImageFile_Downloaded) && isOverPreview(mousePos))
		{
            if (platform::is_apple())
            {
                openPreviewMac(event->globalX(), event->globalY());
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
		return isState(State::ImageFile_MiniPreviewLoaded) ||
			   isState(State::ImageFile_FullPreviewLoaded) ||
			   isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Downloaded) ||
			   isState(State::ImageFile_Uploading) ||
			   isState(State::ImageFile_Uploaded);
	}

	bool FileSharingWidget::isState(const State state) const
	{
		return (Private_.State_ == state);
	}

	bool FileSharingWidget::loadPreviewFromLocalFile()
	{
		assert(isState(State::ImageFile_Initial));
		assert(QFile::exists(FsInfo_->GetLocalPath()));

		QPixmap preview;

		if (!preview.load(FsInfo_->GetLocalPath()))
		{
			return false;
		}

		setPreview(preview);

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

    void FileSharingWidget::openPreviewMac(int x, int y)
    {
        Q_UNUSED(x);
        Q_UNUSED(y);

        assert(QFile::exists(DownloadedFileLocalPath_));

#ifdef __APPLE__
        MacSupport::showPreview(DownloadedFileLocalPath_, x, y);
#endif
    }

	void FileSharingWidget::openPreview()
	{
        Ui::MainPage::instance()->cancelSelection();

		assert(QFile::exists(DownloadedFileLocalPath_));

        if (!Preview_.FullImg_.isNull())
		{
			Previewer::ShowPreview(Preview_.FullImg_);
			return;
		}

		// try to load with the default image format first
		Preview_.FullImg_.load(DownloadedFileLocalPath_);

		if (!Preview_.FullImg_.isNull())
		{
			Previewer::ShowPreview(Preview_.FullImg_);
			return;
		}

		// try to load the image with the explicitly specified format
		// it's a workaround for the jfif/png issue and so on

		static const char *availableFormats[] = { "jpg", "png", "gif", "bmp" };

		for (auto fmt : availableFormats)
		{
			Preview_.FullImg_.load(DownloadedFileLocalPath_, fmt);

			if (!Preview_.FullImg_.isNull())
			{
				Previewer::ShowPreview(Preview_.FullImg_);
				return;
			}
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
		QPen pen(QBrush(0x579e1c), penWidth);
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

	void FileSharingWidget::requestMetainfo()
	{
		assert(FsInfo_);
		assert(isState(State::PlainFile_Initial) || isState(State::ImageFile_Initial));

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(),
			core::file_sharing_function::download_meta);

        setCurrentProcessId(procId);
	}

	void FileSharingWidget::requestPreview()
	{
		const auto isImageMetainfoLoadedState = isState(State::ImageFile_MetainfoLoaded);
		const auto isImageMiniPreviewLoadedState = isState(State::ImageFile_MiniPreviewLoaded);
		assert(isImageMetainfoLoadedState || isImageMiniPreviewLoadedState);

		if (!isImageMetainfoLoadedState && !isImageMiniPreviewLoadedState)
		{
			return;
		}

		const auto &previewUri = (isImageMetainfoLoadedState ? Metainfo_.MiniPreviewUri_ : Metainfo_.FullPreviewUri_);

		if (previewUri.isEmpty())
		{
			if (isImageMetainfoLoadedState)
			{
				connectImageDownloadedSignal(false);
			}

			return;
		}

		connectImageDownloadedSignal(true);

		const auto procId = Ui::GetDispatcher()->downloadImagePreview(previewUri);

        setCurrentProcessId(procId);
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
		}
		else
		{
			if (loadPreviewFromLocalFile())
			{
				setState(State::ImageFile_Uploading);
			}
			else
			{
				setState(State::PlainFile_Uploading);
			}
		}

		startDataTransferAnimation();

		connectFileUploadingSignals(true);

		__INFO(
			"fs",
			"resuming file sharing upload\n"
			"	local_path=<" << FsInfo_->GetLocalPath() << ">\n"
			"	uploading_id=<" << FsInfo_->GetUploadingProcessId() << ">");
	}

    void FileSharingWidget::retryRequest()
    {
        if (isState(State::PlainFile_Initial) ||
            isState(State::ImageFile_Initial))
        {
            requestMetainfo();
            return;
        }

        if (isState(State::ImageFile_MetainfoLoaded))
        {
            requestPreview();
            return;
        }

        assert(!"unexpected state");
    }

    bool FileSharingWidget::retryRequestLater()
    {
        if (!isState(State::PlainFile_Initial) &&
            !isState(State::ImageFile_Initial) &&
            !isState(State::ImageFile_MetainfoLoaded))
        {
            return false;
        }

        assert(RetryCount_ >= 0);
        assert(RetryCount_ <= RETRY_MAX);
        if (RetryCount_ >= RETRY_MAX)
        {
            return false;
        }

        ++RetryCount_;

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
                if (build::is_debug())
				{
// 					const auto isCheckingLocalCopy = isState(State::PlainFile_CheckingLocalCopy);
// 					const auto isUploadError = isState(State::PlainFile_UploadError);
// 					const auto isUploading = isState(State::PlainFile_Uploading);
// 					const auto isUploaded = isState(State::PlainFile_Uploaded);

				//	assert(isCheckingLocalCopy || isUploadError || isUploading || isUploaded);
				}
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
					   (state == State::PlainFile_Downloaded));
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
                    if (isState(State::ImageFile_Initial))
                    {
                        startDownloadingFullImage();
                    }
				}
				break;

			case State::ImageFile_MetainfoLoaded:
				assert(isState(State::ImageFile_MiniPreviewLoaded) ||
					   isState(State::PlainFile_MetainfoLoaded) ||
                       isState(State::ImageFile_Downloading));
                if (isState(State::ImageFile_Downloading))
                {
                    startDownloadingFullImage();
                }
				break;

			case State::ImageFile_MiniPreviewLoaded:
				assert(isState(State::ImageFile_FullPreviewLoaded) ||
                       isState(State::ImageFile_Downloading));
                if (isState(State::ImageFile_Downloading))
                {
                    startDownloadingFullImage();
                }
				break;

			case State::ImageFile_FullPreviewLoaded:
				assert(isState(State::ImageFile_Downloading));
				startDownloadingFullImage();
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
                            openPreviewMac(-1, -1);
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

		connectFileDownloadSignals(true);

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(),
			core::file_sharing_function::download_file);

        setCurrentProcessId(procId);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);
	}

	void FileSharingWidget::startDownloadingFullImage()
	{
		assert(isState(State::ImageFile_Downloading));

        // start animation

		startDataTransferAnimation();

        // reset existing process

        connectMetainfoSignal(false);

		connectImageDownloadedSignal(false);

        resetCurrentProcessId();

        // start image downloading

        connectFileDownloadSignals(true);

		const auto procId = Ui::GetDispatcher()->downloadSharedFile(
			ContactUin_,
			FsInfo_->GetUri(),
            Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(),
			core::file_sharing_function::download_file);

        setCurrentProcessId(procId);
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
        BaseAngleAnimation_->stop();
	}

	void FileSharingWidget::stopDownloading()
	{
		assert(isState(State::PlainFile_Downloading) ||
			   isState(State::ImageFile_Downloading));

		if (isState(State::PlainFile_Downloading))
		{
			setState(State::PlainFile_MetainfoLoaded);
			connectFileDownloadSignals(false);
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

	void FileSharingWidget::setFileInfo(qint64 seq, QString rawUri, QString miniPreviewUri, QString fullPreviewUri, QString downloadUri, QString filename, qint64 size, bool)
	{
		assert(!rawUri.isEmpty());
		assert(!filename.isEmpty());
		assert(!downloadUri.isEmpty());
		assert(size > 0);

		if (!isCurrentProcessId(seq))
		{
			return;
		}

        RetryCount_ = 0;

		connectMetainfoSignal(false);
        resetCurrentProcessId();

		assert(isState(State::ImageFile_Initial) ||
			   isState(State::PlainFile_Initial));

		Metainfo_.Filename_ = filename;
		Metainfo_.FileSize_ = size;
		Metainfo_.MiniPreviewUri_ = miniPreviewUri;
		Metainfo_.FullPreviewUri_ = fullPreviewUri;
		Metainfo_.DownloadUri_ = downloadUri;

		if (isState(State::ImageFile_Initial))
		{
			setState(State::ImageFile_MetainfoLoaded);
			requestPreview();
		}
		else
		{
			setState(State::PlainFile_CheckingLocalCopy);
			checkLocalCopyExistence();
		}

        invalidateSizes();
		update();
	}

	void FileSharingWidget::imageDownloaded(qint64 seq, QString uri, QPixmap preview, QString)
	{
		assert(!uri.isEmpty());

		if (!isCurrentProcessId(seq))
		{
			return;
		}

        const auto isMetainfoLoaded = isState(State::ImageFile_MetainfoLoaded);
		const auto isMiniPreviewLoaded = isState(State::ImageFile_MiniPreviewLoaded);
		const auto isDownloading = isState(State::ImageFile_Downloading);
		if (!isMetainfoLoaded && !isMiniPreviewLoaded && !isDownloading)
		{
			return;
		}

		if (isMetainfoLoaded)
		{
			assert(!Metainfo_.MiniPreviewUri_.isEmpty());
			assert(uri == Metainfo_.MiniPreviewUri_);
		}

		if (isMiniPreviewLoaded)
		{
			assert(!Metainfo_.FullPreviewUri_.isEmpty());
			assert(uri == Metainfo_.FullPreviewUri_);
		}

		if (isDownloading)
		{
			assert(!Metainfo_.DownloadUri_.isEmpty());
			assert(uri == Metainfo_.DownloadUri_);
		}

        resetCurrentProcessId();

		if (preview.isNull() && isMetainfoLoaded)
		{
            if (!retryRequestLater())
            {
                connectImageDownloadedSignal(false);
            }

			return;
		}

		if (isDownloading)
		{
			Preview_.FullImg_ = preview;
			setState(State::ImageFile_Downloaded);
			connectImageDownloadedSignal(false);
		}

		if (isMetainfoLoaded || isMiniPreviewLoaded)
		{
			setPreview(preview);
		}

		if (isMetainfoLoaded)
		{
			setState(State::ImageFile_MiniPreviewLoaded);
			requestPreview();
		}

		if (isMiniPreviewLoaded)
		{
			setState(State::ImageFile_FullPreviewLoaded);
			connectImageDownloadedSignal(false);
		}

        invalidateSizes();
		update();
	}

	void FileSharingWidget::fileDownloaded(qint64 seq, QString rawUri, QString localPath)
	{
		assert(!rawUri.isEmpty());
		assert(QFile::exists(localPath));

		if (!isCurrentProcessId(seq))
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

		stopDataTransferAnimation();

        invalidateSizes();
		update();

        if (CopyFile_)
        {
            CopyFile_ = false;
            copyFile();
        }

        if (SaveAs_)
        {
            SaveAs_ = false;
        }
	}

	void FileSharingWidget::fileDownloading(qint64 seq, QString uri, qint64 bytesDownloaded)
	{
		assert(bytesDownloaded >= 0);

		if (!isCurrentProcessId(seq))
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

	void FileSharingWidget::fileDownloadError(qint64 seq, QString uri, qint32)
	{
		if (!isCurrentProcessId(seq))
		{
			return;
		}

        resetCurrentProcessId();

        if (retryRequestLater())
        {
            return;
        }

		connectFileDownloadSignals(false);

        if (isImagePreview())
        {
		    convertToPlainImageView();
        }
        else
        {
            convertToPlainFileView();
        }

		stopDataTransferAnimation();

        invalidateSizes();
        update();
	}

	void FileSharingWidget::fileLocalCopyChecked(qint64 seq, bool exists, QString localPath)
	{
		if (!isCurrentProcessId(seq))
		{
			return;
		}

		assert(isState(State::PlainFile_CheckingLocalCopy));

		connectFileDownloadSignals(false);

        resetCurrentProcessId();

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

		const auto isCompleted = (bytesUploaded == Metainfo_.FileSize_);

		__DISABLE(
			__TRACE(
				"fs",
				"uploading progress\n" <<
				"	transferred=<" << BytesTransferred_ << "/" << Metainfo_.FileSize_ << ">"
			);
		);

		if (isCompleted)
		{
			if (isState(State::ImageFile_Uploading))
			{
				setState(State::ImageFile_Uploaded);
			}
			else
			{
				setState(State::PlainFile_Uploaded);
			}

			connectFileUploadingSignals(false);
			stopDataTransferAnimation();
		}

		update();
	}

    void FileSharingWidget::metaDownloadError(qint64 seq, QString, qint32)
    {
        if (!isCurrentProcessId(seq))
        {
            return;
        }

        resetCurrentProcessId();

        if (retryRequestLater())
        {
            return;
        }

        connectMetainfoSignal(false);

        convertToPlainImageView();

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

