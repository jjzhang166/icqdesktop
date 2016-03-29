#include "stdafx.h"
#include "theme_settings.h"
#include "utils/gui_coll_helper.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "utils/utils.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "utils/log/log.h"
#include "main_window/history_control/HistoryControlPageThemePanel.h"

namespace Ui
{
    qt_theme_settings::qt_theme_settings() : default_theme_(NULL), themesIdToLoad_(std::vector<int>()), contactsThemes_(QMap<QString,int>()), contactsThemesBackup_(QMap<QString,int>()), setting_in_process_(false)
    {
        theme_request_.reset();
        connect(Ui::GetDispatcher(), SIGNAL(on_theme(int,bool)), this, SLOT(onThemeImageArrived(int,bool)));
    }
    
    qt_theme_settings* get_qt_theme_settings()
    {
        static std::unique_ptr<qt_theme_settings> settings(new qt_theme_settings());
        return settings.get();
    }
    
    qt_theme_settings::~qt_theme_settings()
    {
    }
    
    themes::themePtr qt_theme_settings::initialTheme()
    {
        if (!initial_theme_)
        {
            initial_theme_ = std::make_shared<themes::theme>();
        }
        return initial_theme_;
    }
    
    void qt_theme_settings::postDefaultThemeIdToCore(const int _theme_id) const
    {
        Ui::gui_coll_helper cl_coll(GetDispatcher()->create_collection(), true);
        cl_coll.set_value_as_int("id", _theme_id);
        
        GetDispatcher()->post_message_to_core("themes/default/id", cl_coll.get());
    }
    
    void qt_theme_settings::unserialize(core::coll_helper _collection)
    {
        core::iarray* values_array = _collection.get_value_as_array("values");
        if (!values_array)
        {
            assert(false);
            return;
        }
        
        QString tint_color;
        QByteArray imageDataArray;
        QByteArray thumbDataArray;
        QString contactsThemes;
        int theme_id = -1;
        bool tile = false;

        for (int i = 0; i < values_array->size(); ++i)
        {
            const core::ivalue* val = values_array->get_at(i);
            
            gui_coll_helper coll_val(val->get_as_collection(), false);
            std::string name = coll_val.get_value_as_string("name");
            core::istream* idata = coll_val.get_value_as_stream("value");
            int len = idata->size();
            
            const char* data = (char *)idata->read(len);
            
            if (name == "id")
            {
                if (len == 4)
                {
                    theme_id = *(data) | *(data+1) << 8 | *(data+2) << 16 | *(data+3) << 24;
                }
            }
            else if (name == "image")
            {
                imageDataArray = QByteArray(data, len);
            }
            else if (name == "thumb")
            {
                thumbDataArray = QByteArray(data, len);
            }
            else if (name == "tint_color")
            {
                tint_color = QString(std::string(data, len).c_str());
            }
            else if (name == "contacts_themes")
            {
                contactsThemes = QString(std::string(data, len).c_str());
            }
            else if (name == "tile")
            {
                if (len == 1)
                {
                    tile = *(data) & 0xFF;
                }
            }
        }
        
        if (theme_id != -1)
        {
            default_theme_ = std::make_shared<themes::theme>(theme_id, tint_color, imageDataArray, thumbDataArray, tile);
        }
        if (contactsThemes.length() > 1)
        {
            QStringList list = contactsThemes.split(",");
            for (int i = 0; i < list.count(); ++i)
            {
                QString item = list[i];
                QStringList contactTheme = item.split(":");
                if (contactTheme.count() == 2)
                {
                    QString aimId = contactTheme[0];
                    int themeId = contactTheme[1].toInt();
                    if (themeId >= 0)
                    {
                        contactsThemes_[aimId] = themeId;
                        themesIdToLoad_.push_back(themeId);
                    }
                }
            }
        }
    }
    
    void qt_theme_settings::onThemeImageArrived(int _theme_id, bool _failed)
    {
        if (!_failed)
        {
            auto theme = themeForId(_theme_id);
            if (theme_request_.requested_id_ == _theme_id && theme_request_.callback_)
            {
                theme_request_.callback_(theme);
                theme_request_.reset();
            }
        }
    }
    
    void qt_theme_settings::themes_data_unserialized()
    {
        if (default_theme_)
        {
            // replace temporary preloaded theme
            auto theme = Ui::themes::loaded_themes()[default_theme_->get_id()];
            if (theme)
            {
                default_theme_ = theme;
            }
            else
            {
                default_theme_ = initialTheme();
            }
        }
        else
        {
            processNoDefaultThemeCase();
        }
    }
    
    void qt_theme_settings::processNoDefaultThemeCase()
    {
        default_theme_ = initialTheme();
//        default_theme_ = Ui::themes::loaded_themes().begin()->second;   // consider first as default
//        if (default_theme_)
//        {
//            this->postDefaultThemeIdToCore(default_theme_->get_id());
//            requestThemeImage(default_theme_->get_id(), [this](themes::themePtr theme)
//            {
//                if (theme)
//                {
//                    default_theme_ = theme;
//                }
//            });
//        }
    }
    
    // preload used themes images
    void qt_theme_settings::flushThemesToLoad()
    {
        for (int i = 0; i < (int) themesIdToLoad_.size(); ++i)
        {
            int theme_id_to_load = themesIdToLoad_[i];
            Ui::GetDispatcher()->getTheme(theme_id_to_load);
        }
        themesIdToLoad_.clear();
    }
    
    bool qt_theme_settings::setThemeToWindow(std::shared_ptr<themes::theme> _theme)
    {
        if (_theme)
        {
            if (_theme->is_image_loaded())
            {
                QPixmap pixmap = _theme->get_image();
                
                QPainter pixPaint(&pixmap);
                QBrush brush(_theme->get_tint_color());
                pixPaint.fillRect(pixmap.rect(), brush);
                
                Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(pixmap, _theme->is_tile());
            }
            else
            {
                return false;
            }
        }
        else
        {
            QPixmap p;
            Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(p, true);
        }
        return true;
    }
    
    void qt_theme_settings::requestThemeImage(int _theme_id, std::function<void(themes::themePtr)> _callback)
    {
        Ui::GetDispatcher()->getTheme(_theme_id);
        theme_request_.requested_id_ = _theme_id;
        theme_request_.callback_ = _callback;
    }
    
    void qt_theme_settings::setOrLoadDefaultTheme()
    {
        if (!setThemeToWindow(default_theme_))
        {
            requestThemeImage(default_theme_->get_id(), [this](themes::themePtr theme)
            {
                if (theme)
                {
                    this->setThemeToWindow(theme);
                }
            });
        }
    }
    
    int qt_theme_settings::contactOpenned(QString _aimId)
    {
        auto theme = themeForContact(_aimId);
        if (theme && theme->get_id() >= 0)
        {
            if (!setThemeToWindow(theme))
            {
                requestThemeImage(theme->get_id(), [this](themes::themePtr theme)
                {
                    if (theme)
                    {
                        this->setThemeToWindow(theme);
                    }
                });
            }
            return theme->get_id();
        }
        return -1;
    }
    
    int qt_theme_settings::themeIdForContact(QString _aimId)
    {
        return themeForContact(_aimId)->get_id();
    }
    
    themes::themePtr qt_theme_settings::themeForContact(QString _aimId)
    {
        if (!contactsThemes_.contains(_aimId))
        {
            if (default_theme_)
            {
                return default_theme_;
            }
        }
        else if (contactsThemes_[_aimId] > 0)
        {
            int theme_id = contactsThemes_[_aimId];
            auto theme = Ui::themes::loaded_themes()[theme_id];
            if (theme)
            {
                return theme;
            }
        }
        return initialTheme();
    }
    
    std::shared_ptr<themes::theme> qt_theme_settings::themeForId(int _theme_id)
    {
        if (_theme_id == 0)
        {
            return initialTheme();
        }
        
        auto themes = Ui::themes::loaded_themes();
        auto theme = themes[_theme_id];
        if (theme)
        {
            return theme;
        }
        return default_theme_;
    }
    
    void qt_theme_settings::setDefaultTheme(std::shared_ptr<themes::theme> _theme)
    {
        default_theme_ = _theme;
        if (!setThemeToWindow(default_theme_))
        {
            requestThemeImage(_theme->get_id(), [this](themes::themePtr theme)
            {
                if (theme)
                {
                    this->setThemeToWindow(theme);
                }
            });
        }
        int theme_id = _theme->get_id();
        contactsThemes_.clear();
        postContactsThemesToCore();
        postDefaultThemeIdToCore(theme_id);
    }
    
    void qt_theme_settings::themeSelected(int _theme_id, const QString& _targetContact)
    {
        auto theme = themeForId(_theme_id);
        QString contactToOpen = _targetContact;
        bool showSetThemeToCurrent = true;
        if (contactToOpen == "")
        {
            contactToOpen = Logic::GetContactListModel()->contactToTryOnTheme();
            showSetThemeToCurrent = false;
        }
        
        Logic::GetContactListModel()->setCurrent(contactToOpen, true, [this, theme, _targetContact, showSetThemeToCurrent, contactToOpen](HistoryControlPage *page)
        {
            // consider peculiar case when themeSelected gets called without ThemePanelChoice-callback from previous call
            if (setting_in_process_)
            {
                this->restoreThemesMapping();
            }
            setting_in_process_ = true;
            
            page->update(_targetContact);
            this->saveThemesMapping();
            this->setThemeIdForContact(theme->get_id(), contactToOpen, false);
            page->updateWidgetsTheme();
            page->showThemesTopPanel(true, showSetThemeToCurrent, [this, theme, _targetContact, page, contactToOpen](ThemePanelChoice res) {
                if (res == ThemePanelCancel || res == ThemePanelBackToSettings)
                {
                    this->restoreThemesMapping();
                    this->setThemeIdForContact(this->themeIdForContact(contactToOpen), contactToOpen);
                    page->updateWidgetsTheme();
                    this->unloadUnusedThemesImages();
                }
                else if (res == ThemePanelSet)
                {
                    this->setThemeIdForContact(theme->get_id(), contactToOpen);
                }
                else if (res == ThemePanelSetToAll)
                {
                    this->setDefaultTheme(theme);
                }
                setting_in_process_ = false;
            });
        });
    }
    
    void qt_theme_settings::setThemeIdForContact(int _theme_id, const QString& _aimId, const bool saveContactsThemes)
    {
        auto theme = themeForId(_theme_id);
        if (_aimId != "")
        {
            if (!setThemeToWindow(theme))
            {
                requestThemeImage(_theme_id, [this](themes::themePtr theme)
                {
                    if (theme)
                    {
                        this->setThemeToWindow(theme);
                    }
                });
            }
            contactsThemes_[_aimId] = _theme_id;
            if (saveContactsThemes)
            {
                postContactsThemesToCore();
            }
        }
        else if (theme)
        {
            setDefaultTheme(theme);
        }
        
    }
    
    QString qt_theme_settings::serializedContactsThemes() const
    {
        QString result = "-";
        if (contactsThemes_.size())
        {
            result = "";
        }
        for (auto iter = contactsThemes_.begin(); iter != contactsThemes_.end(); ++iter)
        {
            QString aimId = iter.key();
            int themeId = iter.value();
            result.append(QString("%1:%2,").arg(aimId).arg(themeId));
        }
        if (contactsThemes_.size())
        {
            result.remove(result.length()-1, 1);
        }
        return result;
    }
    
    void qt_theme_settings::postContactsThemesToCore() const
    {
        Ui::gui_coll_helper cl_coll(GetDispatcher()->create_collection(), true);
        
        core::ifptr<core::istream> data_stream(cl_coll->create_stream());
        
        QString result = serializedContactsThemes();

        uint size = result.toLatin1().size();
        if (size)
        {
            data_stream->write((const uint8_t*) result.toLatin1().data(), size);
        }
        
        cl_coll.set_value_as_qstring("name", "contacts_themes");
        cl_coll.set_value_as_stream("value", data_stream.get());

        GetDispatcher()->post_message_to_core("themes/settings/set", cl_coll.get());
    }
    
    void qt_theme_settings::saveThemesMapping()
    {
        contactsThemesBackup_ = contactsThemes_;
        if (default_theme_)
        {
            contactsThemesBackup_["default"] = default_theme_->get_id();
        }
    }
    
    void qt_theme_settings::restoreThemesMapping()
    {
        int default_theme_id = contactsThemesBackup_["default"];
        
        default_theme_ = themeForId(default_theme_id);
        contactsThemesBackup_.remove("default");
        
        contactsThemes_ = contactsThemesBackup_;
    }
    
    void qt_theme_settings::unloadUnusedThemesImages()
    {
        std::set<int> used;
        for (auto it = contactsThemes_.begin(); it != contactsThemes_.end(); ++it)
        {
            int usedTheme = it.value();
            used.insert(usedTheme);
        }
        if (default_theme_)
        {
            used.insert(default_theme_->get_id());
        }
        Ui::themes::unload_unused_themes_images(used);
    }
}
