#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "../../../common.shared/version_info_constants.h"

#include "../contact_list/ContactListModel.h"
#include "../contact_list/contact_profile.h"

#include "../../controls/CustomButton.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/BackButton.h"
#include "../../controls/ConnectionSettingsWidget.h"
#include "../../controls/GeneralCreator.h"

#include "../../main_window/MainWindow.h"

namespace Ui
{
    Utils::SignalsDisconnector* GetDisconnector()
    {
        static auto disconnector_ = std::make_shared<Utils::SignalsDisconnector>();
        return disconnector_.get();
    }

    const QList<QString> &getLanguagesStrings()
    {
        static QList<QString> slist;
        if (slist.isEmpty())
        {
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "ru"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "en"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "uk"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "de"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "pt"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "cs"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "fr"));
            //slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "ar"));
            assert(slist.size() == Utils::GetTranslator()->getLanguages().size());
        }
        return slist;
    }

    QString languageToString(const QString &code)
    {
        auto codes = Utils::GetTranslator()->getLanguages();
        auto strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (1)");
        auto i = codes.indexOf(code);
        if (i >= 0 && i < codes.size())
            return strs[i];
        return "";
    }

    QString stringToLanguage(const QString &str)
    {
        auto codes = Utils::GetTranslator()->getLanguages();
        auto strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (2)");
        auto i = strs.indexOf(str);
        if (i >= 0 && i < strs.size())
            return codes[i];
        return "";
    }

    void GeneralSettingsWidget::Creator::initGeneral(GeneralSettings* parent, std::map<std::string, Synchronizator> &collector)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        {
            GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","General settings"));

            if (platform::is_windows())
            {
                GeneralCreator::addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Launch ICQ when system starts"), Utils::is_start_on_startup(), [](bool c) -> QString
                {
                    if (Utils::is_start_on_startup() != c)
                        Utils::set_start_on_startup(c);
                    return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
                });
                GeneralCreator::addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show taskbar icon"), get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true), [](bool c) -> QString
                {
                    emit Utils::InterConnector::instance().showIconInTaskbar(c);
                    get_gui_settings()->set_value<bool>(settings_show_in_taskbar, c);
                    return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
                });
            }
            
            if (platform::is_apple())
            {
                GeneralCreator::addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show in menu bar"), get_gui_settings()->get_value(settings_show_in_menubar, true), [](bool c) -> QString
                {
                    if (Utils::InterConnector::instance().getMainWindow())
                        Utils::InterConnector::instance().getMainWindow()->showMenuBarIcon(c);
                    if (get_gui_settings()->get_value(settings_show_in_menubar, true) != c)
                        get_gui_settings()->set_value(settings_show_in_menubar, c);
                    return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
                });
            }

            GeneralCreator::addSwitcher(&collector, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Sounds"), get_gui_settings()->get_value<bool>(settings_sounds_enabled, true), [](bool c) -> QString
            {
                Ui::GetDispatcher()->getVoipController().setMuteSounds(!c);
                if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != c)
                    get_gui_settings()->set_value<bool>(settings_sounds_enabled, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
            /*
            addSwitcher(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","Download files automatically"), get_gui_settings()->get_value<bool>(settings_download_files_automatically, true), [](bool c) -> QString
            {
            if (get_gui_settings()->get_value<bool>(settings_download_files_automatically, true) != c)
            get_gui_settings()->set_value<bool>(settings_download_files_automatically, c);
            return (c ? QT_TRANSLATE_NOOP("settings_pages","On") : QT_TRANSLATE_NOOP("settings_pages","Off"));
            });
            */
            {
                auto dp = get_gui_settings()->get_value(settings_download_directory, QString());
                if (!dp.length())
                    get_gui_settings()->set_value(settings_download_directory, Utils::DefaultDownloadsPath()); // workaround for core

            }
            GeneralCreator::addChooser(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Save to:"), QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())), [parent](TextEmojiWidget* b)
            {
#ifdef __linux__
                QWidget* parentForDialog = 0;
#else
                QWidget* parentForDialog = parent;
#endif //__linux__
                auto r = QFileDialog::getExistingDirectory(parentForDialog, QT_TRANSLATE_NOOP("settings_pages", "Choose new path"), QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())),
                    QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
                if (r.length())
                {
                    b->setText(r);
                    get_gui_settings()->set_value(settings_download_directory, QDir::toNativeSeparators(r));
                }
            });

            const auto& keysIndex = Utils::getSendKeysIndex();

            std::vector<QString> values;

            for (const auto& key : keysIndex)
                values.push_back(key.first);

            int currentKey = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);
            int selectedIndex = 0;

            for (unsigned int i = 0; i < keysIndex.size(); ++i)
            {
                if (currentKey == (int) keysIndex[i].second)
                    selectedIndex = i;
            }

            GeneralCreator::addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Send message by:"), values, selectedIndex, -1, [&keysIndex](QString v, int ix, TextEmojiWidget*)
            {
                get_gui_settings()->set_value<int>(settings_key1_to_send_message, keysIndex[ix].second);

            }, false, false, [](bool) -> QString { return ""; });
        }
        {
            GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Privacy"));
            GeneralCreator::addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show last message in recents"), get_gui_settings()->get_value<bool>(settings_show_last_message, true), [](bool c) -> QString
            {
                if (get_gui_settings()->get_value<bool>(settings_show_last_message, true) != c)
                    get_gui_settings()->set_value<bool>(settings_show_last_message, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });

            GeneralCreator::addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Preview images and videos"), get_gui_settings()->get_value<bool>(settings_show_video_and_images, true), [](bool c)
            {
                if (get_gui_settings()->get_value<bool>(settings_show_video_and_images, true) != c)
                    get_gui_settings()->set_value<bool>(settings_show_video_and_images, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
        }
        {
            GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Interface"));
#ifndef __APPLE__
            auto i = (get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::get_basic_scale_coefficient()) - 1.f) / .25f; if (i > 3) i = 3;
            std::vector< QString > sc; sc.push_back("100"); sc.push_back("125"); sc.push_back("150"); sc.push_back("200");
            GeneralCreator::addProgresser(scroll_area, scroll_area_content_layout, sc, i, [sc](TextEmojiWidget* w, TextEmojiWidget* aw, int i) -> void
            {
                static auto su = get_gui_settings()->get_value(settings_scale_coefficient, Utils::get_basic_scale_coefficient());
                double r = sc[i].toDouble() / 100.f;
                if (fabs(get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::get_basic_scale_coefficient()) - r) >= 0.25f)
                    get_gui_settings()->set_value<double>(settings_scale_coefficient, r);
                w->setText(QString("%1 %2%").arg(QT_TRANSLATE_NOOP("settings_pages", "Interface scale:")).arg(sc[i]), QColor("#282828"));
                if (fabs(su - r) >= 0.25f)
                    aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(ICQ restart required)"), QColor("#579e1c"));
                else if (fabs(Utils::get_basic_scale_coefficient() - r) < 0.05f)
                    aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(Recommended)"), QColor("#282828"));
                else
                    aw->setText(" ", QColor("#282828"));
            });
#endif

            auto ls = getLanguagesStrings();
            auto lc = languageToString(get_gui_settings()->get_value(settings_language, QString("")));
            auto li = ls.indexOf(lc);
            GeneralCreator::addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Language:"), ls.toVector().toStdVector(), li, -1, [scroll_area](QString v, int /*ix*/, TextEmojiWidget* ad)
            {
                static auto sl = get_gui_settings()->get_value(settings_language, QString(""));
                {
                    auto cl = stringToLanguage(v);
                    get_gui_settings()->set_value(settings_language, cl);
                    Utils::GetTranslator()->updateLocale();
                    if (ad && cl != sl)
                        ad->setText(QT_TRANSLATE_NOOP("settings_pages", "(ICQ restart required)"), QColor("#579e1c"));
                    else if (ad)
                        ad->setText(" ", QColor("#282828"));
                }
            },
                false, false, [](bool) -> QString { return ""; });
        }
        
        {
            GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","Connection settings"));
            parent->connection_type_chooser_ = GeneralCreator::addChooser(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Connection type:"), "Auto",
                [parent](TextEmojiWidget* b)
            {
                auto connection_settings_widget_ = new ConnectionSettingsWidget(parent);
                connection_settings_widget_->show();
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
            });
        }
    }

    void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* parent, VoiceAndVideoOptions& voiceAndVideo)//, std::map<std::string, Synchronizator> &/*collector*/)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        auto __deviceChanged = [&voiceAndVideo] (const int ix, const voip_proxy::evoip_dev_types dev_type)
        {
            assert(ix >= 0);
            if (ix < 0) { return; }

            std::vector<DeviceInfo>* devList = NULL;
            QString settingsName;
            switch (dev_type) {
            case voip_proxy::kvoip_dev_type_audio_playback: { settingsName = settings_speakers; devList   = &voiceAndVideo.aPlaDeviceList; break; }
            case voip_proxy::kvoip_dev_type_audio_capture:  { settingsName = settings_microphone; devList = &voiceAndVideo.aCapDeviceList; break; }
            case voip_proxy::kvoip_dev_type_video_capture:  { settingsName = settings_webcam; devList     = &voiceAndVideo.vCapDeviceList; break; }
            case voip_proxy::kvoip_dev_type_undefined:
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
            description.uid  = info.uid;
            description.dev_type = dev_type;

            Ui::GetDispatcher()->getVoipController().setActiveDevice(description);

            if (get_gui_settings()->get_value<QString>(settingsName, "") != description.uid.c_str())
                get_gui_settings()->set_value<QString>(settingsName, description.uid.c_str());
        };

        GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
        {
            std::vector< QString > vs;
            const auto di = GeneralCreator::addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Microphone:"), vs, 0, -1, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_audio_capture);
            },
                /*
                true, get_gui_settings()->get_value<bool>(settings_microphone_gain, false), [](bool c) -> QString
                {
                if (get_gui_settings()->get_value<bool>(settings_microphone_gain, false) != c)
                get_gui_settings()->set_value<bool>(settings_microphone_gain, c);
                return QT_TRANSLATE_NOOP("settings_pages", "Gain");
                });
                */
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.audioCaptureDevices = di.menu;
            voiceAndVideo.aCapSelected = di.currentSelected;
        }
        {
            std::vector< QString > vs;
            const auto di = GeneralCreator::addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Speakers:"), vs, 0, -1, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_audio_playback);
            },
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.audioPlaybackDevices = di.menu;
            voiceAndVideo.aPlaSelected = di.currentSelected;
        }
        {
            std::vector< QString > vs;
            const auto di = GeneralCreator::addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Webcam:"), vs, 0, -1, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_video_capture);
            },
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.videoCaptureDevices = di.menu;
            voiceAndVideo.vCapSelected = di.currentSelected;
        }
    }

    void GeneralSettingsWidget::Creator::initThemes(QWidget* parent)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content, true);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        GeneralCreator::addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
    }

    namespace { bool initialized_ = false; }

    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* parent):
        QStackedWidget(parent),
        general_(nullptr),
        notifications_(nullptr),
        about_(nullptr),
        contactus_(nullptr)
    {
        initialized_ = false;

        _voiceAndVideo.rootWidget = NULL;
        _voiceAndVideo.audioCaptureDevices = NULL;
        _voiceAndVideo.audioPlaybackDevices = NULL;
        _voiceAndVideo.videoCaptureDevices = NULL;
        _voiceAndVideo.aCapSelected = NULL;
        _voiceAndVideo.aPlaSelected = NULL;
        _voiceAndVideo.vCapSelected = NULL;
        
        _voiceAndVideo.rootWidget = new QWidget(this);
        _voiceAndVideo.rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initVoiceVideo(_voiceAndVideo.rootWidget, _voiceAndVideo);//, collector);
        addWidget(_voiceAndVideo.rootWidget);

        auto setActiveDevice = [] (const voip_proxy::evoip_dev_types& type)
        {
            QString settingsName;
            switch (type)
            {
                case voip_proxy::kvoip_dev_type_audio_playback: { settingsName = settings_speakers;   break; }
                case voip_proxy::kvoip_dev_type_audio_capture:  { settingsName = settings_microphone; break; }
                case voip_proxy::kvoip_dev_type_video_capture:  { settingsName = settings_webcam;     break; }
                case voip_proxy::kvoip_dev_type_undefined:
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
        
        setActiveDevice(voip_proxy::kvoip_dev_type_audio_playback);
        setActiveDevice(voip_proxy::kvoip_dev_type_audio_capture);
        setActiveDevice(voip_proxy::kvoip_dev_type_video_capture);
        
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
        
        setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss", Utils::get_scale_coefficient(), true));
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        
        QHBoxLayout* main_layout;
        main_layout = new QHBoxLayout();
        main_layout->setSpacing(0);
        main_layout->setContentsMargins(0, 0, 0, 0);
        
        std::map<std::string, Synchronizator> collector;
        
        general_ = new GeneralSettings(this);
        general_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initGeneral(general_, collector);
        addWidget(general_);

        /*
        _voiceAndVideo.rootWidget = new QWidget(this);
        _voiceAndVideo.rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initVoiceVideo(_voiceAndVideo.rootWidget, _voiceAndVideo, collector);
        addWidget(_voiceAndVideo.rootWidget);
        */
        
        notifications_ = new QWidget(this);
        notifications_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initNotifications(notifications_, collector);
        addWidget(notifications_);
        
        themes_ = new QWidget(this);
        themes_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        //Creator::initThemes(themes_);
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
        
        /*
        auto setActiveDevice = [] (const voip_proxy::evoip_dev_types& type)
        {
            QString settingsName;
            switch (type)
            {
                case voip_proxy::kvoip_dev_type_audio_playback: { settingsName = settings_speakers;   break; }
                case voip_proxy::kvoip_dev_type_audio_capture:  { settingsName = settings_microphone; break; }
                case voip_proxy::kvoip_dev_type_video_capture:  { settingsName = settings_webcam;     break; }
                case voip_proxy::kvoip_dev_type_undefined:
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
        
        setActiveDevice(voip_proxy::kvoip_dev_type_audio_playback);
        setActiveDevice(voip_proxy::kvoip_dev_type_audio_capture);
        setActiveDevice(voip_proxy::kvoip_dev_type_video_capture);
        
        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), this, SLOT(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), Qt::DirectConnection);
        */

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
            setCurrentWidget(_voiceAndVideo.rootWidget);
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

    void GeneralSettingsWidget::onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& devices)
    {
        devices_ = devices;

        bool video_ca_upd = false;
        bool audio_pl_upd = false;
        bool audio_ca_upd = false;

        //const QString aCapDev = get_gui_settings()->get_value<QString>(settings_microphone, "");
        //const QString aPlaDev = get_gui_settings()->get_value<QString>(settings_speakers, "");
        //const QString vCapDev = get_gui_settings()->get_value<QString>(settings_webcam, "");

        using namespace voip_proxy;
        for (unsigned ix = 0; ix < devices_.size(); ix++) {
            const device_desc& desc = devices_[ix];

            QMenu* menu  = NULL;
            bool* flag_ptr = NULL;
            std::vector<DeviceInfo>* deviceList = NULL;
            TextEmojiWidget* currentSelected = NULL;
            //const QString* currentUID = NULL;

            switch (desc.dev_type) {
            case kvoip_dev_type_audio_capture:
                menu = _voiceAndVideo.audioCaptureDevices;
                flag_ptr = &audio_ca_upd;
                deviceList = &_voiceAndVideo.aCapDeviceList;
                currentSelected = _voiceAndVideo.aCapSelected;
                //currentUID = &aCapDev;
                break;

            case kvoip_dev_type_audio_playback:
                menu = _voiceAndVideo.audioPlaybackDevices;
                flag_ptr = &audio_pl_upd;
                deviceList = &_voiceAndVideo.aPlaDeviceList;
                currentSelected = _voiceAndVideo.aPlaSelected;
                //currentUID = &aPlaDev;
                break;

            case  kvoip_dev_type_video_capture:
                menu = _voiceAndVideo.videoCaptureDevices;
                flag_ptr = &video_ca_upd;
                deviceList = &_voiceAndVideo.vCapDeviceList;
                currentSelected = _voiceAndVideo.vCapSelected;
                //currentUID = &vCapDev;
                break;

            case kvoip_dev_type_undefined:
            default:
                assert(false);
                continue;
            }

            assert(menu && flag_ptr && deviceList);
            if (!menu || !flag_ptr || !deviceList) {
                continue;
            }

            if (!*flag_ptr)
            {
                *flag_ptr = true;
                menu->clear();
                deviceList->clear();
                if (currentSelected) {
                    currentSelected->setText(desc.name.c_str());
                }
            }

            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            menu->addAction(desc.name.c_str());
            deviceList->push_back(di);

            if ((currentSelected && desc.isActive)) {
                currentSelected->setText(desc.name.c_str());
            }
        }
    }

    void GeneralSettingsWidget::hideEvent(QHideEvent *e)
    {
        QStackedWidget::hideEvent(e);
    }

    void GeneralSettingsWidget::showEvent(QShowEvent *e)
    {
        initialize();
        QStackedWidget::showEvent(e);
    }
    
    void GeneralSettingsWidget::paintEvent(QPaintEvent* event)
    {
        QStackedWidget::paintEvent(event);

        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(geometry().x() - 1, geometry().y() - 1, visibleRegion().boundingRect().width() + 2, visibleRegion().boundingRect().height() + 2);
    }

    GeneralSettings::GeneralSettings(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    void GeneralSettings::recvUserProxy()
    {
        auto user_proxy = Utils::get_proxy_settings();
        std::ostringstream str;
        if (user_proxy->type > core::proxy_types::min && user_proxy->type < core::proxy_types::max)
            str << user_proxy->type;
        else
            str << core::proxy_types::auto_proxy;

        auto connection_type_name = str.str();
        if (connection_type_chooser_)
            connection_type_chooser_->setText(QString(connection_type_name.c_str()));
    }
}
