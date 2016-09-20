#include "stdafx.h"
#include "VideoPlayer.h"

#include "../../utils/utils.h"
#include "../../gui_settings.h"

#include "FFMpegPlayer.h"



namespace Ui
{
    int getControlPanelMaxHeight()
    {
        return (int) Utils::scale_value(104);
    }

    int getTimeWidth()
    {
        return (int) Utils::scale_value(70 - 16);
    }

    int getMargin()
    {
        return (int) Utils::scale_value(16);
    }

    int getVolumeSliderWidth()
    {
        return (int) Utils::scale_value(70);
    }

    int getVolumeSliderHeight()
    {
        return (int) Utils::scale_value(24);
    }

    int getHeaderMaxHeight()
    {
        return (int) Utils::scale_value(32);
    }

    double getMinWidth()
    {
        return Utils::scale_value(360.0);
    }

    double getMinHeight()
    {
        return Utils::scale_value(300.0);
    }

    double getMinCenteredWidht()
    {
        return Utils::scale_value(500.0);
    }

    //////////////////////////////////////////////////////////////////////////
    // VideoProgressSlider
    //////////////////////////////////////////////////////////////////////////
    VideoProgressSlider::VideoProgressSlider(Qt::Orientation _orientation, QWidget* _parent = 0)
        : QSlider(_orientation, _parent)
    {

    }

    void VideoProgressSlider::mousePressEvent(QMouseEvent* _event)
    {
        int32_t sliderWidth = width();

        if (_event->button() == Qt::LeftButton)
        {
            int32_t value = 0;

            if (orientation() == Qt::Vertical)
            {
                value = minimum() + ((maximum() - minimum()) * (height() - _event->y())) / height();
            }
            else
            {
                value = minimum() + ((maximum() - minimum()) * _event->x()) / sliderWidth;
            }

            setValue(value);

            _event->accept();

            emit sliderMoved(value);
        }

        QSlider::mousePressEvent(_event);
    }

    //////////////////////////////////////////////////////////////////////////
    // VideoPlayerHeader
    //////////////////////////////////////////////////////////////////////////
    VideoPlayerHeader::VideoPlayerHeader(QWidget* _parent, const QString& _caption)
        : QWidget(_parent)
    {

        QHBoxLayout* rootLayout = new QHBoxLayout();
        rootLayout->setContentsMargins(Utils::scale_value(12), 0, 0, 0);

        caption_ = new QLabel(this);
        caption_->setObjectName("VideoPlayerCaption");
        caption_->setText(_caption);

        rootLayout->addWidget(caption_);

        rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

        closeButton_ = new QPushButton(this);
        closeButton_->setObjectName("VideoPlayerCloseButton");
        closeButton_->setFixedHeight(Utils::scale_value(32));
        closeButton_->setFixedWidth(Utils::scale_value(48));
        closeButton_->setCursor(Qt::PointingHandCursor);
        rootLayout->addWidget(closeButton_);

        setLayout(rootLayout);

        connect(closeButton_, &QPushButton::clicked, this, [this](bool)
        {
            emit signalClose();
        });
    }

    VideoPlayerHeader::~VideoPlayerHeader()
    {

    }

    void VideoPlayerHeader::setCaption(const QString& _caption)
    {
        caption_->setText(_caption);
    }

    void VideoPlayerHeader::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }







    //////////////////////////////////////////////////////////////////////////
    // VideoPlayerControlPanel
    //////////////////////////////////////////////////////////////////////////
    VideoPlayerControlPanel::VideoPlayerControlPanel(QWidget* _parent, FFMpegPlayer* _player)
        :   QWidget(_parent),
            player_(_player),
            duration_(0),
            position_(0),
            positionSliderTimer_(new QTimer(this))
    {
        bool mute = get_gui_settings()->get_value<bool>(setting_mplayer_mute, false);
        int32_t volume = get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

        player_->setVolume(volume);
        player_->setMute(mute);

        QHBoxLayout* rootLayout = new QHBoxLayout();
        rootLayout->setContentsMargins(getMargin(), Utils::scale_value(6), getMargin(), getMargin());
        rootLayout->setAlignment(Qt::AlignTop);

        QWidget* timeLeftWidget = new QWidget(this);
        timeLeftWidget->setFixedWidth(getTimeWidth());
        QVBoxLayout* timeLeftLayout = new QVBoxLayout();
        timeLeftLayout->setContentsMargins(0, Utils::scale_value(10), 0, 0);
        timeLeftLayout->setSpacing(0);

        {
            timeLeft_ = new QLabel(this);
            timeLeft_->setObjectName("VideoTimeProgress");
            timeLeft_->setFixedWidth(getTimeWidth());
            timeLeft_->setText("00:00:00");
            timeLeft_->setAlignment(Qt::AlignLeft);
            timeLeftLayout->addWidget(timeLeft_);
            timeLeftWidget->setLayout(timeLeftLayout);
        }

        rootLayout->addWidget(timeLeftWidget);

        QWidget* sliderWidget = new QWidget(this);
        rootLayout->addWidget(sliderWidget);

        {
            QVBoxLayout* sliderLayout = new QVBoxLayout();
            sliderLayout->setContentsMargins(0, 0, 0, 0);
            sliderLayout->setSpacing(0);


            progressSlider_ = new VideoProgressSlider(Qt::Orientation::Horizontal, this);
            progressSlider_->setObjectName("VideoProgressSlider");
            progressSlider_->setFixedHeight(Utils::scale_value(24));
            progressSlider_->setCursor(Qt::PointingHandCursor);

            sliderLayout->addWidget(progressSlider_);

            {
                QHBoxLayout* buttonsLayout = new QHBoxLayout();
                buttonsLayout->setContentsMargins(0, 0, 0, 0);
                buttonsLayout->setSpacing(Utils::scale_value(24));
                buttonsLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

                volumeSpacer_ = new QSpacerItem(0, 0);
                buttonsLayout->addSpacerItem(volumeSpacer_);

                playButton_ = new QPushButton(this);
                playButton_->setObjectName("VideoPlayerPlayButton");
                playButton_->setFixedHeight(Utils::scale_value(40));
                playButton_->setFixedWidth(Utils::scale_value(40));
                playButton_->setCursor(Qt::PointingHandCursor);
                playButton_->setCheckable(true);

                buttonsLayout->addWidget(playButton_);

                const int soundButtonVolumeSpacing = Utils::scale_value(24);

                soundWidget_ = new QWidget(this);
                soundWidget_->setFixedHeight(Utils::scale_value(40));
                soundWidget_->setFixedWidth(Utils::scale_value(40) + getVolumeSliderWidth() + soundButtonVolumeSpacing);
                QHBoxLayout* soundLayout = new QHBoxLayout();
                soundLayout->setContentsMargins(0, 0, 0, 0);
                soundLayout->setSpacing(0);
                soundWidget_->setLayout(soundLayout);
                soundWidget_->installEventFilter(this);
                buttonsLayout->addWidget(soundWidget_);
                {
                    soundButton_ = new QPushButton(this);
                    soundButton_->setObjectName("VideoPlayerSoundButton");
                    soundButton_->setFixedHeight(Utils::scale_value(40));
                    soundButton_->setFixedWidth(Utils::scale_value(40));
                    soundButton_->setCursor(Qt::PointingHandCursor);
                    soundButton_->setCheckable(true);
                    soundButton_->setChecked(!mute);
                    soundButton_->installEventFilter(this);
                    
                    soundLayout->addWidget(soundButton_);

                    soundLayout->addSpacing(soundButtonVolumeSpacing);
                        
                    QWidget* volumeContainer = new QWidget(this);
                    volumeContainer->setFixedWidth(getVolumeSliderWidth());
                    soundLayout->addWidget(volumeContainer);
                    {
                        QHBoxLayout* volumeLayout = new QHBoxLayout();
                        volumeLayout->setContentsMargins(0, 0, 0, 0);
                        volumeLayout->setSpacing(0);
                        volumeContainer->setLayout(volumeLayout);

                        volumeSlider_ = new QSlider(this);
                        volumeSlider_->setOrientation(Qt::Orientation::Horizontal);
                        volumeSlider_->setObjectName("VideoVolumeSlider");
                        volumeSlider_->setMinimum(0);
                        volumeSlider_->setMaximum(100);
                        volumeSlider_->setPageStep(10);
                        volumeSlider_->setFixedHeight(getVolumeSliderHeight());
                        volumeSlider_->setCursor(Qt::PointingHandCursor);
                        volumeSlider_->setSliderPosition(volume);
                        volumeSlider_->setFixedWidth(getVolumeSliderWidth());
                        volumeSlider_->hide();
                        volumeLayout->addWidget(volumeSlider_);
                    }
                }
                                
                sliderLayout->addLayout(buttonsLayout);
            }

            sliderWidget->setLayout(sliderLayout);
        }

        QWidget* timeRightWidget = new QWidget(this);
        timeRightWidget->setFixedWidth(getTimeWidth());
        QVBoxLayout* timeRightLayout = new QVBoxLayout();
        timeRightLayout->setContentsMargins(0, Utils::scale_value(10), 0, 0);
        timeRightLayout->setSpacing(0);
        timeRightWidget->setLayout(timeRightLayout);
        {
            timeRight_ = new QLabel(this);
            timeRight_->setObjectName("VideoTimeProgress");
            timeRight_->setText("00:00:00");
            timeRight_->setFixedWidth(getTimeWidth());
            timeRight_->setAlignment(Qt::AlignRight);
            timeRightLayout->addWidget(timeRight_);

            timeRightLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

            QHBoxLayout* fsLayout = new QHBoxLayout();
            fsLayout->setContentsMargins(0, 0, 0, 0);
            fsLayout->setAlignment(Qt::AlignRight);
            fullscreenButton_ = new QPushButton(this);
            fullscreenButton_->setObjectName("VideoPlayerFullscreenButton");
            fullscreenButton_->setFixedHeight(Utils::scale_value(40));
            fullscreenButton_->setFixedWidth(Utils::scale_value(40));
            fullscreenButton_->setCursor(Qt::PointingHandCursor);
            fullscreenButton_->setCheckable(true);
            
            fsLayout->addWidget(fullscreenButton_);

            timeRightLayout->addLayout(fsLayout);
        }

        rootLayout->addWidget(timeRightWidget);

        setLayout(rootLayout);

        connectSignals(_player);
    }
    
    VideoPlayerControlPanel::~VideoPlayerControlPanel()
    {

    }

    void VideoPlayerControlPanel::showVolumeControl(const bool _isShow)
    {
        if (!_isShow)
        {
            volumeSlider_->hide();
        }
        else
        {
            volumeSlider_->show();
        }
    }

    void VideoPlayerControlPanel::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    bool VideoPlayerControlPanel::eventFilter(QObject* _obj, QEvent* _event)
    {
        QObject* objectSoundButton = qobject_cast<QObject*>(soundButton_);
        QObject* objectSoundWidget = qobject_cast<QObject*>(soundWidget_);

        if (objectSoundButton == _obj && QEvent::Enter == _event->type())
        {
            showVolumeControl(true);
        }
        else if (objectSoundWidget == _obj && QEvent::Leave == _event->type())
        {
            showVolumeControl(false);
        }

        return QObject::eventFilter(_obj, _event);
    }

    void VideoPlayerControlPanel::mouseDoubleClickEvent(QMouseEvent* /*_event*/)
    {

    }

    void VideoPlayerControlPanel::mousePressEvent(QMouseEvent* /*_event*/)
    {

    }

    void VideoPlayerControlPanel::resizeEvent(QResizeEvent* _event)
    {
        if (_event->size().width() < getMinCenteredWidht())
        {
            volumeSpacer_->changeSize(0, 0);
        }
        else
        {
            volumeSpacer_->changeSize(getVolumeSliderWidth(), 0);
        }
    }

    QString getDurationString(const qint64& _duration)
    {
        const qint64 one_hour = (1000 * 60 * 60);
        const qint64 one_minute = (1000 * 60);
        const qint64 one_sec = (1000);

        qint64 hours = _duration / one_hour;
        qint64 minutes = (_duration - hours * one_hour) / one_minute;
        qint64 seconds = (_duration - hours * one_hour - minutes * one_minute) / one_sec;

        QString sout;
        sout.sprintf("%02d:%02d:%02d", (int) hours, (int) minutes, (int) seconds);

        return sout;
    }

    void VideoPlayerControlPanel::connectSignals(FFMpegPlayer* _player)
    {
        connect(_player, &FFMpegPlayer::durationChanged, this, [this](qint64 _duration)
        {
            duration_ = _duration;

            progressSlider_->setMinimum(0);
            progressSlider_->setMaximum((int) _duration);
            progressSlider_->setPageStep(_duration/10);

            timeLeft_->setText(getDurationString(0));
            timeRight_->setText(getDurationString(_duration));
        });

        connect(_player, &FFMpegPlayer::positionChanged, this, [this](qint64 _position)
        {
            position_ = _position;

            if (!progressSlider_->isSliderDown())
                progressSlider_->setSliderPosition((int) _position);

            timeLeft_->setText(getDurationString(position_));
            timeRight_->setText(getDurationString(duration_ - position_));
        });

        connect(progressSlider_, &QSlider::sliderMoved, this, [this](int /*_value*/)
        {
            player_->setPosition(progressSlider_->value());

            positionSliderTimer_->stop();
        });


        connect(playButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            if (_checked)
            {
                player_->pause();
            }
            else
            {
                player_->play();
            }
        });

         connect(volumeSlider_, &QSlider::valueChanged, this, [this](int _value)
         {
             player_->setVolume(_value);
             get_gui_settings()->set_value<int32_t>(setting_mplayer_volume, _value);
         });

        connect(soundButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            player_->setMute(!_checked);
            get_gui_settings()->set_value<bool>(setting_mplayer_mute, !_checked);
        });

        connect(fullscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            emit signalFullscreen(_checked);
        });

        connect(_player, &FFMpegPlayer::mediaFinished, this, [this]()
        {
            playButton_->setChecked(true);
            progressSlider_->setValue(0);
        });



    }

    bool VideoPlayerControlPanel::isFullscreen() const
    {
        return fullscreenButton_->isChecked();
    }

    void VideoPlayerControlPanel::setFullscreen(const bool _fullScreen)
    {
        fullscreenButton_->setChecked(_fullScreen);
    }

    bool VideoPlayerControlPanel::isPause() const
    {
        return playButton_->isChecked();
    }

    void VideoPlayerControlPanel::setPause(const bool& _pause)
    {
        playButton_->setChecked(_pause);
    }


    
    

    VideoPlayer::VideoPlayer(QWidget* _parent)
        :   QWidget(_parent),
            animControlPanel_(nullptr),
            animHeader_(nullptr),
            parent_(_parent),
            mediaLoaded_(false)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/mplayer/mstyles.qss"));
        
        setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        QVBoxLayout* rootLayout = new QVBoxLayout();
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        player_ = new FFMpegPlayer(this);

        rootLayout->addWidget(player_);

        controlPanel_ = new VideoPlayerControlPanel(this, player_);
        header_ = new VideoPlayerHeader(this, "");

        connect(header_, &VideoPlayerHeader::signalClose, this, [this]()
        {
            emit signalClose();
        });

        installEventFilter(this);

        connect(controlPanel_, &VideoPlayerControlPanel::signalFullscreen, this, &VideoPlayer::fullScreen);

        setMouseTracking(true);

        updateSize(QSize(getMinWidth(), getMinHeight()));
    }

    VideoPlayer::~VideoPlayer()
    {
    }

    void VideoPlayer::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void VideoPlayer::changeEvent(QEvent* _e)
    {
        if (QEvent::ActivationChange == _e->type())
        {
            if (!isActiveWindow())
            {
                //emit signalClose();
            }
        }
    }

    void VideoPlayer::mouseDoubleClickEvent(QMouseEvent* /*_event*/)
    {
        bool isFullScreen = controlPanel_->isFullscreen();

        fullScreen(!isFullScreen);

        controlPanel_->setFullscreen(!isFullScreen);
    }

    void VideoPlayer::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (!rect().contains(_event->pos()))
            return QWidget::mouseReleaseEvent(_event);

        bool isPause = controlPanel_->isPause();

        if (isPause)
        {
            player_->play();
        }
        else
        {
            player_->pause();
        }

        controlPanel_->setPause(!isPause);
    }

    void VideoPlayer::mousePressEvent(QMouseEvent* /*_event*/)
    {

    }

    const QSize VideoPlayer::calculatePlayerSize(const QSize& _videoSize)
    {
        const QSize _scaledVideoSize(Utils::scale_value(_videoSize.width()), Utils::scale_value(_videoSize.height()));

        const double minWidth = getMinWidth();
        const double minHeight = getMinHeight();

        const QRect parentRect = parent_->geometry();

        const double maxWidth = ((double) parentRect.width()) * 0.6;
        const double maxHeight = ((double) parentRect.height()) * 0.7;

        QSize outSize(_videoSize);


        if (_videoSize.width() < _videoSize.height())
        {
            if (_scaledVideoSize.height() > maxHeight)
            {
                outSize.setHeight(maxHeight);
                outSize.setWidth(_scaledVideoSize.width()*(maxHeight/_scaledVideoSize.height()));
            }
        }
        else
        {
            if (_scaledVideoSize.width() > maxWidth)
            {
                outSize.setWidth(maxWidth);
                outSize.setHeight(_scaledVideoSize.height()*(maxWidth/_scaledVideoSize.width()));

            }
        }

        if (outSize.width() < minWidth)
            outSize.setWidth(minWidth);

        if (outSize.height() < minHeight)
            outSize.setHeight(minHeight);

        return outSize;
    }

    void VideoPlayer::updateSize(const QSize& _sz)
    {
        setFixedWidth(_sz.width());
        setFixedHeight(_sz.height());

        const auto rcParent = parent_->geometry();

        int cw = (rcParent.width()/2) - (_sz.width()/2);
        int ch = (rcParent.height()/2) - (_sz.height()/2);

        move(rcParent.left() + cw, rcParent.top() + ch);

        player_->setFixedSize(_sz);

        controlPanel_->setGeometry(0, _sz.height() - getControlPanelMaxHeight(), _sz.width(), getControlPanelMaxHeight());
        header_->setGeometry(0, 0, _sz.width(), getHeaderMaxHeight());

        if (underMouse())
        {
            showControlPanel(true);
            showHeader(true);
        }
        else
        {
            showControlPanel(false);
            showHeader(false);
        }

        emit sizeChanged(_sz);
    }

    void VideoPlayer::play(const QString& _path)
    {
        mediaPath_ = QDir::fromNativeSeparators(_path);

        if (player_->openMedia(mediaPath_))
        {
            playerSize_ = calculatePlayerSize(player_->getVideoSize());

            updateSize(playerSize_);

            header_->setCaption(QFileInfo(_path).fileName());

            player_->play();
        }
    }



    bool VideoPlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        switch (_event->type())
        {
            case QEvent::Enter:
            {
                showControlPanel(true);
                showHeader(true);
                break;
            }
            case QEvent::Leave:
            {
                showControlPanel(false);
                showHeader(false);
                break;
            }
        }


        return QObject::eventFilter(_obj, _event);
    }

    void VideoPlayer::setControlPanelHeight(int _val)
    {
        controlPanel_->setGeometry(0, geometry().height() - _val, geometry().width(), getControlPanelMaxHeight());
    }

    int VideoPlayer::getControlPanelHeight() const
    {
        return controlPanel_->height();
    }

    void VideoPlayer::setHeaderHeight(int _val)
    {
        header_->setGeometry(0, _val - getHeaderMaxHeight(), geometry().width(), _val);
    }

    int VideoPlayer::getHeaderHeight() const
    {
        return header_->height();
    }



    void VideoPlayer::showHeader(const bool _isShow)
    {
        if (!animHeader_)
        {
            animHeader_ = new QPropertyAnimation(this, "headerHeight");
        }

        int startValue = _isShow ? 0 : getHeaderMaxHeight();
        int endValue = _isShow ? getHeaderMaxHeight() : 0;

        QEasingCurve easingCurve = QEasingCurve::InQuad;
        int duration = 200;

        animHeader_->stop();
        animHeader_->setDuration(duration);
        animHeader_->setStartValue(startValue);
        animHeader_->setEndValue(endValue);
        animHeader_->setEasingCurve(easingCurve);
        animHeader_->start();
    }

    void VideoPlayer::showControlPanel(const bool _isShow)
    {
        if (!animControlPanel_)
        {
            animControlPanel_ = new QPropertyAnimation(this, "controlPanelHeight");
        }


        int startValue = _isShow ? 0 : getControlPanelMaxHeight();
        int endValue = _isShow ? getControlPanelMaxHeight() : 0;

        QEasingCurve easingCurve = QEasingCurve::InQuad;
        int duration = 200;

        animControlPanel_->stop();
        animControlPanel_->setDuration(duration);
        animControlPanel_->setStartValue(startValue);
        animControlPanel_->setEndValue(endValue);
        animControlPanel_->setEasingCurve(easingCurve);
        animControlPanel_->start();
    }

    void VideoPlayer::fullScreen(bool _checked)
    {
        if (_checked)
        {
            const auto rc = parent_->geometry();

            updateSize(QSize(rc.width(), rc.height()));
        }
        else
        {
            updateSize(playerSize_);
        }
        
    }
}
