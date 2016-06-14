#include "stdafx.h"
#include "CallPanelMain.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../utils/utils.h"
#include "VideoPanelHeader.h"
#include "VideoPanel.h"
#include "VoipTools.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "PushButton_t.h"

extern const QString widgetWithBg      ;
extern const QString widgetWithoutBg   ;
extern const QString widgetWithoutBgEx ;
const QString widgetWithoutColor = "QWidget { color: transparent; background-color : rgba(0,0,0,0%); background: rgba(253,220,111,1%); }";

const QString closeButtonStyle    = "QPushButton { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_close_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; padding-top: 2dip; padding-bottom: 2dip; padding-left: 11dip; padding-right: 12dip; border: none; }"
                                    "QPushButton:hover  { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_close_100_hover.png); background-color: #e81123; }"
                                    "QPushButton:hover:pressed { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_close_100_active.png); background-color: #d00516; }";

const QString minButtonStyle      = "QPushButton { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_minimize_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; padding-top: 2dip; padding-bottom: 2dip; padding-left: 11dip; padding-right: 11dip; border: none; }"
                                    "QPushButton:hover { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_minimize_100_hover.png); background-color: #d3d3d3; }"
                                    "QPushButton:hover:pressed { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_minimize_100_active.png); background-color: #c8c8c8; }";

const QString maxButtonStyle      = "QPushButton { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_bigwindow_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; padding-top: 2dip; padding-bottom: 2dip; padding-left: 11dip; padding-right: 11dip; border: none; }"
                                    "QPushButton:hover { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_bigwindow_100_hover.png); background-color: #d3d3d3; }"
                                    "QPushButton:hover:pressed { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_bigwindow_100_active.png); background-color: #c8c8c8; }";

const QString maxButtonStyle2     = "QPushButton { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; padding-top: 2dip; padding-bottom: 2dip; padding-left: 11dip; padding-right: 11dip; border: none; }"
                                    "QPushButton:hover { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100.png); background-color: #d3d3d3; }"
                                    "QPushButton:hover:pressed { width: 24dip; height: 24dip; background-image: url(:/resources/main_window/contr_smallwindow_100_active.png); background-color: #c8c8c8; }";

const QString buttonStyleEnabled  = "QPushButton { background-color: transparent; background-repeat: no-repeat; background-position: center; }"; 

const QString buttonStyleDisabled = "QPushButton { background-color: transparent; background-repeat: no-repeat; background-position: center; }";

const QString buttonGoChat        = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_chat_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_chat_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_chat_mini_100_active.png); }";

const QString buttonCameraEnable  = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_camera_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_camera_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_camera_mini_100_active.png); }";

const QString buttonCameraDisable = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_camera_off_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_camera_off_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_camera_off_mini_100_active.png); }";

const QString buttonStopCall      = "QPushButton { min-height: 40dip; max-height: 40dip; min-width: 40dip; max-width: 40dip; border-image: url(:/resources/contr_endcall_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/contr_endcall_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/contr_endcall_100_active.png); }";

const QString buttonMicEnable     = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_micro_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_micro_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_micro_mini_100_active.png); }";

const QString buttonMicDisable    = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_micro_off_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_micro_off_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_micro_off_mini_100_active.png); }";

const QString buttonSoundEnable   = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_volume_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_volume_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_volume_mini_100_active.png); }";

const QString buttonSoundDisable  = "QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip; border-image: url(:/resources/video_panel/videoctrl_volume_off_mini_100.png); }"
                                    "QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_volume_off_mini_100_hover.png); }"
                                    "QPushButton:hover:pressed { border-image: url(:/resources/video_panel/videoctrl_volume_off_mini_100_active.png); }";

const QString loginNameLable	  = "QPushButton:!enabled { color: rgb(0,0,0);}";

namespace {
    enum {
        kmenu_item_volume = 0,
        kmenu_item_mic = 1,
        kmenu_item_cam = 2
    };
}
#define ICON_SIZE Utils::scale_value(20)

#define PANEL_DEF_COLOR_R 127
#define PANEL_DEF_COLOR_G 127
#define PANEL_DEF_COLOR_B 127
#define PANEL_DEF_COLOR_A 255

#define COLOR_R_VAL(x) (((x) & 0xff000000) >> 24)
#define COLOR_G_VAL(x) (((x) & 0x00ff0000) >> 16)
#define COLOR_B_VAL(x) (((x) & 0x0000ff00) >>  8)
#define COLOR_A_VAL(x) (((x) & 0x000000ff)      )

#define TOP_PANEL_BUTTONS_W              Utils::scale_value(46)
#define TOP_PANEL_TITLE_W                Utils::scale_value(80)
#define BOTTOM_PANEL_BETWEEN_BUTTONS_GAP Utils::scale_value(32)
#define TOP_PANEL_SECURE_CALL_OFFSET     Utils::scale_value(8)

#define SECURE_BTN_BORDER_W    Utils::scale_value(24)
#define SECURE_BTN_ICON_W      Utils::scale_value(16)
#define SECURE_BTN_ICON_H      SECURE_BTN_ICON_W
#define SECURE_BTN_TEXT_W      Utils::scale_value(50)
#define SECURE_BTN_ICON2TEXT_W TOP_PANEL_SECURE_CALL_OFFSET

#define COLOR_SECURE_BTN_ACTIVE   QColor(0xf5, 0xc8, 0x36, 0xff)
#define COLOR_SECURE_BTN_INACTIVE QColor(0, 0, 0, 0)

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
    slider_icon_->setText(QString(), QString());
    
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

Ui::CallPanelMainEx::CallPanelMainEx(QWidget* parent, const CallPanelMainFormat& panelFormat)
: QWidget(parent)
, format_(panelFormat)
, nameLabel_(NULL)
, rootWidget_(new QWidget(this))
, _buttonMaximize(NULL)
, _buttonLocalCamera(NULL)
, _buttonLocalMic(NULL)
, _buttonSoundTurn(NULL)
, vVolControl_(this, false, widgetWithoutBgEx, widgetWithBg, [] (QPushButton& btn, bool muted)
{
    if (muted) {
        Utils::ApplyStyle(&btn, buttonStyleEnabled);
        Utils::ApplyStyle(&btn, buttonSoundDisable);
    } else {
        Utils::ApplyStyle(&btn, buttonStyleDisabled);
        Utils::ApplyStyle(&btn, buttonSoundEnable);
    }
})
, hVolControl_(this, true, widgetWithoutBgEx, widgetWithoutColor, [] (QPushButton& btn, bool muted)
{
    if (muted) {
        Utils::ApplyStyle(&btn, buttonStyleEnabled);
        Utils::ApplyStyle(&btn, buttonSoundDisable);
    } else {
        Utils::ApplyStyle(&btn, buttonStyleDisabled);
        Utils::ApplyStyle(&btn, buttonSoundEnable);
    }
})
, _secureCallEnabled(false)
, _secureCallWnd(NULL)
{
    setFixedHeight(format_.topPartHeight + format_.bottomPartHeight);
    {
        QVBoxLayout* l = new QVBoxLayout();
        l->setSpacing(0);
        l->setContentsMargins(0, 0, 0, 0);
        setLayout(l);
    }

    { // we need root widget to make transcluent window
        layout()->addWidget(rootWidget_);
    }

    {
        QVBoxLayout* l = new QVBoxLayout();
        l->setSpacing(0);
        l->setContentsMargins(0, 0, 0, 0);
        rootWidget_->setLayout(l);
    }

    {
        std::stringstream rootWidgetStyle;
        rootWidgetStyle << 
            "QWidget { background: rgba(" << COLOR_R_VAL(format_.rgba) <<
            "," << COLOR_G_VAL(format_.rgba) <<
            "," << COLOR_B_VAL(format_.rgba) <<
            "," << COLOR_A_VAL(format_.rgba) <<
            "); }";

        Utils::ApplyStyle(rootWidget_, rootWidgetStyle.str().c_str());
    }

    if (!platform::is_apple()) 
    { // top part
        QWidget* topPartWidget = new QWidget(rootWidget_);
        topPartWidget->setFixedHeight(format_.topPartHeight);
        {
            QHBoxLayout* l = new QHBoxLayout();
            l->setSpacing(0);
            l->setContentsMargins(0, 0, 0, 0);
            topPartWidget->setLayout(l);
        }

        if (format_.topPartFormat & kVPH_ShowLogo) {
            QWidget* logoWidg = new QWidget(topPartWidget);
            logoWidg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            {
                QHBoxLayout* l = new QHBoxLayout();
                l->setSpacing(0);
                l->setContentsMargins(0, 0, 0, 0);
                l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
                logoWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(logoWidg);

            QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            sizePolicy.setHorizontalStretch(0);
            sizePolicy.setVerticalStretch(0);

            QPushButton* logoBtn = new voipTools::BoundBox<QPushButton>(logoWidg);
            logoBtn->setProperty("WindowIcon", QVariant(true));
            logoBtn->setAttribute(Qt::WA_TransparentForMouseEvents);
            logoWidg->layout()->addWidget(logoBtn);

            sizePolicy.setHeightForWidth(logoBtn->sizePolicy().hasHeightForWidth());
            logoBtn->setSizePolicy(sizePolicy);

            QLabel* title = new voipTools::BoundBox<QLabel>(logoWidg);
            title->setProperty("Title", QVariant(true));
            title->setText("ICQ");
            title->setAttribute(Qt::WA_TransparentForMouseEvents);
            title->setMouseTracking(true);
            title->setFixedWidth(TOP_PANEL_TITLE_W);
            logoWidg->layout()->addWidget(title);
        }

        if (format_.topPartFormat & kVPH_ShowName) {
            QWidget* buttonWidg = new QWidget(topPartWidget);
            buttonWidg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            {
                QHBoxLayout* l = new QHBoxLayout();
                l->setSpacing(0);
                l->setContentsMargins(0, 0, 0, 0);
                l->setAlignment(Qt::AlignCenter);
                buttonWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(buttonWidg);

            QFont f = QApplication::font();
            f.setPixelSize(format_.topPartFontSize);
            f.setStyleStrategy(QFont::PreferAntialias);

            nameLabel_ = new voipTools::BoundBox<PushButton_t>(buttonWidg);
            nameLabel_->setFont(f);
            nameLabel_->setAlignment(Qt::AlignCenter);
            nameLabel_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
            nameLabel_->setPostfix(std::string(" " + getFotmatedTime(0)).c_str());
            nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
            nameLabel_->setPostfixColor(QColor(0x86, 0x6f, 0x3c, 0xff));

			// Make disable text color also black.
			Utils::ApplyStyle(nameLabel_, loginNameLable);

            connect(nameLabel_, SIGNAL(clicked()), this, SLOT(onSecureCallClicked()), Qt::QueuedConnection);
            buttonWidg->layout()->addWidget(nameLabel_);
        }

        {
            QWidget* sysWidg = new QWidget(topPartWidget);
            sysWidg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            {
                QHBoxLayout* l = new QHBoxLayout();
                l->setSpacing(0);
                l->setContentsMargins(0, 0, 0, 0);
                l->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
                sysWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(sysWidg);

            if (format_.topPartFormat & kVPH_ShowMin) {
                addButton<QPushButton>(*sysWidg, minButtonStyle, SLOT(_onMinimize()));
            }

            if (format_.topPartFormat & kVPH_ShowMax) {
                _buttonMaximize = addButton<QPushButton>(*sysWidg, maxButtonStyle, SLOT(_onMaximize()));
            }

            if (format_.topPartFormat & kVPH_ShowClose) {
                addButton<QPushButton>(*sysWidg, closeButtonStyle, SLOT(_onClose()));
            }
        }

        rootWidget_->layout()->addWidget(topPartWidget);
    }

    { // bottom part
        QWidget* bottomPartWidget = new QWidget(rootWidget_);
        bottomPartWidget->setFixedHeight(format_.bottomPartHeight);
        {
            QHBoxLayout* l = new QHBoxLayout();
            l->setSpacing(0);
            l->setContentsMargins(0, 0, 0, 0);
            bottomPartWidget->setLayout(l);
        }
        bottomPartWidget->layout()->setAlignment(Qt::AlignCenter);

        addButton<QPushButton>(*bottomPartWidget, buttonGoChat,        SLOT(_onClickGoChat()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        _buttonLocalCamera = addButton<QPushButton>(*bottomPartWidget, buttonCameraDisable, SLOT(_onCameraTurn()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        addButton<QPushButton>(*bottomPartWidget, buttonStopCall,      SLOT(_onStopCall()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        _buttonLocalMic = addButton<QPushButton>(*bottomPartWidget, buttonMicDisable,    SLOT(_onMicTurn()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        
        // For Mac Volume button has default cursor, because it fix blinking on mouse hover.
#ifdef __APPLE__
        _buttonSoundTurn = addButton<QPushButtonEx>(*bottomPartWidget, buttonSoundDisable,  SLOT(_onSoundTurn()), true);
#else
        _buttonSoundTurn = addButton<QPushButtonEx>(*bottomPartWidget, buttonSoundDisable,  SLOT(_onSoundTurn()));
#endif
        connect(_buttonSoundTurn , SIGNAL(onHover()), this, SLOT(onSoundTurnHover()), Qt::QueuedConnection);

        rootWidget_->layout()->addWidget(bottomPartWidget);
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), this, SLOT(onVoipCallNameChanged(const std::vector<voip_manager::Contact>&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipUpdateCipherState(const voip_manager::CipherState&)), this, SLOT(onVoipUpdateCipherState(const voip_manager::CipherState&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);

    QObject::connect(&hVolControl_, SIGNAL(onMuteChanged(bool)), this, SLOT(onMuteChanged(bool)), Qt::QueuedConnection);

    hVolControl_.hide();
    vVolControl_.hide();
}

Ui::CallPanelMainEx::~CallPanelMainEx()
{

}

void Ui::CallPanelMainEx::processOnWindowMaximized()
{
    if (_buttonMaximize) {
        Utils::ApplyStyle(_buttonMaximize, maxButtonStyle2);
    }
}

void Ui::CallPanelMainEx::processOnWindowNormalled()
{
    if (_buttonMaximize) {
        Utils::ApplyStyle(_buttonMaximize, maxButtonStyle);
    }
}

void Ui::CallPanelMainEx::onSoundTurnHover()
{
    if (!_buttonSoundTurn) {
        return;
    }

    const auto rc = rect();

    VolumeControl* vc;
    int xOffset = 0, yOffset = 0;
    
    if (rc.width() >= Utils::scale_value(660)) {
        vc = &hVolControl_;
#ifdef __APPLE__
//        xOffset = Utils::scale_value(6); // i don't know where appeared this offsets
        yOffset = Utils::scale_value(1);
#endif
    } else {
        vc = &vVolControl_;
#ifdef __APPLE__
 //       yOffset = Utils::scale_value(-8);
        xOffset = Utils::scale_value(1);
#endif
    }

    auto p = vc->getAnchorPoint();
    auto p2 = _buttonSoundTurn->mapToGlobal(_buttonSoundTurn->rect().topLeft());
    
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

void Ui::CallPanelMainEx::_onSoundTurn()
{
    assert(false);
}

void Ui::CallPanelMainEx::_onMicTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::CallPanelMainEx::_onMinimize()
{
    emit onMinimize();
}

void Ui::CallPanelMainEx::_onMaximize()
{
    emit onMaximize();
}

void Ui::CallPanelMainEx::_onStopCall()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::CallPanelMainEx::_onClose()
{
    emit onClose();
}

void Ui::CallPanelMainEx::onVoipCallNameChanged(const std::vector<voip_manager::Contact>& contacts) {
    if(contacts.empty()) {
        return;
    }

    active_contact_ = contacts[0].contact;
    QString friendlyName = Logic::GetContactListModel()->getDisplayName(active_contact_.c_str());
    friendlyName += " -";

    if (nameLabel_) {
        nameLabel_->setPrefix(friendlyName);
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
    }
}

void Ui::CallPanelMainEx::_onClickGoChat()
{
    emit onClickOpenChat(active_contact_);
}

void Ui::CallPanelMainEx::onVoipMediaLocalVideo(bool enabled) {
    if (_buttonLocalCamera) {
        if (enabled) {
            Utils::ApplyStyle(_buttonLocalCamera, buttonStyleEnabled);
            Utils::ApplyStyle(_buttonLocalCamera, buttonCameraEnable);
        } else {
            Utils::ApplyStyle(_buttonLocalCamera, buttonStyleDisabled);
            Utils::ApplyStyle(_buttonLocalCamera, buttonCameraDisable);
        }
    }
}

void Ui::CallPanelMainEx::onVoipMediaLocalAudio(bool enabled) {
    if (_buttonLocalMic) {
        if (enabled) {
            Utils::ApplyStyle(_buttonLocalMic, buttonStyleEnabled);
            Utils::ApplyStyle(_buttonLocalMic, buttonMicEnable);
        } else {
            Utils::ApplyStyle(_buttonLocalMic, buttonStyleDisabled);
            Utils::ApplyStyle(_buttonLocalMic, buttonMicDisable);
        }
    }
}

void Ui::CallPanelMainEx::_onCameraTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

template<typename __ButtonType>
__ButtonType* Ui::CallPanelMainEx::addButton(QWidget& parentWidget, const QString& propertyName, const char* slot, bool bDefaultCursor)
{
    __ButtonType* btn = new voipTools::BoundBox<__ButtonType>(&parentWidget);
    Utils::ApplyStyle(btn, propertyName);
    btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

    // For volume control we have blinking with cursor under mac.
    // We will use dfault cursor to fix it.
    if (!bDefaultCursor)
    {
        btn->setCursor(QCursor(Qt::PointingHandCursor));
    }
    btn->setFlat(true);
    btn->setFixedWidth(TOP_PANEL_BUTTONS_W);
    parentWidget.layout()->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, slot, Qt::QueuedConnection);

    return btn;
}

void Ui::CallPanelMainEx::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::moveEvent(QMoveEvent* e) {
    QWidget::moveEvent(e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::enterEvent(QEvent* e)
{
    QWidget::enterEvent(e);
//    hVolControl_.hide();
//    vVolControl_.hide();
}

void Ui::CallPanelMainEx::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::onMuteChanged(bool muted) {
    if (_buttonSoundTurn) {
        if (muted) {
            Utils::ApplyStyle(_buttonSoundTurn, buttonStyleEnabled);
            Utils::ApplyStyle(_buttonSoundTurn, buttonSoundDisable);
        } else {
            Utils::ApplyStyle(_buttonSoundTurn, buttonStyleDisabled);
            Utils::ApplyStyle(_buttonSoundTurn, buttonSoundEnable);
        }
    }
}

void Ui::CallPanelMainEx::onVoipUpdateCipherState(const voip_manager::CipherState& state)
{
    const bool enable = voip_manager::CipherState::kCipherStateEnabled == state.state;
    _secureCallEnabled = enable;
    if (nameLabel_) {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

        nameLabel_->setEnabled(enable);
        nameLabel_->setOffsets(enable ? TOP_PANEL_SECURE_CALL_OFFSET : 0);
        nameLabel_->setImageForState(PushButton_t::normal, enable ? ":/resources/video_panel/content_securecall_100.png" : "");
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
        nameLabel_->setCursor(enable ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));

		// Return postfix time text to default state.
		if (!enable)
		{
			nameLabel_->setPostfix(std::string(" " + getFotmatedTime(0)).c_str());
		}
    }

    if (_secureCallWnd && voip_manager::CipherState::kCipherStateEnabled == state.state) {
        _secureCallWnd->setSecureCode(state.secureCode);
    }
}

void Ui::CallPanelMainEx::onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex) {
    if (contact_ex.call_count <= 1) {
        voip_manager::CipherState state;
        state.state = voip_manager::CipherState::kCipherStateFailed;
        onVoipUpdateCipherState(state);

        if (_secureCallWnd) {
            _secureCallWnd->hide();
        }
    }
}

void Ui::CallPanelMainEx::onVoipCallTimeChanged(unsigned sec_elapsed, bool /*have_call*/) {
    if (nameLabel_) {
        nameLabel_->setPostfix(_secureCallEnabled ? getFotmatedTime(sec_elapsed).c_str() : std::string(" " + getFotmatedTime(sec_elapsed)).c_str());
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
    }
}

void Ui::CallPanelMainEx::onSecureCallWndOpened()
{
    assert(nameLabel_);
    if (nameLabel_) {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_ACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, COLOR_SECURE_BTN_ACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, COLOR_SECURE_BTN_ACTIVE);
    }
}

void Ui::CallPanelMainEx::onSecureCallWndClosed()
{
    assert(nameLabel_);
    if (nameLabel_) {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, _secureCallEnabled ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, _secureCallEnabled ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setCursor(_secureCallEnabled ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
    }
}

void Ui::CallPanelMainEx::onSecureCallClicked()
{
    assert(nameLabel_);
    if (!nameLabel_) {
        return;
    }

    QRect rc = geometry();
    rc.moveTopLeft(mapToGlobal(rc.topLeft()));

    if (!_secureCallWnd) {
        _secureCallWnd = new SecureCallWnd();
        connect(_secureCallWnd, SIGNAL(onSecureCallWndOpened()), this, SLOT(onSecureCallWndOpened()), Qt::QueuedConnection);
        connect(_secureCallWnd, SIGNAL(onSecureCallWndClosed()), this, SLOT(onSecureCallWndClosed()), Qt::QueuedConnection);
    }

    assert(_secureCallWnd);
    if (!_secureCallWnd) {
        return;
    }

    const QPoint windowTCPt(rc.center());
    const QPoint secureCallWndTLPt(windowTCPt.x() - _secureCallWnd->width()*0.5f, windowTCPt.y() - Utils::scale_value(12));

    voip_manager::CipherState cipherState;
    Ui::GetDispatcher()->getVoipController().getSecureCode(cipherState);

    if (voip_manager::CipherState::kCipherStateEnabled == cipherState.state) {
        _secureCallWnd->setSecureCode(cipherState.secureCode);
        _secureCallWnd->move(secureCallWndTLPt);
        _secureCallWnd->show();
        _secureCallWnd->raise();
        _secureCallWnd->setFocus(Qt::NoFocusReason);
    }
}