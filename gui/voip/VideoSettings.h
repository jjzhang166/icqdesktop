#ifndef __VIDEO_SETTINGS_H__
#define __VIDEO_SETTINGS_H__



namespace voip_proxy {
    struct device_desc;
}

namespace Ui {

    class videoSetting;
    class VideoSettings : public QWidget {
        Q_OBJECT

    private Q_SLOTS:
        void audioCaptureDevChanged(int ix);
        void audioPlaybackDevChanged(int ix);
        void videoCaptureDevChanged(int ix);

        void audioPlaybackSettings();
        void audioCaptureSettings();

        void onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>& devices);

    public:
        VideoSettings(QWidget* parent);
        ~VideoSettings();

    private:
        std::vector<voip_proxy::device_desc> devices_;
        void _onComboBoxItemChanged(QComboBox& cb, int ix, const std::string& dev_type);
        QComboBox *cb_video_capture_;
        QComboBox *cb_audio_playback_;
        QComboBox *cb_audio_capture_;
        QPushButton *button_audio_playback_set_;
        QPushButton *button_audio_capture_set_;
    };
}

#endif//__VIDEO_SETTINGS_H__