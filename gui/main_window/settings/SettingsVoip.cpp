#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    mainWidget->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

    scrollArea->setWidget(mainWidget);

    auto layout = new QHBoxLayout(_parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);

    auto __deviceChanged = [&_voiceAndVideo](const int ix, const voip_proxy::EvoipDevTypes dev_type)
    {
        assert(ix >= 0);
        if (ix < 0) { return; }

        std::vector<DeviceInfo>* devList = NULL;
        QString settingsName;
        switch (dev_type) {
        case voip_proxy::kvoipDevTypeAudioPlayback: { settingsName = settings_speakers; devList = &_voiceAndVideo.aPlaDeviceList; break; }
        case voip_proxy::kvoipDevTypeAudioCapture: { settingsName = settings_microphone; devList = &_voiceAndVideo.aCapDeviceList; break; }
        case voip_proxy::kvoipDevTypeVideoCapture: { settingsName = settings_webcam; devList = &_voiceAndVideo.vCapDeviceList; break; }
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(!"unexpected device type");
            return;
        };

        assert(devList);
        if (devList->empty()) { return; }

        assert(ix < (int)devList->size());
        const DeviceInfo& info = (*devList)[ix];

        voip_proxy::device_desc description;
        description.name = info.name;
        description.uid = info.uid;
        description.dev_type = dev_type;

        Ui::GetDispatcher()->getVoipController().setActiveDevice(description);

        if (get_gui_settings()->get_value<QString>(settingsName, "") != description.uid.c_str())
            get_gui_settings()->set_value<QString>(settingsName, description.uid.c_str());
    };

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Microphone:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioCapture);
        },

            false, false, [](bool) -> QString { return ""; });

        _voiceAndVideo.audioCaptureDevices = di.menu;
        _voiceAndVideo.aCapSelected = di.currentSelected;
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Speakers:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioPlayback);
        },
            false, false, [](bool) -> QString { return ""; });

        _voiceAndVideo.audioPlaybackDevices = di.menu;
        _voiceAndVideo.aPlaSelected = di.currentSelected;
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Webcam:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeVideoCapture);
        },
            false, false, [](bool) -> QString { return ""; });

        _voiceAndVideo.videoCaptureDevices = di.menu;
        _voiceAndVideo.vCapSelected = di.currentSelected;
    }
}