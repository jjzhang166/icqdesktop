#pragma once
#include "stdafx.h"
#include "../corelib/collection_helper.h"
#include "cache/themes/themes.h"

namespace Ui
{
    class HistoryControlPage;
    
    class qt_theme_settings : public QObject
    {
        Q_OBJECT
        void set_value_simple_data(const QString& _name, const char* _data, int _len, bool _post_to_core = true);
        
        struct {
            int requested_id_;
            void reset() { requested_id_ = -1; callback_ = nullptr; }
            std::function<void(themes::themePtr)> callback_;
        } theme_request_;
        
        struct {
            int id_;
            QString aimId_;
        } former_theme_;
        
        QMap<QString,int> contactsThemes_;
        QMap<QString,int> contactsThemesBackup_;
        std::vector<int> themesIdToLoad_;
    public:
        qt_theme_settings();
        ~qt_theme_settings();
        void themes_data_unserialized();
        int themeIdForContact(QString _aimId);
        void unserialize(core::coll_helper _collection);
        void postDefaultThemeIdToCore(const int _theme_id) const;
        themes::themePtr default_theme_;
        themes::themePtr initial_theme_;
        void setOrLoadDefaultTheme();
        std::shared_ptr<themes::theme> getDefaultTheme() { return default_theme_; }
        void setDefaultTheme(std::shared_ptr<themes::theme> _theme);
        void postContactsThemesToCore() const;
        std::shared_ptr<themes::theme> themeForContact(QString _aimId);
        int contactOpenned(QString _aimId);
        void setThemeIdForContact(int _theme_id, const QString& _aimId, const bool saveContactsThemes = true);
        std::shared_ptr<themes::theme> themeForId(int _theme_id);
        void flushThemesToLoad();
        void themeSelected(int _theme_id, const QString& _aimId);
    private:
        bool setThemeToWindow(std::shared_ptr<themes::theme> _theme);
        void requestThemeImage(int _theme_id, std::function<void(themes::themePtr)> requestThemeCallback);
        void processNoDefaultThemeCase();
        QString serializedContactsThemes() const;
        void unloadUnusedThemesImages();
        
        void saveThemesMapping();
        void restoreThemesMapping();
        themes::themePtr initialTheme();
        bool setting_in_process_;
    private Q_SLOTS:
        void onThemeImageArrived(int, bool);
    };
    
    qt_theme_settings* get_qt_theme_settings();
}
