#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/TextEmojiWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    Utils::SignalsDisconnector* GetDisconnector()
    {
        static auto disconnector_ = std::make_shared<Utils::SignalsDisconnector>();
        return disconnector_.get();
    }

    namespace { bool initialized_ = false; }

    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* _parent):
        QStackedWidget(_parent),
        general_(nullptr),
        notifications_(nullptr),
        about_(nullptr),
        contactus_(nullptr)
    {
        initialized_ = false;

        voiceAndVideo_.rootWidget = NULL;
        voiceAndVideo_.audioCaptureDevices = NULL;
        voiceAndVideo_.audioPlaybackDevices = NULL;
        voiceAndVideo_.videoCaptureDevices = NULL;
        voiceAndVideo_.aCapSelected = NULL;
        voiceAndVideo_.aPlaSelected = NULL;
        voiceAndVideo_.vCapSelected = NULL;
        
        voiceAndVideo_.rootWidget = new QWidget(this);
        voiceAndVideo_.rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initVoiceVideo(voiceAndVideo_.rootWidget, voiceAndVideo_);
        addWidget(voiceAndVideo_.rootWidget);

        auto setActiveDevice = [] (const voip_proxy::EvoipDevTypes& type)
        {
            QString settingsName;
            switch (type)
            {
                case voip_proxy::kvoipDevTypeAudioPlayback: { settingsName = settings_speakers;   break; }
                case voip_proxy::kvoipDevTypeAudioCapture:  { settingsName = settings_microphone; break; }
                case voip_proxy::kvoipDevTypeVideoCapture:  { settingsName = settings_webcam;     break; }
                case voip_proxy::kvoipDevTypeUndefined:
                default:
                    assert(!"unexpected device type");
                    return;
            };
            
            QString val = get_gui_settings()->get_value<QString>(settingsName, "");
            if (val != "")
            {
                voip_proxy::device_desc description;
                description.uid      = std::string(val.toUtf8());
                description.dev_type = type;
                
                Ui::GetDispatcher()->getVoipController().setActiveDevice(description);
            }
        };
        
        setActiveDevice(voip_proxy::kvoipDevTypeAudioPlayback);
        setActiveDevice(voip_proxy::kvoipDevTypeAudioCapture);
        setActiveDevice(voip_proxy::kvoipDevTypeVideoCapture);
        
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), this, SLOT(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), Qt::DirectConnection);
    }

    GeneralSettingsWidget::~GeneralSettingsWidget()
    {
        GetDisconnector()->clean();
    }

    void GeneralSettingsWidget::initialize()
    {
        if (initialized_)
            return;
        
        initialized_ = true;
        
        setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss"));
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        
        QHBoxLayout* mainLayout;
        mainLayout = new QHBoxLayout();
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        std::map<std::string, Synchronizator> collector;
        
        general_ = new GeneralSettings(this);
        general_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initGeneral(general_, collector);
        addWidget(general_);
        
        notifications_ = new QWidget(this);
        notifications_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initNotifications(notifications_, collector);
        addWidget(notifications_);
        
        themes_ = new QWidget(this);
        themes_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        addWidget(themes_);
        
        about_ = new QWidget(this);
        about_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAbout(about_, collector);
        addWidget(about_);
        
        contactus_ = new QWidget(this);
        contactus_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initContactUs(contactus_, collector);
        addWidget(contactus_);
        
        attachUin_ = new QWidget(this);
        attachUin_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachUin(attachUin_, collector);
        addWidget(attachUin_);
        
        attachPhone_ = new QWidget(this);
        attachPhone_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachPhone(attachPhone_, collector);
        addWidget(attachPhone_);
        
        setCurrentWidget(general_);
        
        for (auto cs: collector)
        {
            auto &s = cs.second;
            for (size_t si = 0, sz = s.widgets_.size(); sz > 1 && si < (sz - 1); ++si)
                for (size_t sj = si + 1; sj < sz; ++sj)
                    connect(s.widgets_[si], s.signal_, s.widgets_[sj], s.slot_),
                    connect(s.widgets_[sj], s.signal_, s.widgets_[si], s.slot_);
        }

        general_->recvUserProxy();
        QObject::connect(Ui::GetDispatcher(), &core_dispatcher::getUserProxy, general_, &GeneralSettings::recvUserProxy, Qt::DirectConnection);
    }
    
    void GeneralSettingsWidget::setType(int _type)
    {
        initialize();
        
        Utils::CommonSettingsType type = (Utils::CommonSettingsType)_type;

        if (type == Utils::CommonSettingsType::CommonSettingsType_General)
        {
            setCurrentWidget(general_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo)
        {
            setCurrentWidget(voiceAndVideo_.rootWidget);
            if (devices_.empty())
                Ui::GetDispatcher()->getVoipController().setRequestSettings();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Notifications)
        {
            setCurrentWidget(notifications_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Themes)
        {
            setCurrentWidget(themes_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_About)
        {
            setCurrentWidget(about_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settings_about_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_ContactUs)
        {
            setCurrentWidget(contactus_);
            emit Utils::InterConnector::instance().generalSettingsContactUsShown();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::feedback_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachPhone)
        {
            setCurrentWidget(attachPhone_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachUin)
        {
            setCurrentWidget(attachUin_);
            emit Utils::InterConnector::instance().updateFocus();
        }
    }

    void GeneralSettingsWidget::onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& _devices)
    {
        devices_ = _devices;

        bool videoCaUpd = false;
        bool audioPlUpd = false;
        bool audioCaUpd = false;

        //const QString aCapDev = get_gui_settings()->get_value<QString>(settings_microphone, "");
        //const QString aPlaDev = get_gui_settings()->get_value<QString>(settings_speakers, "");
        //const QString vCapDev = get_gui_settings()->get_value<QString>(settings_webcam, "");

        using namespace voip_proxy;
        for (unsigned ix = 0; ix < devices_.size(); ix++)
        {
            const device_desc& desc = devices_[ix];

            QMenu* menu  = NULL;
            bool* flagPtr = NULL;
            std::vector<DeviceInfo>* deviceList = NULL;
            TextEmojiWidget* currentSelected = NULL;
            //const QString* currentUID = NULL;

            switch (desc.dev_type)
            {
            case kvoipDevTypeAudioCapture:
                menu = voiceAndVideo_.audioCaptureDevices;
                flagPtr = &audioCaUpd;
                deviceList = &voiceAndVideo_.aCapDeviceList;
                currentSelected = voiceAndVideo_.aCapSelected;
                //currentUID = &aCapDev;
                break;

            case kvoipDevTypeAudioPlayback:
                menu = voiceAndVideo_.audioPlaybackDevices;
                flagPtr = &audioPlUpd;
                deviceList = &voiceAndVideo_.aPlaDeviceList;
                currentSelected = voiceAndVideo_.aPlaSelected;
                //currentUID = &aPlaDev;
                break;

            case  kvoipDevTypeVideoCapture:
                menu = voiceAndVideo_.videoCaptureDevices;
                flagPtr = &videoCaUpd;
                deviceList = &voiceAndVideo_.vCapDeviceList;
                currentSelected = voiceAndVideo_.vCapSelected;
                //currentUID = &vCapDev;
                break;

            case kvoipDevTypeUndefined:
            default:
                assert(false);
                continue;
            }

            assert(menu && flagPtr && deviceList);
            if (!menu || !flagPtr || !deviceList)
            {
                continue;
            }

            if (!*flagPtr)
            {
                *flagPtr = true;
                menu->clear();
                deviceList->clear();
                if (currentSelected)
                {
                    currentSelected->setText(desc.name.c_str());
                }
            }

            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            menu->addAction(desc.name.c_str());
            deviceList->push_back(di);

            if ((currentSelected && desc.isActive))
            {
                currentSelected->setText(desc.name.c_str());
            }
        }
    }

    void GeneralSettingsWidget::hideEvent(QHideEvent* _e)
    {
        QStackedWidget::hideEvent(_e);
    }

    void GeneralSettingsWidget::showEvent(QShowEvent* _e)
    {
        initialize();
        QStackedWidget::showEvent(_e);
    }
    
    void GeneralSettingsWidget::paintEvent(QPaintEvent* _event)
    {
        QStackedWidget::paintEvent(_event);

        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(
            geometry().x() - 1,
            geometry().y() - 1,
            visibleRegion().boundingRect().width() + 2,
            visibleRegion().boundingRect().height() + 2
        );
    }

    GeneralSettings::GeneralSettings(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    void GeneralSettings::recvUserProxy()
    {
        auto userProxy = Utils::get_proxy_settings();
        std::ostringstream str;
        if (userProxy->type_ > core::proxy_types::min && userProxy->type_ < core::proxy_types::max)
            str << userProxy->type_;
        else
            str << core::proxy_types::auto_proxy;

        auto connectionTypeName = str.str();
        if (connectionTypeChooser_)
            connectionTypeChooser_->setText(QString(connectionTypeName.c_str()));
    }
}
