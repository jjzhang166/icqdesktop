#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../main_window/ContactDialog.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../types/images.h"
#include "../utils/InterConnector.h"

#include "DownloadWidget.h"
#include "GalleryFrame.h"
#include "ImageCache.h"
#include "ImageIterator.h"
#include "ImageLoader.h"
#include "ImageViewerWidget.h"

#include "GalleryWidget.h"

namespace
{
    const qreal backgroundOpacity = 0.8;
    const QBrush backgroundColor = QColor::fromRgb(0, 0, 0);
    const int smallSpacing = 8;
    const int largeSpacing = 24;
    const int cacheSize = 5; // odd number only
    const int middleCachePos = cacheSize / 2;
    const int scrollTimeoutMsec = 200;
    const int delayTimeoutMsec = 300;

    enum
    {
        DownloadIndex = 0,
        ImageIndex
    };
}

Previewer::GalleryWidget::GalleryWidget(QWidget* _parent)
    : QWidget(_parent)
    , loaders_(cacheSize)
    , navigationKeyPressed_(false)
    , initComplete_(false)
    , cacheLoaded_(false)
{
    assert(cacheSize % 2);

    setMouseTracking(true);

    QHBoxLayout* buttonWrapperLayout = new QHBoxLayout();
    buttonWrapperLayout->addWidget(createButtonFrame());
    buttonWrapperLayout->addSpacing(Utils::scale_value(smallSpacing));
    buttonWrapperLayout->addWidget(createCloseFrame());

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addLayout(buttonWrapperLayout);
    buttonLayout->addStretch();

    download_ = new DownloadWidget(this);
    connect(download_, &DownloadWidget::clicked, this, &GalleryWidget::onImageClicked);

    imageViewer_ = new ImageViewerWidget(this);
    connect(imageViewer_, &ImageViewerWidget::imageClicked, this, &GalleryWidget::onImageClicked);

    imageOrDownload_ = new QStackedLayout();
    imageOrDownload_->addWidget(download_);
    imageOrDownload_->addWidget(imageViewer_);
    imageOrDownload_->setCurrentIndex(ImageIndex);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addSpacing(Utils::scale_value(largeSpacing));
    layout->addLayout(imageOrDownload_);
    layout->addSpacing(Utils::scale_value(largeSpacing));
    layout->addLayout(buttonLayout);
    layout->addSpacing(Utils::scale_value(largeSpacing));

    setLayout(layout);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

    scrollTimer_ = new QTimer(this);
    connect(scrollTimer_, &QTimer::timeout, scrollTimer_, &QTimer::stop);

    delayTimer_ = new QTimer(this);
    connect(delayTimer_, &QTimer::timeout,
        [this]()
        {
            delayTimer_->stop();
            save_->setEnabled(false);
            imageOrDownload_->setCurrentIndex(DownloadIndex);
            download_->startLoading();
        });
}

Previewer::GalleryWidget::~GalleryWidget()
{
}

void Previewer::GalleryWidget::openGallery(const QString& _aimId, const Data::Image& _image, const QString& _localPath)
{
    assert(!_image.isNull());
    assert(!_localPath.isEmpty());

    aimId_ = _aimId;

    info_->setText(getDefaultText());

    imageCache_.reset(new ImageCache(_aimId));

    imageIterator_.reset(new ImageIterator(imageCache_.get(), _image));
    connect(imageIterator_.get(), &ImageIterator::iteratorUpdated, this, &GalleryWidget::onIteratorUpdated, Qt::QueuedConnection);
    connect(imageIterator_.get(), &ImageIterator::numberFound, this, &GalleryWidget::onNumberFound, Qt::QueuedConnection);
    connect(imageIterator_.get(), &ImageIterator::cacheLoaded, this, &GalleryWidget::onCacheLoaded, Qt::QueuedConnection);

    moveToScreen();

    showFullScreen();

    showProgress();

    setFirstImage(_image, _localPath);

    grabKeyboard();
}

void Previewer::GalleryWidget::closeGallery()
{
    if (!imageCache_) // allready closed
        return;

    initComplete_ = false;
    cacheLoaded_ = false;

    releaseKeyboard();
    Ui::MainPage::instance()->getContactDialog()->setFocusOnInputWidget();

    for (auto& loader : loaders_)
        loader.reset();

    imageCache_.reset();
    imageIterator_.reset();

    imageViewer_->reset();

    repaint();

    showNormal();

    close();
}

void Previewer::GalleryWidget::moveToScreen()
{
    const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
    const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);
    setGeometry(screenGeometry);
}

QFrame* Previewer::GalleryWidget::createButtonFrame()
{
    auto buttonsFrame = new GalleryFrame(this);

    buttonsFrame->addSpace(GalleryFrame::SpaceSize::Small);

    next_ = buttonsFrame->addButton("prev", GalleryFrame::Disabled | GalleryFrame::Hover | GalleryFrame::Pressed);
    next_->setDisabled(true);
    connect(next_, &QPushButton::clicked, this, &GalleryWidget::onNextClicked);

    buttonsFrame->addSpace(GalleryFrame::SpaceSize::Medium);

    prev_ = buttonsFrame->addButton("next", GalleryFrame::Disabled | GalleryFrame::Hover | GalleryFrame::Pressed);
    prev_->setDisabled(true);
    connect(prev_, &QPushButton::clicked, this, &GalleryWidget::onPrevClicked);

    buttonsFrame->addSpace(GalleryFrame::SpaceSize::Large);

    info_ = buttonsFrame->addLabel();
    info_->setAlignment(Qt::AlignCenter);
    info_->setMinimumWidth(Utils::scale_value(90));

    buttonsFrame->addSpace(GalleryFrame::SpaceSize::Large);

    save_ = buttonsFrame->addButton("download", GalleryFrame::Disabled | GalleryFrame::Active | GalleryFrame::Hover | GalleryFrame::Pressed);
    save_->setEnabled(false);
    connect(save_, &QPushButton::clicked, this, &GalleryWidget::onSaveClicked);

    buttonsFrame->addSpace(GalleryFrame::SpaceSize::Small);

    return buttonsFrame;
}

QFrame* Previewer::GalleryWidget::createCloseFrame()
{
    auto closeFrame = new GalleryFrame(nullptr);

    auto closeButton = closeFrame->addButton("close", GalleryFrame::Hover | GalleryFrame::Pressed);
    connect(closeButton, &QPushButton::clicked, this, &GalleryWidget::closeGallery);

    return closeFrame;
}

QString Previewer::GalleryWidget::getDefaultText() const
{
    return QT_TRANSLATE_NOOP("previewer", "Loading...");
}

QString Previewer::GalleryWidget::getInfoText(int _n, int _total) const
{
    return QT_TRANSLATE_NOOP("previewer", "%1 of %2").arg(_n).arg(_total);
}

void Previewer::GalleryWidget::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        closeGallery();
    }
}

void Previewer::GalleryWidget::keyPressEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (key == Qt::Key_Escape)
    {
        closeGallery();
    }
    else if (key == Qt::Key_Right && hasPrev())
    {
        navigationKeyPressed_ = true;
        prev();
    }
    else if (key == Qt::Key_Left && hasNext())
    {
        navigationKeyPressed_ = true;
        next();
    }
}

void Previewer::GalleryWidget::keyReleaseEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (navigationKeyPressed_ && (key == Qt::Key_Left || key == Qt::Key_Right))
    {
        navigationKeyPressed_ = false;
        startLoading();
    }
}

void Previewer::GalleryWidget::wheelEvent(QWheelEvent* _event)
{
    const int delta = _event->delta();
    if (delta == 0)
        return;

    if (scrollTimer_->isActive())
        return;

    scrollTimer_->start(scrollTimeoutMsec);

    if (delta < 0 && hasNext())
    {
        next();
        startLoading();
    }
    else if (delta > 0 && hasPrev())
    {
        prev();
        startLoading();
    }
}

void Previewer::GalleryWidget::paintEvent(QPaintEvent* /*_event*/)
{
    QPainter painter(this);
    painter.setOpacity(backgroundOpacity);
    painter.fillRect(0, 0, width(), height(), backgroundColor);
}

void Previewer::GalleryWidget::onNumberFound()
{
    for (int i = 1; i <= middleCachePos; ++i)
    {
        downloadRelativeImage(-i);
        downloadRelativeImage(i);
    }
    initComplete_ = true;
}

void Previewer::GalleryWidget::onIteratorUpdated()
{
    updateButtonFrame();
}

void Previewer::GalleryWidget::updateButtonFrame()
{
    next_->setEnabled(hasNext());
    prev_->setEnabled(hasPrev());

    if (cacheLoaded_)
        info_->setText(getInfoText(imageIterator_->getNumber(), imageIterator_->getTotal()));
}

void Previewer::GalleryWidget::onPrevClicked()
{
    if (!initComplete_)
        return;

    prev();
    startLoading();
}

void Previewer::GalleryWidget::onNextClicked()
{
    if (!initComplete_)
        return;

    next();
    startLoading();
}

void Previewer::GalleryWidget::onImageClicked()
{
    if (hasPrev())
    {
        prev();
        startLoading();
    }
}

void Previewer::GalleryWidget::onSaveClicked()
{
    if (!initComplete_)
        return;

    auto& loader = currentLoader();

    QString dir;
    QString name;

    if (!Utils::saveAs(loader.getFileName(), name, dir, false))
    {
        return;
    }

    save_->setActive(true);
    const bool success = QFile::copy(loader.getLocalFileName(), QFileInfo(dir, name).absoluteFilePath());
    (void) success; // supress warning
    save_->setActive(false);

    assert(success);
}

void Previewer::GalleryWidget::onImageLoaded()
{
    auto& loader = currentLoader();
    if (loader.getState() == ImageLoader::State::Success)
    {
        showImage(loader.getPixmap(), loader.getLocalFileName());
    }
}

void Previewer::GalleryWidget::onImageLoadingError()
{
    QTimer::singleShot(0, [this]() { showError(); });
}

void Previewer::GalleryWidget::onCacheLoaded()
{
    cacheLoaded_ = true;
    updateButtonFrame();
}

void Previewer::GalleryWidget::showImage(const QPixmap& _preview, const QString& _fileName)
{
    delayTimer_->stop();
    save_->setEnabled(true);
    imageOrDownload_->setCurrentIndex(ImageIndex);
    imageViewer_->showImage(_preview, _fileName);
    updateButtonFrame();
}

void Previewer::GalleryWidget::showProgress()
{
    delayTimer_->start(delayTimeoutMsec);
}

void Previewer::GalleryWidget::showError()
{
    delayTimer_->stop();
    save_->setEnabled(false);
    imageOrDownload_->setCurrentIndex(DownloadIndex);
    download_->stopLoading();
}

void Previewer::GalleryWidget::setFirstImage(const Data::Image& _imageInfo, const QString& _localPath)
{
    showProgress();
    auto loader = std::unique_ptr<ImageLoader>(new ImageLoader(aimId_, _imageInfo, _localPath));
    tryShowImage(*loader);
    loaders_[middleCachePos].swap(loader);
    connectLoader();
}

void Previewer::GalleryWidget::downloadRelativeImage(int _n)
{
    if (!imageIterator_ || !imageIterator_->hasRelative(_n))
        return;

    const auto& image = imageIterator_->peekImageRelative(_n);
    auto loader = std::unique_ptr<ImageLoader>(new ImageLoader(aimId_, image));
    loaders_[middleCachePos + _n].swap(loader);
}

Previewer::ImageLoader& Previewer::GalleryWidget::currentLoader() const
{
    assert(loaders_[middleCachePos]);
    return *loaders_[middleCachePos];
}

void Previewer::GalleryWidget::connectLoader()
{
    auto loader = &currentLoader();
    connect(loader, &ImageLoader::imageLoaded, this, &GalleryWidget::onImageLoaded);
    connect(loader, &ImageLoader::imageLoadingError, this, &GalleryWidget::onImageLoadingError);
    connect(loader, &ImageLoader::imageLoadingError, download_, &DownloadWidget::onDownloadingError);
    connect(download_, &DownloadWidget::cancelDownloading, loader, &ImageLoader::onCancelLoading);
    connect(download_, &DownloadWidget::tryDownloadAgain, loader, &ImageLoader::onTryAgain);
}

void Previewer::GalleryWidget::disconnectLoader()
{
    disconnect(&currentLoader(), 0, 0, 0);
    disconnect(download_, 0, 0, 0);
}

bool Previewer::GalleryWidget::hasPrev() const
{
    return initComplete_ && imageIterator_->hasPrev();
}

void Previewer::GalleryWidget::prev()
{
    assert(hasPrev());

    showProgress();

    disconnectLoader();

    imageIterator_->prev();

    if (loaders_.back())
        loaders_.back()->cancelLoading();
    loaders_.pop_back();
    loaders_.push_front(std::unique_ptr<ImageLoader>());

    downloadRelativeImage(-middleCachePos);

    connectLoader();

    updateButtonFrame();
}

bool Previewer::GalleryWidget::hasNext() const
{
    return initComplete_ && imageIterator_->hasNext();
}

void Previewer::GalleryWidget::next()
{
    assert(hasNext());

    showProgress();

    disconnectLoader();

    imageIterator_->next();

    if (loaders_.front())
        loaders_.front()->cancelLoading();
    loaders_.pop_front();
    loaders_.push_back(std::unique_ptr<ImageLoader>());

    downloadRelativeImage(middleCachePos);

    connectLoader();

    updateButtonFrame();
}

void Previewer::GalleryWidget::startLoading()
{
    for (auto& loader : loaders_)
    {
        if (loader)
        {
            const auto state = loader->getState();
            if (state == ImageLoader::State::ReadyToLoad
                || state == ImageLoader::State::Error)
            {
                loader->load();
            }
        }
    }

    tryShowImage(currentLoader());
}

void Previewer::GalleryWidget::tryShowImage(ImageLoader& _loader)
{
    if (_loader.getState() == ImageLoader::State::Success)
    {
        showImage(_loader.getPixmap(), _loader.getLocalFileName());
    }
}
