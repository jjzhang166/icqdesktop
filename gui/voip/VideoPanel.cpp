#include "stdafx.h"
#include "VideoPanel.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../utils/utils.h"

#define DISPLAY_ADD_CHAT_BUTTON 0
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "VoipTools.h"

Ui::QSliderEx::QSliderEx(Qt::Orientation orientation, QWidget* parent)
: QSlider(orientation, parent) {
    
}

Ui::QSliderEx::~QSliderEx() {
    
}

void Ui::QSliderEx::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::LeftButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum()-minimum()) * (height()-ev->y())) / height() ) ;
        else
            setValue(minimum() + ((maximum()-minimum()) * ev->x()) / width() ) ;

        ev->accept();
    }
    QSlider::mousePressEvent(ev);
 }

bool Ui::__underMouse(QWidget& widg) {
    auto rc = widg.rect();
    auto pg = widg.mapToGlobal(rc.topLeft());
    QRect rcg(pg.x(), pg.y(), rc.width(), rc.height());
    return rcg.contains(QCursor::pos());
}

Ui::QPushButtonEx::QPushButtonEx(QWidget* parent)
	: QPushButton(parent) {

}

Ui::QPushButtonEx::~QPushButtonEx() {

}

void Ui::QPushButtonEx::enterEvent(QEvent* e) {
	QPushButton::enterEvent(e);
	emit onHover();
}

Ui::VolumeControl::VolumeControl(QWidget* parent, bool horizontal, bool withBackground)
	: QWidget(NULL)
    , _parent(parent)
    , _horizontal(horizontal)
    , _border_offset(Utils::scale_value((50 - 24) / 2) - 1)
	, _actual_vol(0) {

    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
        
    setContentsMargins(0, 0, 0, 0);

    if (horizontal) {
        setProperty("VideoPanelVolumeSliderEx", true);
    } else {
        setProperty("VideoPanelVolumeSliderExV", true);
    }

	btn = new QPushButton(this);
    btn->setCursor(QCursor(Qt::PointingHandCursor));

	slider = new QSliderEx(horizontal ? Qt::Horizontal : Qt::Vertical, this);
    slider->setCursor(QCursor(Qt::PointingHandCursor));

	auto widg = new QWidget(this);
	widg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    widg->setContentsMargins(0, 0, 0, 0);
	
	QBoxLayout* l;
    if (horizontal) {
        l = new QHBoxLayout();
    	l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    } else {
        l = new QVBoxLayout();
    	l->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    }

	l->setContentsMargins(0, 0, 0, 0);
        
    l->addSpacing(_border_offset);
    if (horizontal) {
        l->addWidget(btn);
        l->addSpacing(Utils::scale_value(12));
        l->addWidget(slider);
    } else {
        l->addWidget(slider);
        l->addSpacing(Utils::scale_value(12));
        l->addWidget(btn);
    }

    l->addSpacing(_border_offset);
    widg->setLayout(l);

    if (!withBackground) {
        widg->setProperty("WidgetWithoutBG", true);
        setProperty("WidgetWithoutBG", true);
    } else {
        widg->setProperty("WidgetWithBG", true);
        setProperty("WidgetWithBG", true);
    }
    
    btn->setProperty("CallPanelEnBtn", true);
	btn->setProperty("CallSoundOn", true);

    if (horizontal) {
        slider->setProperty("VideoPanelVolumeSlider", true);
    	slider->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    } else {
        slider->setProperty("VideoPanelVolumeSliderV", true);
	    slider->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    }

    auto ll = new QHBoxLayout();
    ll->setContentsMargins(0, 0, 0, 0);
    ll->addWidget(widg);
	setLayout(ll);
	
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
	connect(btn, SIGNAL(clicked()), this, SLOT(onMuteOnOffClicked()), Qt::QueuedConnection);

	QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMuteChanged(const std::string&,bool)), this, SLOT(onVoipMuteChanged(const std::string&,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVolumeChanged(const std::string&,int)), this, SLOT(onVoipVolumeChanged(const std::string&,int)), Qt::DirectConnection);
}

Ui::VolumeControl::~VolumeControl() {

}

QPoint Ui::VolumeControl::getAnchorPoint() const {
    const auto rcb = btn->rect();
    const auto rcc = rect();

    if (_horizontal) {
        return QPoint(_border_offset, (rcc.height() - rcb.height()) / 2);
    } else {
        return QPoint((rcc.width() - rcb.width()) / 2, rcc.height() - rcb.height() - _border_offset);
    }
}

void Ui::VolumeControl::onVoipMuteChanged(const std::string& device_type, bool muted) {
    if (device_type == "audio_playback") {
        slider->setEnabled(!muted);

        btn->setProperty("CallPanelEnBtn", !muted);
        btn->setProperty("CallPanelDisBtn", muted);

        btn->setProperty("CallSoundOn", !muted);
        btn->setProperty("CallSoundOff", muted);
        btn->setStyle(QApplication::style());
    }
}

void Ui::VolumeControl::onVolumeChanged(int vol) {
    const int new_vol = std::max(std::min(100, vol), 0);
    if (_actual_vol != new_vol) {
		Ui::GetDispatcher()->getVoipController().setVolumeAPlayback(new_vol);
    }
}

void Ui::VolumeControl::onMuteOnOffClicked() {
	Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
}

void Ui::VolumeControl::onVoipVolumeChanged(const std::string& device_type, int vol) {
    if (device_type == "audio_playback") {
        _actual_vol = std::max(std::min(100, vol), 0);

        slider->blockSignals(true);
        slider->setValue(_actual_vol);
        slider->blockSignals(false);
    }
}

void Ui::VolumeControl::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    emit controlActivated(true);
}

void Ui::VolumeControl::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    emit controlActivated(false);
}

void Ui::VolumeControl::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
	hide();
}

void Ui::VolumeControl::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
    if (e->type() == QEvent::WindowDeactivate) {
        hide();
    }
}

#define internal_spacer_w  (Utils::scale_value(24))
#define internal_spacer_w2 ((internal_spacer_w) / 2)
#define internal_spacer_w3 (Utils::scale_value(18))
#define internal_spacer_w4 (Utils::scale_value(36))

Ui::VideoPanel::VideoPanel(QWidget* parent, QWidget* container)
: QWidget(NULL)
, container_(container)
, root_widget_(NULL)
, _parent(parent)
, mouse_under_panel_(false)
, v_vol_control_(this, false, true)
, h_vol_control_(this, true, false)
, fullscreen_button_(NULL)
, add_chat_button_(NULL)
, settings_button_(NULL)
, stop_call_button_(NULL)
, video_button_(NULL)
, mic_button_(NULL)
, sound_on_off_button_(NULL) {
    setProperty("VideoPanel", true);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);  

    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignVCenter);
    setLayout(root_layout);

    root_widget_ = new QWidget(this);
    root_widget_->setContentsMargins(0, 0, 0, 0);
    root_widget_->setProperty("VideoPanel", true);
    layout()->addWidget(root_widget_);
    
    QHBoxLayout* layoutTarget = new QHBoxLayout();
    layoutTarget->setContentsMargins(0, 0, 0, 0);
    layoutTarget->setSpacing(0);
    layoutTarget->setAlignment(Qt::AlignVCenter);
    root_widget_->setLayout(layoutTarget);

    QWidget* parentWidget = root_widget_;
    auto __addButton = [this, parentWidget, layoutTarget] (const char* propertyName, const char* slot)->QPushButton* {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);
        if (propertyName != NULL) {
            btn->setProperty(propertyName, true);
        }
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        layoutTarget->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, slot, Qt::QueuedConnection);
        return btn;
    };

    QPushButton* btn = NULL;
    layoutTarget->addSpacing(internal_spacer_w4);
    btn = __addButton("CallGoChat", SLOT(onClickGoChat()));
    layoutTarget->addSpacing(internal_spacer_w);

    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    mic_button_ = __addButton("CallEnableMic", SLOT(onAudioOnOffClicked()));
    mic_button_->setProperty("CallDisableMic", true);

    layoutTarget->addSpacing(internal_spacer_w);
    video_button_ = __addButton(NULL, SLOT(onVideoOnOffClicked()));

    layoutTarget->addSpacing(internal_spacer_w);
    stop_call_button_ = __addButton("StopCallButton", SLOT(onHangUpButtonClicked()));

    if (DISPLAY_ADD_CHAT_BUTTON) {
        layoutTarget->addSpacing(internal_spacer_w);
        add_chat_button_ = __addButton("CallAddChat", SLOT(onClickAddChat()));
    }

    layoutTarget->addSpacing(internal_spacer_w);
    sound_on_off_button_ = new voipTools::BoundBox<QPushButtonEx>(parentWidget);
    sound_on_off_button_->setCursor(QCursor(Qt::PointingHandCursor));
    sound_on_off_button_->setProperty("CallPanelEnBtn", true);
	sound_on_off_button_->setProperty("CallSoundOn", true);
    sound_on_off_button_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    sound_on_off_button_->setFlat(true);
    connect(sound_on_off_button_, SIGNAL(onHover()), this, SLOT(onSoundOnOffHover()), Qt::QueuedConnection);
    layoutTarget->addWidget(sound_on_off_button_);

    layoutTarget->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    settings_button_ = __addButton("CallSettings", SLOT(onClickSettings()));

    layoutTarget->addSpacing(internal_spacer_w);
    fullscreen_button_ = __addButton(NULL, SLOT(_onFullscreenClicked()));

    layoutTarget->addSpacing(internal_spacer_w4);

    _reset_hangup_text();
    connect(&v_vol_control_, SIGNAL(controlActivated(bool)), this, SLOT(controlActivated(bool)), Qt::QueuedConnection);
    connect(&h_vol_control_, SIGNAL(controlActivated(bool)), this, SLOT(controlActivated(bool)), Qt::QueuedConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMuteChanged(const std::string&,bool)), this, SLOT(onVoipMuteChanged(const std::string&,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);

    h_vol_control_.hide();
    v_vol_control_.hide();
}

Ui::VideoPanel::~VideoPanel() {
	
}

void Ui::VideoPanel::keyReleaseEvent(QKeyEvent* e) {
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        emit onkeyEscPressed();
    }
}

void Ui::VideoPanel::controlActivated(bool activated) {
    mouse_under_panel_ = activated;
    if (__underMouse(*this)) { // this prevents multiple signalling of mouse enter/mouse leave
        return;
    }

    if (activated) {
        emit onMouseEnter();
    } else {
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::onClickSettings() {
    if (MainPage* mainPage = MainPage::instance()) {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window())) {
            wnd->raise();
            wnd->activate();
        }
        mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }
}

void Ui::VideoPanel::onClickGoChat() {
    if (MainPage* mainPage = MainPage::instance()) {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window())) {
            wnd->raise();
            wnd->activate();
        }
    }

    if (!active_contact_.empty()) {
        Logic::GetContactListModel()->setCurrent(active_contact_.c_str(), true);
    }
}

void Ui::VideoPanel::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts) {
    if(contacts.empty()) {
        return;
    }
    active_contact_ = contacts[0].contact;
}

void Ui::VideoPanel::onClickAddChat() {
    assert(false);
}

void Ui::VideoPanel::onSoundOnOffHover() {
    const auto rc = rect();

    VolumeControl* vc;
    int xOffset = 0, yOffset = 0;
    
    if (rc.width() >= Utils::scale_value(660)) {
        vc = &h_vol_control_;
#ifdef __APPLE__
        xOffset = Utils::scale_value(6); // i don't know where appeared this offsets
        yOffset = Utils::scale_value(1);
#endif
    } else {
        vc = &v_vol_control_;
#ifdef __APPLE__
        yOffset = Utils::scale_value(-8);
        xOffset = Utils::scale_value(1);
#endif
    }

    auto p = vc->getAnchorPoint();
    auto p2 = sound_on_off_button_->mapToGlobal(sound_on_off_button_->rect().topLeft());
    
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

void Ui::VideoPanel::onVoipMuteChanged(const std::string& device_type, bool muted) {
    if (device_type == "audio_playback" && sound_on_off_button_) {
        sound_on_off_button_->setProperty("CallPanelEnBtn", !muted);
        sound_on_off_button_->setProperty("CallPanelDisBtn", muted);

        sound_on_off_button_->setProperty("CallSoundOn", !muted);
        sound_on_off_button_->setProperty("CallSoundOff", muted);
        sound_on_off_button_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanel::setFullscreenMode(bool en) {
    if (!fullscreen_button_) {
        return;
    }

	fullscreen_button_->setProperty("CallFSOff", en);
	fullscreen_button_->setProperty("CallFSOn", !en);
    fullscreen_button_->setStyle(QApplication::style());
}

void Ui::VideoPanel::_onFullscreenClicked() {
	emit onFullscreenClicked();
}

void Ui::VideoPanel::enterEvent(QEvent* e) {
	QWidget::enterEvent(e);
    if (!mouse_under_panel_) {
	    emit onMouseEnter();
    }
}

void Ui::VideoPanel::leaveEvent(QEvent* e) {
	QWidget::leaveEvent(e);

    const bool focusOnVolumePanel = __underMouse(h_vol_control_) || __underMouse(v_vol_control_);
    if (!focusOnVolumePanel) {
    	emit onMouseLeave();
    }
}

void Ui::VideoPanel::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    h_vol_control_.hide();
    v_vol_control_.hide();
}

void Ui::VideoPanel::moveEvent(QMoveEvent* e) {
    QWidget::moveEvent(e);
    h_vol_control_.hide();
    v_vol_control_.hide();
}

void Ui::VideoPanel::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    auto rc = rect();

    if (rc.width() >= Utils::scale_value(457)) {
        if (settings_button_) {
            settings_button_->show();
        }

        if (add_chat_button_) {
            add_chat_button_->show();
        }
    } else {
        if (settings_button_) {
            settings_button_->hide();
        }

        if (add_chat_button_) {
            add_chat_button_->hide();
        }
    }
    
#ifdef __APPLE__
    assert(_parent);
    if (_parent && !_parent->isFullScreen()) {
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), Utils::scale_value(5), Utils::scale_value(5));
        
        QRegion region(path.toFillPolygon().toPolygon());
        region = region + QRect(0, 0, rc.width(), Utils::scale_value(5));
        
        setMask(region);
    } else {
        clearMask();
    }
#endif

    h_vol_control_.hide();
    v_vol_control_.hide();

    /* WHEN I WILL KNOW HOW TO CHANGE PUSH BUTTON OPACITY, I WILL REMOVE REM FROM THIS CODE */
    //float opacity = (float(rc.width()) - 403.0f) / 54.0f;
    //opacity = std::min(std::max(0.0f, opacity), 1.0f);
    //if (opacity < 0.001f) {
    //    Ui_->settingsButton->hide();
    //    Ui_->addChatButton->hide();
    //} else if (opacity > 0.999f) {
    //    Ui_->settingsButton->show();
    //    Ui_->addChatButton->show();
    //} else {
    //    Ui_->settingsButton->setWindowOpacity(opacity);  <---------- HERE NEED TO CHANGE OPACITY !
    //    Ui_->addChatButton->setWindowOpacity(opacity);
    //}
}

void Ui::VideoPanel::_reset_hangup_text() {
    if (stop_call_button_) {
        stop_call_button_->setText("");
        stop_call_button_->repaint();
    }
}


void Ui::VideoPanel::onHangUpButtonClicked() {
	Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoPanel::onVoipMediaLocalAudio(bool enabled) {
    if (mic_button_) {
        mic_button_->setProperty("CallPanelEnBtn", enabled);
        mic_button_->setProperty("CallPanelDisBtn", !enabled);
        mic_button_->setProperty("CallEnableMic", enabled);
        mic_button_->setProperty("CallDisableMic", !enabled);
        mic_button_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanel::onVoipMediaLocalVideo(bool enabled) {
    if (!video_button_) {
        return;
    }
    video_button_->setProperty("CallPanelEnBtn", enabled);
    video_button_->setProperty("CallPanelDisBtn", !enabled);
    video_button_->setProperty("CallEnableCam", enabled);
    video_button_->setProperty("CallDisableCam", !enabled);
    video_button_->setStyle(QApplication::style());
}


void Ui::VideoPanel::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);

    if (e->type() == QEvent::ActivationChange) {
        if (isActiveWindow() || (root_widget_ && root_widget_->isActiveWindow())) {
            if (container_) {
                container_->raise();
                raise();
            }
        }
    }
}

void Ui::VideoPanel::onAudioOnOffClicked() {
	Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::VideoPanel::onVideoOnOffClicked() {
	Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}