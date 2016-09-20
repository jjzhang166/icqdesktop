#pragma once

#include "../../controls/GeneralCreator.h"

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
        
        TextEmojiWidget* connectionTypeChooser_;
        GeneralCreator::addSwitcherWidgets launchMinimized_;
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
        voiceAndVideo_;

        GeneralSettings* general_;
        QWidget* notifications_;
        QWidget* themes_;
        QWidget* about_;
        QWidget* contactus_;
        QWidget* attachPhone_;
        QWidget* attachUin_;


        struct Creator
        {
            static void initAbout(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initGeneral(GeneralSettings* _parent, std::map<std::string, Synchronizator>& _collector);
            static void initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo);
            static void initThemes(QWidget* _parent);
            static void initNotifications(QWidget* _parent, std::map<std::string, Synchronizator>& _collector);
            static void initContactUs(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachUin(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachPhone(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/);
        };

    public:
        GeneralSettingsWidget(QWidget* _parent = nullptr);
        ~GeneralSettingsWidget();

        void setType(int _type);

    private:
        void initialize();
        
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void hideEvent(QHideEvent* _e) override;
        virtual void showEvent(QShowEvent* _e) override;

        private Q_SLOTS:
            void onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& _devices);
    };
}