#ifndef __VIDEO_SETTINGS_H__
#define __VIDEO_SETTINGS_H__



namespace voip_proxy
{
    struct device_desc;
}

namespace Ui
{

    class videoSetting;
    class VideoSettings : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void audioCaptureDevChanged(int _ix);
        void audioPlaybackDevChanged(int _ix);
        void videoCaptureDevChanged(int _ix);

        void audioPlaybackSettings();
        void audioCaptureSettings();

        void onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>& _devices);

    public:
        VideoSettings(QWidget* _parent);
        ~VideoSettings();

    private:
        std::vector<voip_proxy::device_desc> devices_;
        void _onComboBoxItemChanged(QComboBox& _cb, int _ix, const std::string& _devType);
        QComboBox *cbVideoCapture_;
        QComboBox *cbAudioPlayback_;
        QComboBox *cbAudioCapture_;
        QPushButton *buttonAudioPlaybackSet_;
        QPushButton *buttonAudioCaptureSet_;
    };
}

#endif//__VIDEO_SETTINGS_H__