#include "stdafx.h"
#include "VideoSettings.h"
#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"

Ui::VideoSettings::VideoSettings(QWidget* parent)
: QWidget(parent) {
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("videoSetting"));
    this->resize(493, 101);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);
    cb_video_capture_ = new QComboBox(this);
    cb_video_capture_->setObjectName(QStringLiteral("cbVideoCapture"));
    cb_video_capture_->setGeometry(QRect(10, 10, 301, 22));
    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(255);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(cb_video_capture_->sizePolicy().hasHeightForWidth());
    cb_video_capture_->setSizePolicy(sizePolicy1);
    cb_audio_playback_ = new QComboBox(this);
    cb_audio_playback_->setObjectName(QStringLiteral("cbAudioPlayback"));
    cb_audio_playback_->setGeometry(QRect(10, 40, 301, 22));
    sizePolicy1.setHeightForWidth(cb_audio_playback_->sizePolicy().hasHeightForWidth());
    cb_audio_playback_->setSizePolicy(sizePolicy1);
    cb_audio_capture_ = new QComboBox(this);
    cb_audio_capture_->setObjectName(QStringLiteral("cbAudioCapture"));
    cb_audio_capture_->setGeometry(QRect(10, 70, 301, 22));
    sizePolicy1.setHeightForWidth(cb_audio_capture_->sizePolicy().hasHeightForWidth());
    cb_audio_capture_->setSizePolicy(sizePolicy1);
    button_audio_playback_set_ = new QPushButton(this);
    button_audio_playback_set_->setObjectName(QStringLiteral("btnAudioPlaybackSet"));
    button_audio_playback_set_->setGeometry(QRect(320, 40, 141, 23));
    button_audio_capture_set_ = new QPushButton(this);
    button_audio_capture_set_->setObjectName(QStringLiteral("btnAudioCaptureSet"));
    button_audio_capture_set_->setGeometry(QRect(320, 70, 141, 23));
    button_audio_playback_set_->setText(QApplication::translate("voip_pages", "Settings", 0));
    QMetaObject::connectSlotsByName(this);
    

    connect(cb_audio_capture_, SIGNAL(currentIndexChanged(int)), this, SLOT(audioCaptureDevChanged(int)), Qt::QueuedConnection);
    connect(cb_audio_playback_, SIGNAL(currentIndexChanged(int)), this, SLOT(audioPlaybackDevChanged(int)), Qt::QueuedConnection);
    connect(cb_video_capture_, SIGNAL(currentIndexChanged(int)), this, SLOT(videoCaptureDevChanged(int)), Qt::QueuedConnection);

    connect(button_audio_playback_set_, SIGNAL(clicked()), this, SLOT(audioPlaybackSettings()), Qt::QueuedConnection);
    connect(button_audio_capture_set_, SIGNAL(clicked()), this, SLOT(audioCaptureSettings()), Qt::QueuedConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), this, SLOT(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), Qt::DirectConnection);
    Ui::GetDispatcher()->getVoipController().setRequestSettings();
}

Ui::VideoSettings::~VideoSettings() {

}

void Ui::VideoSettings::onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>& devices) {
    devices_ = devices;

    bool video_ca_upd = false;
    bool audio_pl_upd = false;
    bool audio_ca_upd = false;

    using namespace voip_proxy;
    for (unsigned ix = 0; ix < devices_.size(); ix++) {
        const device_desc& desc = devices_[ix];

        QComboBox* cb  = NULL;
        bool* flag_ptr = NULL;

        switch (desc.dev_type) {
        case kvoip_dev_type_audio_capture:
            cb = cb_audio_capture_;
            flag_ptr = &audio_ca_upd;
            break;

        case kvoip_dev_type_audio_playback:
            cb = cb_audio_playback_;
            flag_ptr = &audio_pl_upd;
            break;

        case  kvoip_dev_type_video_capture:
            cb = cb_video_capture_;
            flag_ptr = &video_ca_upd;
            break;

        case kvoip_dev_type_undefined:
        default:
            assert(false);
            continue;
        }

        assert(cb && flag_ptr);
        if (!cb || !flag_ptr) { continue; }

        if (!*flag_ptr) {
            *flag_ptr = true;
            cb->clear();
        }
        cb->addItem(QIcon (":/resources/main_window/appicon.ico"), desc.name.c_str(), desc.uid.c_str());
    }
}

void Ui::VideoSettings::audioPlaybackSettings() {
#ifdef _WIN32
    SHELLEXECUTEINFOA shell_info;
    memset(&shell_info, 0, sizeof(shell_info));

    shell_info.cbSize = sizeof(SHELLEXECUTEINFOA);
    shell_info.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shell_info.hwnd   = (HWND)winId();
    shell_info.lpVerb = "";
    shell_info.lpFile = "mmsys.cpl";
    shell_info.lpParameters = ",0";
    shell_info.lpDirectory  = NULL;
    shell_info.nShow        = SW_SHOWDEFAULT;

    BOOL ret = ShellExecuteExA(&shell_info);
	if (!ret)
	{
		assert(ret);
	}
#else//WIN32
    #warning audioPlaybackSettings
#endif//WIN32
}

void Ui::VideoSettings::audioCaptureSettings() {
#ifdef _WIN32
    SHELLEXECUTEINFOA shell_info;
    memset(&shell_info, 0, sizeof(shell_info));

    shell_info.cbSize = sizeof(SHELLEXECUTEINFOA);
    shell_info.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shell_info.hwnd   = (HWND)winId();
    shell_info.lpVerb = "";
    shell_info.lpFile = "mmsys.cpl";
    shell_info.lpParameters = ",1";
    shell_info.lpDirectory  = NULL;
    shell_info.nShow        = SW_SHOWDEFAULT;

    BOOL ret = ShellExecuteExA(&shell_info);
	if (!ret)
	{
		assert(false);
	}
    
#else//WIN32
    #warning audioPlaybackSettings
#endif//WIN32
}

void Ui::VideoSettings::_onComboBoxItemChanged(QComboBox& cb, int ix, const std::string& dev_type) {
    assert(ix >= 0 && ix < cb.count());
    if (ix < 0 || ix >= cb.count()) { return; }

    auto var = cb.itemData(ix);
    assert(var.type() == QVariant::String);
    if ((var.type() != QVariant::String)) { return; }

    std::string uid = var.toString().toUtf8().constData();
    assert(!uid.empty());
    if (uid.empty()) { return; }

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", dev_type);
    collection.set_value_as_string("uid", uid);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void Ui::VideoSettings::audioCaptureDevChanged(int ix) {
    assert(cb_audio_capture_);
    if (cb_audio_capture_) {
        _onComboBoxItemChanged(*cb_audio_capture_, ix, "audio_capture");
    }
}

void Ui::VideoSettings::audioPlaybackDevChanged(int ix) {
    assert(cb_audio_playback_);
    if (cb_audio_playback_) {
        _onComboBoxItemChanged(*cb_audio_playback_, ix, "audio_playback");
    }
}

void Ui::VideoSettings::videoCaptureDevChanged(int ix) {
    assert(cb_video_capture_);
    if (cb_video_capture_) {
        _onComboBoxItemChanged(*cb_video_capture_, ix, "video_capture");
    }
}
