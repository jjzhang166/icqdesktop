#include "stdafx.h"
#include "CallPanelMain.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../utils/utils.h"
#include "../main_window/sounds/SoundsManager.h"
#include "VideoPanelHeader.h"
#include "VoipTools.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "VoipTools.h"

namespace {
    enum {
        kmenu_item_volume = 0,
        kmenu_item_mic = 1,
        kmenu_item_cam = 2
    };
}
#define ICON_SIZE Utils::scale_value(20)

extern std::string getFotmatedTime(unsigned ts);

Ui::SliderEx::SliderEx(QWidget* parent)
: QWidget(parent) {
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("sliderEx"));
    this->resize(252, 45);
    horizontal_layout_ = new QHBoxLayout(this);
    horizontal_layout_->setSpacing(0);
    horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
    horizontal_layout_->setContentsMargins(0, 0, 0, 0);

    slider_icon_ = new voipTools::BoundBox<PushButton_t>(this);
    slider_icon_->setObjectName(QStringLiteral("sliderIcon"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    //sizePolicy.setHorizontalStretch(0);
    //sizePolicy.setVerticalStretch(0);
    //sizePolicy.setHeightForWidth(slider_icon_->sizePolicy().hasHeightForWidth());
    slider_icon_->setSizePolicy(sizePolicy);
    
    horizontal_layout_->addWidget(slider_icon_);
    
    slider_ = new voipTools::BoundBox<QSlider>(this);
    slider_->setObjectName(QStringLiteral("slider"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
    //sizePolicy1.setHorizontalStretch(0);
    //sizePolicy1.setVerticalStretch(0);
    //sizePolicy1.setHeightForWidth(slider_->sizePolicy().hasHeightForWidth());
    slider_->setSizePolicy(sizePolicy1);
    slider_->setOrientation(Qt::Horizontal);
    
    horizontal_layout_->addWidget(slider_);
    slider_icon_->setText(QString());
    
    QMetaObject::connectSlotsByName(this);

    slider_->setMaximum(100);
    slider_->setMinimum(0);

    connect(slider_, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
    connect(slider_, SIGNAL(sliderReleased()), this, SLOT(onVolumeReleased()), Qt::QueuedConnection);
    connect(slider_icon_, SIGNAL(clicked()), this, SLOT(onIconClicked()), Qt::QueuedConnection);
}

Ui::SliderEx::~SliderEx(){

}

void Ui::SliderEx::setIconSize(const int w, const int h) {
    if (!!slider_icon_) {
        slider_icon_->setIconSize(w, h);
    }
}

void Ui::SliderEx::onVolumeChanged(int v) {
    emit onSliderValueChanged(v);
}

void Ui::SliderEx::onVolumeReleased() {
    emit onSliderReleased();
}

void Ui::SliderEx::onIconClicked() {
    emit onIconClick();
}

void Ui::SliderEx::set_enabled(bool en) {
    slider_->setEnabled(en);
}

void Ui::SliderEx::set_value(int v) {
    slider_->setValue(v);
}

void Ui::SliderEx::set_property_for_icon(const char* name, bool val) {
    slider_icon_->setProperty(name, val);
    slider_icon_->setStyle(QApplication::style());
}

void Ui::SliderEx::setIconForState(const PushButton_t::eButtonState state, const std::string& image) {
    if (!!slider_icon_) {
        slider_icon_->setImageForState(state, image);
    }
}

void Ui::SliderEx::set_property_for_slider(const char* name, bool val) {
    slider_->setProperty(name, val);
    slider_->setStyle(QApplication::style());
}

Ui::CallPanelMain::CallPanelMain(QWidget* parent)
    : QWidget(parent)
    , _menu(NULL)
    , actual_vol_(0)
    , menu_show_(NULL)
    , name_and_status_container_(NULL)
    , avatar_container_(NULL) {

        setProperty("CallPanelMain", true);

        QVBoxLayout* root_layout = new QVBoxLayout();
        root_layout->setContentsMargins(0, 0, 0, 0);
        root_layout->setSpacing(0);
        root_layout->setAlignment(Qt::AlignVCenter);
        setLayout(root_layout);

        QWidget* rootWidget = new QWidget(this);
        rootWidget->setContentsMargins(0, 0, 0, 0);
        rootWidget->setProperty("CallPanelMain", true);
        layout()->addWidget(rootWidget);

        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setAlignment(Qt::AlignVCenter);
        rootWidget->setLayout(layout);

        avatar_container_ = new voipTools::BoundBox<AvatarContainerWidget>(rootWidget, Utils::scale_value(50), Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(5));
        avatar_container_->setOverlap(0.6f);
        layout->addWidget(avatar_container_);

        name_and_status_container_ = new NameAndStatusWidget(rootWidget, Utils::scale_value(17), Utils::scale_value(12));
        name_and_status_container_->setNameProperty("CallPanelMainText_Name", true);
        name_and_status_container_->setStatusProperty("CallPanelMainText_Status", true);
        name_and_status_container_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        layout->addWidget(name_and_status_container_);

        layout->addSpacing(Utils::scale_value(6));

        menu_show_ = new voipTools::BoundBox<QPushButton>(rootWidget);
        menu_show_->setProperty("MenuShow", true);
		menu_show_->setCursor(QCursor(Qt::PointingHandCursor));
        connect(menu_show_, SIGNAL(clicked()), this, SLOT(onMenuButtonClicked()), Qt::QueuedConnection);
        layout->addWidget(menu_show_);

        layout->addSpacing(Utils::scale_value(24));

        QPushButton* callStop = new voipTools::BoundBox<QPushButton>(rootWidget);
        callStop->setProperty("StopCallMainButton", true);
		callStop->setCursor(QCursor(Qt::PointingHandCursor));
        connect(callStop, SIGNAL(clicked()), this, SLOT(onHangUpButtonClicked()), Qt::QueuedConnection);
        layout->addWidget(callStop);

        layout->addSpacing(Utils::scale_value(16));
        connect(&_menu, SIGNAL(onMenuOpenChanged(bool)), this, SLOT(onMenuOpenChanged(bool)), Qt::QueuedConnection);
        _menu.hide();

        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVolumeChanged(const std::string&,int)), this, SLOT(onVoipVolumeChanged(const std::string&,int)), Qt::DirectConnection);
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMuteChanged(const std::string&,bool)), this, SLOT(onVoipMuteChanged(const std::string&,bool)), Qt::DirectConnection);
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
		QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);

        {
            SliderEx* slider = new SliderEx(&_menu);
            slider->setProperty("CallMenuItemCommon", true);
            slider->set_property_for_slider("VideoPanelVolumeSlider", true);
            slider->set_property_for_icon("CallMenuItemButtonAPlaybackOff", true);
            slider->setIconSize(ICON_SIZE, ICON_SIZE);

            _menu.add_widget(kmenu_item_volume, slider);
            connect(slider, SIGNAL(onSliderValueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
            connect(slider, SIGNAL(onSliderReleased()), this, SLOT(onVolumeReleased()), Qt::QueuedConnection);
            connect(slider, SIGNAL(onIconClick()), this, SLOT(onVolumeOnOff()), Qt::QueuedConnection);
        }
        
        {
            PushButton_t* btn = new voipTools::BoundBox<PushButton_t>(&_menu);
            btn->setProperty("CallMenuItemCommon", true);
            btn->setProperty("CallMenuItemButton", true);
			btn->setCursor(QCursor(Qt::PointingHandCursor));
            btn->setOffsets(Utils::scale_value(12));
            btn->setIconSize(ICON_SIZE, ICON_SIZE);
            _menu.add_widget(kmenu_item_mic, btn);
            connect(btn, SIGNAL(clicked()), this, SLOT(onMuteMicOnOffClicked()), Qt::QueuedConnection);
        }
        
        {
            PushButton_t* btn = new voipTools::BoundBox<PushButton_t>(&_menu);
            btn->setProperty("CallMenuItemCommon", true);
            btn->setProperty("CallMenuItemButton", true);
            btn->setOffsets(Utils::scale_value(12));
			btn->setCursor(QCursor(Qt::PointingHandCursor));
            btn->setIconSize(ICON_SIZE, ICON_SIZE);
            _menu.add_widget(kmenu_item_cam, btn);
            connect(btn, SIGNAL(clicked()), this, SLOT(onVideoOnOffClicked()), Qt::QueuedConnection);
        }
}

void Ui::CallPanelMain::onVolumeOnOff() {
	Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
}

void Ui::CallPanelMain::onMenuOpenChanged(bool opened) {
    menu_show_->setProperty("MenuShow", !opened);
    menu_show_->setProperty("MenuHide", opened);
    menu_show_->setStyle(QApplication::style());
}

void Ui::CallPanelMain::onVoipMuteChanged(const std::string& device_type, bool muted) {
    if (device_type == "audio_playback") {
        if (SliderEx* slider_vol = (SliderEx*)_menu.get_widget(kmenu_item_volume)) {
            slider_vol->set_enabled(!muted);
            //slider_vol->set_property_for_icon("CallMenuVolume_en", !muted);
            //slider_vol->set_property_for_icon("CallMenuVolume_dis", muted);
            if (muted) {
                slider_vol->setIconForState(PushButton_t::normal, ":/resources/dialog_sound_off_100.png");
            } else {
                slider_vol->setIconForState(PushButton_t::normal, ":/resources/dialog_sound_100.png");
            }

            slider_vol->setStyle(QApplication::style());
			slider_vol->setCursor(QCursor(Qt::PointingHandCursor));
        } else {
            assert(false);
        }
    }
}

void Ui::CallPanelMain::onVoipMediaLocalAudio(bool enabled) {
    if (PushButton_t* btn = (PushButton_t*)_menu.get_widget(kmenu_item_mic)) {
        if (enabled) {
            btn->setImageForState(PushButton_t::normal, ":/resources/dialog_micro_100.png");
        } else {
            btn->setImageForState(PushButton_t::normal, ":/resources/dialog_micro_off_100.png");
        }

		btn->setText(enabled ? QT_TRANSLATE_NOOP("voip_pages", "Turn off microphone") : QT_TRANSLATE_NOOP("voip_pages", "Turn on microphone"));
        btn->setStyle(QApplication::style());
		btn->setCursor(QCursor(Qt::PointingHandCursor));
    } else {
        assert(false);
    }
}

void Ui::CallPanelMain::onVoipMediaLocalVideo(bool enabled) {
    if (PushButton_t* btn = (PushButton_t*)_menu.get_widget(kmenu_item_cam)) {
        if (enabled) {
            btn->setImageForState(PushButton_t::normal, ":/resources/dialog_video_100.png");
        } else {
            btn->setImageForState(PushButton_t::normal, ":/resources/dialog_video_off_100.png");
        }

        btn->setText(enabled ? QT_TRANSLATE_NOOP("voip_pages", "Turn off camera") : QT_TRANSLATE_NOOP("voip_pages", "Turn on camera"));
        btn->setStyle(QApplication::style());
    } else {
        assert(false);
    }
}

void Ui::CallPanelMain::onMuteMicOnOffClicked() {
	Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}


void Ui::CallPanelMain::onVoipCallTimeChanged(unsigned sec_elapsed, bool have_call) {
	if (have_call) {
		if (name_and_status_container_) {
			name_and_status_container_->setStatus(getFotmatedTime(sec_elapsed).c_str());
		}
	}
}

void Ui::CallPanelMain::onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex) {
    if (name_and_status_container_) {
        name_and_status_container_->setName("");
        name_and_status_container_->setStatus("");
    }

    if (!!avatar_container_ && contact_ex.call_count <= 1) {
        avatar_container_->dropExcess(std::vector<std::string>());
    }
}

std::string Ui::CallPanelMain::_get_tick_count() {
    return "00:00:00";
}

void Ui::CallPanelMain::onVideoOnOffClicked() {
	Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::CallPanelMain::onVolumeReleased() {
}

void Ui::CallPanelMain::onVoipVolumeChanged(const std::string& device_type, int vol) {
    if (device_type == "audio_playback") {
        actual_vol_ = std::max(std::min(100, vol), 0);
        SliderEx* slider_vol = (SliderEx*)_menu.get_widget(kmenu_item_volume);
        if (slider_vol) {
            slider_vol->blockSignals(true);
            slider_vol->set_value(actual_vol_);
            slider_vol->blockSignals(false);
        } else {
            assert(false);
        }
    }
}

void Ui::CallPanelMain::onVolumeChanged(int _vol) {
    const int new_vol = std::max(std::min(100, _vol), 0);
    if (actual_vol_ != new_vol) {
		Ui::GetDispatcher()->getVoipController().setVolumeAPlayback(new_vol);
    }
}

Ui::CallPanelMain::~CallPanelMain() {
}

void Ui::CallPanelMain::onMenuButtonClicked() {
    const QRect rect = menu_show_->rect();
    const QPoint lt = menu_show_->mapToGlobal(QPoint(rect.left(), rect.top()));
    const QRect rc(lt.x(), lt.y(), rect.width(), rect.height());

    const QRect video_paner_rc = _menu.rect();

    _menu.move(
        rc.x() + rc.width() - video_paner_rc.width() - Utils::scale_value(6), // 6 - is a margin from qss !!! sic! 
        rc.y() + rc.height());
    _menu.raise();

    if (_menu.isVisible()) {
        _menu.hide();
    } else {
        _menu.show();
        _menu.raise();
        _menu.setFocus(Qt::NoFocusReason);
    }
}

void Ui::CallPanelMain::onHangUpButtonClicked() {
	Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::CallPanelMain::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts) {
    if(contacts.empty()) {
        return;
    }

    std::vector<std::string> users;
    std::vector<std::string> friendly_names;
    for(unsigned ix = 0; ix < contacts.size(); ix++) {
        users.push_back(contacts[ix].contact);
        std::string n = Logic::GetContactListModel()->getDisplayName(contacts[ix].contact.c_str()).toUtf8().data();
        friendly_names.push_back(n);
    }

    if (!!avatar_container_) {
        avatar_container_->dropExcess(users);
    }

    auto name = voip_proxy::VoipController::formatCallName(friendly_names, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8());
    assert(!name.empty());

    name_and_status_container_->setName(name.c_str());
}

Ui::PushButton_t::PushButton_t(QWidget* parent/* = NULL*/) 
    : QPushButton(parent)
    , fromIconToText_(0)
    , iconW_(-1)
    , iconH_(-1)
    , currentState_(normal) {
    
}

void Ui::PushButton_t::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QRect rcDraw = rect();
    const QPixmap& icon   = !bitmapsForStates_[currentState_].isNull() ? bitmapsForStates_[currentState_] : bitmapsForStates_[normal];
    const QString textStr = text();

    if (!icon.isNull()) {
        const int w = std::min(iconW_ >= 0 ? iconW_ : icon.width(),  rcDraw.width());
        const int h = std::min(iconH_ >= 0 ? iconH_ : icon.height(), rcDraw.height());
        const QRect iconRect(rcDraw.left(), (rcDraw.top() + rcDraw.bottom() - h) * 0.5f, w, h);

        painter.drawPixmap(iconRect, icon);
        rcDraw.setLeft(rcDraw.left() + w + fromIconToText_);
    }

    if (!textStr.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        painter.setFont(font());
        painter.drawText(rcDraw, Qt::AlignVCenter | Qt::AlignLeft, textStr);

        painter.end();
    }
}

void Ui::PushButton_t::setIconSize(const int w, const int h) {
    iconW_ = w;
    iconH_ = h;
}

void Ui::PushButton_t::setImageForState(const eButtonState state, const std::string& image) {
    assert(state >= 0);
    assert(state < total);

    if (image.empty()) {
        bitmapsForStates_[state] = QPixmap();
    } else {
        bitmapsForStates_[state] = QPixmap(Utils::parse_image_name(image.c_str()));
    }
}

void Ui::PushButton_t::setOffsets(int fromIconToText) {
    fromIconToText_ = fromIconToText;
}

bool Ui::PushButton_t::event(QEvent *event) {
    if (event->type() == QEvent::Enter)
        (currentState_ = hovered), update();
    else if (event->type() == QEvent::Leave)
        (currentState_ = normal), update();
    if (event->type() == QEvent::MouseButtonPress)
        (currentState_ = pressed), update();

    return QPushButton::event(event);
}
