#include "stdafx.h"
#include "VideoPanel.h"

#include "DetachedVideoWnd.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../controls/ToolTipEx.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"

#define DISPLAY_ADD_CHAT_BUTTON 0
#define DEFAULT_ANIMATION_TIME  500

const QString vertSoundBg = "QWidget { background : #ffffffff; }";
const QString horSoundBg = "QWidget { background : rgba(0,0,0,1%); }";

const QString sliderGreenH =
    "QSlider:handle:horizontal { background: solid #579e1c; width: 8dip; height: 8dip; margin-top: -11dip; margin-bottom: -11dip; border-radius: 4dip; }"
    "QSlider:handle:horizontal:hover { background: solid #67bc21; border-radius: 4dip;}";

const QString sliderRedH =
    "QSlider:handle:horizontal { background: solid #992117; width: 8dip; height: 8dip; margin-top: -11dip; margin-bottom: -11dip; border-radius: 4dip; }"
    "QSlider:handle:horizontal:disabled { background: solid #992117; border-radius: 4dip; }";

const QString sliderGreenV =
    "QSlider:handle:vertical { background: solid #579e1c; border: 1dip solid #579e1c; width: 8dip; height: 8dip; margin-left: -11dip; margin-right: -11dip; border-radius: 4dip; }"
    "QSlider:handle:vertical:hover { background: solid #67bc21; border-radius: 4dip; }";

const QString sliderRedV =
    "QSlider:handle:vertical { background: solid #992117; border: 1dip solid #579e1c; width: 8dip; height: 8dip; margin-left: -11dip; margin-right: -11dip; border-radius: 4dip; }"
    "QSlider:handle:vertical:disabled { background: solid #992117; border-radius: 4dip; }";

Ui::QSliderEx::QSliderEx(
    Qt::Orientation _orientation,
    QWidget* _parent)
    : QSlider(_orientation, _parent)
{
    
}

Ui::QSliderEx::~QSliderEx()
{
    
}

void Ui::QSliderEx::mousePressEvent(QMouseEvent* _ev)
{
    if (_ev->button() == Qt::LeftButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum()-minimum()) * (height()-_ev->y())) / height() ) ;
        else
            setValue(minimum() + ((maximum()-minimum()) * _ev->x()) / width() ) ;

        _ev->accept();
    }
    QSlider::mousePressEvent(_ev);
 }

bool Ui::onUnderMouse(QWidget& _widg)
{
    auto rc = _widg.rect();
    auto pg = _widg.mapToGlobal(rc.topLeft());
    QRect rcg(pg.x(), pg.y(), rc.width(), rc.height());
    return rcg.contains(QCursor::pos());
}

Ui::QPushButtonEx::QPushButtonEx(QWidget* _parent)
    : QPushButton(_parent)
{

}

Ui::QPushButtonEx::~QPushButtonEx()
{

}

void Ui::QPushButtonEx::enterEvent(QEvent* _e)
{
    QPushButton::enterEvent(_e);
    emit onHover();
}

Ui::VolumeControl::VolumeControl
(
    QWidget* _parent, 
    bool _horizontal,
    bool _onMainWindow,
    const QString& _backgroundStyle, 
    const std::function<void(QPushButton&, bool)>& _onChangeStyle)
    : QWidget(_parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint)
    , parent_(_parent)
    , horizontal_(_horizontal)
    , onChangeStyle_(_onChangeStyle)
    , rootWidget_(NULL)
    , audioPlaybackDeviceMuted_(false)
    , background_(_backgroundStyle)
    , checkMousePos_(this)
    , actualVol_(0)
    , onMainWindow_(_onMainWindow)
{
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Utils::LoadStyle(":/voip/volume_control.qss"));

    setContentsMargins(0, 0, 0, 0);

    if (horizontal_)
    {
        setProperty("VolumeControlHor", true);
    }
    else
    {
        setProperty("VolumeControlVert", true);
    }

    btn_ = new QPushButton(this);
    btn_->setCursor(QCursor(Qt::PointingHandCursor));

    slider_ = new QSliderEx(horizontal_ ? Qt::Horizontal : Qt::Vertical, this);
    slider_->setCursor(QCursor(Qt::PointingHandCursor));

    rootWidget_ = new QWidget(this);
    auto widg = rootWidget_;
    widg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    widg->setContentsMargins(0, 0, 0, 0);

    QBoxLayout* layout;
    if (horizontal_)
    {
        layout = new QHBoxLayout();
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else
    {
        layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    }

    layout->setContentsMargins(0, 0, 0, 0);
        
    if (horizontal_)
    {
        layout->addWidget(btn_);
        layout->addSpacing(Utils::scale_value(10));
        layout->addWidget(slider_);
    }
    else
    {
        layout->addSpacing(Utils::scale_value(12));
        layout->addWidget(slider_);
        layout->addSpacing(Utils::scale_value(8));
        layout->addWidget(btn_);
        layout->addSpacing(Utils::scale_value(8));
    }

    widg->setLayout(layout);

    Utils::ApplyStyle(widg, background_);

    if (onChangeStyle_ != NULL && btn_)
    {
        onChangeStyle_(*btn_, true);
    }

    if (horizontal_)
    {
        slider_->setProperty("VolumeSliderHor", true);
        slider_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    }
    else
    {
        slider_->setProperty("VolumeSliderVert", true);
        slider_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    }

    auto horLayout = new QHBoxLayout();
    horLayout->setContentsMargins(0, 0, 0, 0);
    horLayout->addWidget(widg);
    setLayout(horLayout);

    connect(slider_, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
    connect(btn_, SIGNAL(clicked()), this, SLOT(onMuteOnOffClicked()), Qt::QueuedConnection);
    connect(&checkMousePos_, SIGNAL(timeout()), this, SLOT(onCheckMousePos()), Qt::QueuedConnection);
    checkMousePos_.setInterval(500);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMuteChanged(const std::string&,bool)), this, SLOT(onVoipMuteChanged(const std::string&,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVolumeChanged(const std::string&,int)), this, SLOT(onVoipVolumeChanged(const std::string&,int)), Qt::DirectConnection);
}

Ui::VolumeControl::~VolumeControl()
{

}

void Ui::VolumeControl::onCheckMousePos()
{
    if (!onUnderMouse(*this))
    {
        hide();
    }
}

QPoint Ui::VolumeControl::getAnchorPoint() const
{
    const auto rcb = btn_->rect();
    const auto rcc = rect();

    if (horizontal_)
    {
        return QPoint(0, (rcc.height() - rcb.height()) / 2);
    }
    else
    {
        return QPoint((rcc.width() - rcb.width()) / 2, rcc.height() - rcb.height() - Utils::scale_value(8));
    }
}

void Ui::VolumeControl::updateSlider()
{
    if (horizontal_)
    {
        if (audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f)
        {
            Utils::ApplyStyle(slider_, sliderRedH);
        }
        else
        {
            Utils::ApplyStyle(slider_, sliderGreenH);
        }
    }
    else
    {
        if (audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f)
        {
            Utils::ApplyStyle(slider_, sliderRedV);
        }
        else
        {
            Utils::ApplyStyle(slider_, sliderGreenV);
        }
    }
}

void Ui::VolumeControl::onVoipMuteChanged(const std::string& _deviceType, bool _muted)
{
    if (_deviceType == "audio_playback")
    {
        slider_->setEnabled(!_muted);
        //slider_->setVisible(!_muted);

        audioPlaybackDeviceMuted_ = _muted;
        updateSlider();

        if (onChangeStyle_ != NULL && btn_)
        {
            onChangeStyle_(*btn_, _muted || actualVol_ <= 0.0001f);
        }
        emit onMuteChanged(_muted || actualVol_ <= 0.0001f);
    }
}

void Ui::VolumeControl::onVolumeChanged(int _vol)
{
    const int newVol = std::max(std::min(100, _vol), 0);
    if (actualVol_ != newVol)
    {
        Ui::GetDispatcher()->getVoipController().setVolumeAPlayback(newVol);
    }
}

void Ui::VolumeControl::onMuteOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
}

void Ui::VolumeControl::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
{
    if (_deviceType == "audio_playback")
    {
        actualVol_ = std::max(std::min(100, _vol), 0);

        if (onChangeStyle_ != NULL && btn_)
        {
            onChangeStyle_(*btn_, audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f);
        }
        emit onMuteChanged(audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f);

        slider_->blockSignals(true);
        slider_->setValue(actualVol_);
        slider_->blockSignals(false);

        updateSlider();
    }
}

void Ui::VolumeControl::showEvent(QShowEvent* _e)
{
#ifdef __APPLE__
    // Fix blikning of volume button.
    if (onMainWindow_)
    {
        btn_->setAttribute(Qt::WA_UnderMouse);
    }
#endif
    QWidget::showEvent(_e);
    emit controlActivated(true);
    checkMousePos_.start();
}

void Ui::VolumeControl::hideEvent(QHideEvent* _e)
{
    checkMousePos_.stop();
    QWidget::hideEvent(_e);
    emit controlActivated(false);
}

void Ui::VolumeControl::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    hide();
}

void Ui::VolumeControl::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::WindowDeactivate)
    {
        hide();
    }
}

#define internal_spacer_w  (Utils::scale_value(24))
#define internal_spacer_w4 (Utils::scale_value(16))
#define internal_spacer_w_small (Utils::scale_value(12))

Ui::VideoPanel::VideoPanel(
    QWidget* _parent, QWidget* _container)
    : QWidget(_parent, Qt::Window | Qt::FramelessWindowHint)
    , container_(_container)
    , rootWidget_(NULL)
    , parent_(_parent)
    , mouseUnderPanel_(false)
    , vertVolControl_(
        this, false, false, vertSoundBg, [] (QPushButton& _btn, bool _muted)
    {
        _btn.setProperty("CallPanelEnBtn", !_muted);
        _btn.setProperty("CallPanelDisBtn", _muted);
        _btn.setProperty("CallSoundOn", !_muted);
        _btn.setProperty("CallSoundOff", _muted);
        _btn.setStyle(QApplication::style());
    })
    , horVolControl_(
        this, true, false, horSoundBg, [] (QPushButton& _btn, bool _muted)
    {
        _btn.setProperty("CallPanelEnBtn", !_muted);
        _btn.setProperty("CallPanelDisBtn", _muted);
        _btn.setProperty("CallSoundOn", !_muted);
        _btn.setProperty("CallSoundOff", _muted);
        _btn.setStyle(QApplication::style());
    })
        , fullScreenButton_(NULL)
        , addChatButton_(NULL)
        , settingsButton_(NULL)
        , stopCallButton_(NULL)
        , videoButton_(NULL)
        , micButton_(NULL)
        , soundOnOffButton_(NULL) 
        , minimalBandwidthMode_(nullptr)
        , minimalBandwidthTooltip_(nullptr)
{
    setStyleSheet(Utils::LoadStyle(":/voip/video_panel.qss"));
    setProperty("VideoPanel", true);
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);  

    QVBoxLayout* rootLayout = new QVBoxLayout();
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("VideoPanel", true);
    layout()->addWidget(rootWidget_);
    
    QHBoxLayout* layoutTarget = new QHBoxLayout();
    layoutTarget->setContentsMargins(0, 0, 0, 0);
    layoutTarget->setSpacing(0);
    layoutTarget->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layoutTarget);

    QWidget* parentWidget = rootWidget_;
    auto addButton = [this, parentWidget, layoutTarget] (const char* _propertyName, const char* _slot)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);
        if (_propertyName != NULL)
        {
            btn->setProperty(_propertyName, true);
        }
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        layoutTarget->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        return btn;
    };

    QPushButton* btn = NULL;
    layoutTarget->addSpacing(internal_spacer_w4);
    btn = addButton("CallGoChat", SLOT(onClickGoChat()));
    layoutTarget->addSpacing(internal_spacer_w);

    minimalBandwidthMode_ = addButton("MinimalBandwithMode", SLOT(onMinimalBandwithMode()));
    minimalBandwidthMode_->setProperty("MinimalBandwithMode", false);
    minimalBandwidthMode_->hide(); // We will show it on call accepted.

	leftSpacer_ = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding);
    layoutTarget->addSpacerItem(leftSpacer_);
    micButton_ = addButton("CallEnableMic", SLOT(onAudioOnOffClicked()));
    micButton_->setProperty("CallDisableMic", false);

    layoutTarget->addSpacing(internal_spacer_w);
    videoButton_ = addButton(NULL, SLOT(onVideoOnOffClicked()));

    layoutTarget->addSpacing(internal_spacer_w);
    stopCallButton_ = addButton("StopCallButton", SLOT(onHangUpButtonClicked()));

    if (DISPLAY_ADD_CHAT_BUTTON)
    {
        layoutTarget->addSpacing(internal_spacer_w);
        addChatButton_ = addButton("CallAddChat", SLOT(onClickAddChat()));
    }

    layoutTarget->addSpacing(internal_spacer_w);
    soundOnOffButton_ = new voipTools::BoundBox<QPushButtonEx>(parentWidget);
    soundOnOffButton_->setCursor(QCursor(Qt::PointingHandCursor));
    soundOnOffButton_->setProperty("CallPanelEnBtn", true);
    soundOnOffButton_->setProperty("CallSoundOn", true);
    soundOnOffButton_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    soundOnOffButton_->setFlat(true);
    // This button has no focus, because we show VolumeControl above this button.
    soundOnOffButton_->setFocusPolicy(Qt::NoFocus);
    connect(soundOnOffButton_, SIGNAL(onHover()), this, SLOT(onSoundOnOffHover()), Qt::QueuedConnection);
    layoutTarget->addWidget(soundOnOffButton_);

	rightSpacer_ = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding);
    layoutTarget->addSpacerItem(rightSpacer_);
    settingsButton_ = addButton("CallSettings", SLOT(onClickSettings()));

    layoutTarget->addSpacing(internal_spacer_w);
    fullScreenButton_ = addButton(NULL, SLOT(_onFullscreenClicked()));

    layoutTarget->addSpacing(internal_spacer_w4);

    minimalBandwidthTooltip_ = new Ui::ToolTipEx(minimalBandwidthMode_);

    resetHangupText();
    connect(&vertVolControl_, SIGNAL(controlActivated(bool)), this, SLOT(controlActivated(bool)), Qt::QueuedConnection);
    connect(&horVolControl_, SIGNAL(controlActivated(bool)), this, SLOT(controlActivated(bool)), Qt::QueuedConnection);
    connect(&horVolControl_, SIGNAL(onMuteChanged(bool)), this,    SLOT(onMuteChanged(bool)),    Qt::QueuedConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);

    horVolControl_.hide();
    vertVolControl_.hide();

    videoPanelEffect_ = new UIEffects(*this);
    minimalBandwidthTooltipEffect_ = new UIEffects(*minimalBandwidthTooltip_);

    hideBandwidthTooltipTimer = new QTimer();
    hideBandwidthTooltipTimer->setInterval(3000);
    connect(hideBandwidthTooltipTimer, SIGNAL(timeout()), this, SLOT(hideBandwidthTooltip()));

    hideButtonList.push_back(minimalBandwidthMode_);
    hideButtonList.push_back(settingsButton_);
}

Ui::VideoPanel::~VideoPanel()
{

}

void Ui::VideoPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

void Ui::VideoPanel::controlActivated(bool _activated)
{
    mouseUnderPanel_ = _activated;
    if (onUnderMouse(*this))
    { // this prevents multiple signalling of mouse enter/mouse leave
        return;
    }

    if (_activated)
    {
        emit onMouseEnter();
    }
    else
    {
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::onClickSettings()
{
    if (MainPage* mainPage = MainPage::instance())
    {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
        }
        mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }
}

void Ui::VideoPanel::onClickGoChat()
{
    if (MainPage* mainPage = MainPage::instance())
    {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
        }
    }

    if (!activeContact_.empty())
    {
        Logic::getContactListModel()->setCurrent(activeContact_.c_str(), true, true);
    }
}

void Ui::VideoPanel::onMinimalBandwithMode()
{
    Ui::GetDispatcher()->getVoipController().switchMinimalBandwithMode();

    minimalBandwidthMode_->setProperty("MinimalBandwithMode", !minimalBandwidthMode_->property("MinimalBandwithMode").toBool());
    minimalBandwidthMode_->setStyle(QApplication::style());
}

void Ui::VideoPanel::onVoipMinimalBandwidthChanged (bool _bEnable)
{
    bool youTurnOn = minimalBandwidthMode_->property("MinimalBandwithMode").toBool() == _bEnable;

    minimalBandwidthMode_->setProperty("MinimalBandwithMode", _bEnable);
    minimalBandwidthMode_->setStyle(QApplication::style());

    bool show = false;
    emit showToolTip(show);
    if (show)
    {
        if (_bEnable)
        {
            QString companionFullName;

            emit companionName(companionFullName);

            QString toolTipText = youTurnOn ? QT_TRANSLATE_NOOP("voip_pages", "Data saving enabled") : companionFullName + QT_TRANSLATE_NOOP("voip_pages", " enabled data saving");
            minimalBandwidthTooltip_->setText(toolTipText);
            minimalBandwidthTooltip_->show();
            if (!youTurnOn)
            {
                emit showPanel();
            }

            bool autoHide = false;
            emit autoHideToolTip(autoHide);

            if (autoHide)
            {
                startToolTipHideTimer();
            }
        }
        else
        {
            minimalBandwidthTooltip_->hide();
            hideBandwidthTooltipTimer->stop();
        }
    }
}

void Ui::VideoPanel::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& _contacts)
{
    if(_contacts.empty())
    {
        return;
    }
    activeContact_ = _contacts[0].contact;
}

void Ui::VideoPanel::onClickAddChat()
{
    assert(false);
}

void Ui::VideoPanel::onSoundOnOffHover()
{
    const auto rc = rect();

    VolumeControl* vc;
    int xOffset = 0, yOffset = 0;
    
    if (rc.width() >= Utils::scale_value(660))
    {
        vc = &horVolControl_;
#ifdef __APPLE__
        //xOffset = Utils::scale_value(6); // i don't know where appeared this offsets
        yOffset = Utils::scale_value(1);
#endif
    }
    else
    {
        vc = &vertVolControl_;
#ifdef __APPLE__
        //yOffset = Utils::scale_value(-8);
        xOffset = Utils::scale_value(1);
#endif
    }

    auto p = vc->getAnchorPoint();
    auto p2 = soundOnOffButton_->mapToGlobal(soundOnOffButton_->rect().topLeft());
    
    p2.setX(p2.x() - p.x() + xOffset);
    p2.setY(p2.y() - p.y() + yOffset);

    vc->move(p2);
    vc->show();
    vc->activateWindow();
#ifdef __APPLE__
    vc->raise();
#endif
    vc->setFocus(Qt::OtherFocusReason);
}

void Ui::VideoPanel::onMuteChanged(bool _muted)
{
    if (soundOnOffButton_)
    {
        soundOnOffButton_->setProperty("CallPanelEnBtn", !_muted);
        soundOnOffButton_->setProperty("CallPanelDisBtn", _muted);

        soundOnOffButton_->setProperty("CallSoundOn", !_muted);
        soundOnOffButton_->setProperty("CallSoundOff", _muted);
        soundOnOffButton_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanel::setFullscreenMode(bool _en)
{
    if (!fullScreenButton_)
    {
        return;
    }

    fullScreenButton_->setProperty("CallFSOff", _en);
    fullScreenButton_->setProperty("CallFSOn", !_en);
    fullScreenButton_->setStyle(QApplication::style());
}

void Ui::VideoPanel::_onFullscreenClicked()
{
    emit onFullscreenClicked();
}

void Ui::VideoPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    if (!mouseUnderPanel_)
    {
        emit onMouseEnter();
    }
}

void Ui::VideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    const bool focusOnVolumePanel = onUnderMouse(horVolControl_) || onUnderMouse(vertVolControl_);
    if (!focusOnVolumePanel)
    {
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::hideEvent(QHideEvent* _e)
{
    QWidget::hideEvent(_e);
    horVolControl_.hide();
    vertVolControl_.hide();
}

void Ui::VideoPanel::moveEvent(QMoveEvent* _e)
{
    QWidget::moveEvent(_e);
    horVolControl_.hide();
    vertVolControl_.hide();

    updateToolTipsPosition();
}

void Ui::VideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

    bool bVisibleButton = isNormalPanelMode();

	auto rootLayout = qobject_cast<QBoxLayout*>(rootWidget_->layout());

    for (QWidget* button : hideButtonList)
    {
        if (button)
        {
            button->setVisible(bVisibleButton);
        }
    }

	// Change spacers width, if video panel is too small.
	if (rootLayout)
	{
		for (int i = 0; i < rootLayout->count(); i++)
		{
			QLayoutItem * item = rootLayout->itemAt(i);
			bool isCorner = (i == 0 || i == rootLayout->count() - 1);
			if (item)
			{
				QSpacerItem * spacer = item->spacerItem();
				if (spacer && spacer != leftSpacer_ && spacer != rightSpacer_)
				{
					spacer->changeSize(isFitSpacersPanelMode() ? internal_spacer_w_small :
						(isCorner ? internal_spacer_w4 : internal_spacer_w), 1, QSizePolicy::Fixed);
				}
			}
		}

		rootLayout->update();
	}
    
#ifdef __APPLE__
    assert(parent_);
    if (parent_ && !parent_->isFullScreen())
    {
        auto rc = rect();
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), Utils::scale_value(5), Utils::scale_value(5));
        
        QRegion region(path.toFillPolygon().toPolygon());
        region = region + QRect(0, 0, rc.width(), Utils::scale_value(5));
        
        setMask(region);
    }
    else
    {
        clearMask();
    }
#endif

    horVolControl_.hide();
    vertVolControl_.hide();

    updateToolTipsPosition();
}

void Ui::VideoPanel::resetHangupText()
{
    if (stopCallButton_)
    {
        stopCallButton_->setText("");
        stopCallButton_->repaint();
    }
}


void Ui::VideoPanel::onHangUpButtonClicked()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (micButton_)
    {
        micButton_->setProperty("CallPanelEnBtn", _enabled);
        micButton_->setProperty("CallPanelDisBtn", !_enabled);
        micButton_->setProperty("CallEnableMic", _enabled);
        micButton_->setProperty("CallDisableMic", !_enabled);
        micButton_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
    {
        return;
    }
    videoButton_->setProperty("CallPanelEnBtn", _enabled);
    videoButton_->setProperty("CallPanelDisBtn", !_enabled);
    videoButton_->setProperty("CallEnableCam", _enabled);
    videoButton_->setProperty("CallDisableCam", !_enabled);
    videoButton_->setStyle(QApplication::style());
}


void Ui::VideoPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);

    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
        {
            if (container_)
            {
                container_->raise();
                raise();
            }
        }
    }
}

void Ui::VideoPanel::onAudioOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::VideoPanel::onVideoOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::VideoPanel::updateToolTipsPosition()
{
    if (minimalBandwidthTooltip_)
    {
        minimalBandwidthTooltip_->updatePosition();
    }
}

void Ui::VideoPanel::fadeIn(int _kAnimationDefDuration)
{
    videoPanelEffect_->fadeIn(_kAnimationDefDuration);
    minimalBandwidthTooltipEffect_->fadeIn(_kAnimationDefDuration);
}

void Ui::VideoPanel::fadeOut(int _kAnimationDefDuration)
{
    videoPanelEffect_->fadeOut(_kAnimationDefDuration);
    minimalBandwidthTooltipEffect_->fadeOut(_kAnimationDefDuration);
    minimalBandwidthTooltip_->hide();
    hideBandwidthTooltipTimer->stop();
    
    vertVolControl_.hide();
    horVolControl_.hide();
}

void Ui::VideoPanel::hideBandwidthTooltip()
{
    minimalBandwidthTooltip_->hide();
    hideBandwidthTooltipTimer->stop();
}

void Ui::VideoPanel::talkStarted()
{
    minimalBandwidthMode_->setVisible(isNormalPanelMode());
}

void Ui::VideoPanel::talkFinished()
{
    minimalBandwidthMode_->hide();
}

void Ui::VideoPanel::startToolTipHideTimer()
{
    if (minimalBandwidthTooltip_->isVisible())
    {
        hideBandwidthTooltipTimer->start();
    }
}

bool Ui::VideoPanel::isActiveWindow()
{
    return QWidget::isActiveWindow() || vertVolControl_.isActiveWindow() ||
        horVolControl_.isActiveWindow();
}

bool Ui::VideoPanel::isNormalPanelMode()
{
    auto rc = rect();

    return (rc.width() >= Utils::scale_value(436));
}

bool Ui::VideoPanel::isFitSpacersPanelMode()
{
	auto rc = rect();

	return (rc.width() < Utils::scale_value(520));
}