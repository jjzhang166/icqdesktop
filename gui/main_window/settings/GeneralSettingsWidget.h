#pragma once

namespace voip_proxy
{
    struct device_desc;
}

namespace Utils
{
    class SignalsDisconnector;
    struct ProxySettings;
}

namespace Ui
{
    class TextEmojiWidget;
    struct Synchronizator;
    class ConnectionSettingsWidget;

    Utils::SignalsDisconnector* GetDisconnector();
    

    class GeneralSettings : public QWidget
    {
        Q_OBJECT
    
    public:
        GeneralSettings(QWidget * _parent);

        public Q_SLOTS:
            void recvUserProxy();


    public:
        
        TextEmojiWidget* connection_type_chooser_;
    };

    class GeneralSettingsWidget : public QStackedWidget
    {        
        struct DeviceInfo
        {
            std::string name;
            std::string uid;
        };
        
        Q_OBJECT

    private:
        std::vector< voip_proxy::device_desc > devices_;

    private:
        struct VoiceAndVideoOptions
        {
            QWidget* rootWidget;

            std::vector<DeviceInfo> aCapDeviceList;
            std::vector<DeviceInfo> aPlaDeviceList;
            std::vector<DeviceInfo> vCapDeviceList;

            TextEmojiWidget* aCapSelected;
            TextEmojiWidget* aPlaSelected;
            TextEmojiWidget* vCapSelected;

            QMenu* audioCaptureDevices;
            QMenu* audioPlaybackDevices;
            QMenu* videoCaptureDevices;
        }
        _voiceAndVideo;

        GeneralSettings* general_;
        QWidget* notifications_;
        QWidget* themes_;
        QWidget* about_;
        QWidget* contactus_;
        QWidget* attachPhone_;
        QWidget* attachUin_;


        struct Creator
        {
            static void initAbout(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initGeneral(GeneralSettings* parent, std::map<std::string, Synchronizator> &collector);
            static void initVoiceVideo(QWidget* parent, VoiceAndVideoOptions& voiceAndVideo);//, std::map<std::string, Synchronizator> &/*collector*/);
            static void initThemes(QWidget* parent);
            static void initNotifications(QWidget* parent, std::map<std::string, Synchronizator> &collector);
            static void initContactUs(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachUin(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachPhone(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
        };

    public:
        GeneralSettingsWidget(QWidget* parent = nullptr);
        ~GeneralSettingsWidget();

        void setType(int type);

    private:
        void initialize();
        
        virtual void paintEvent(QPaintEvent *event) override;
        virtual void hideEvent(QHideEvent *e) override;
        virtual void showEvent(QShowEvent *e) override;

        private Q_SLOTS:
            void onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& devices);
    };
}