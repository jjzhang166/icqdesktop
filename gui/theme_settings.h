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
    private:
        void set_value_simple_data(const QString& _name, const char* _data, int _len, bool _postToCore = true);

        struct
        {
            int requestedId_;
            void reset()
            {
                requestedId_ = -1;
                callback_ = nullptr;
            }
            std::function<void(themes::themePtr, bool)> callback_;
        } themeRequest_;

        struct {
            int id_;
            QString aimId_;
        } formerTheme_;

        QMap<QString,int> contactsThemes_;
        QMap<QString,int> contactsThemesBackup_;
        std::vector<int> themesIdToLoad_;

        themes::themePtr defaultTheme_;
        themes::themePtr initialTheme_;

    public:
        qt_theme_settings();
        ~qt_theme_settings();

        void themesDataUnserialized();
        int themeIdForContact(QString _aimId);
        void unserialize(core::coll_helper _collection);

        void postDefaultThemeIdToCore(const int _themeId) const;
        void setOrLoadDefaultTheme();
        std::shared_ptr<themes::theme> getDefaultTheme()
        { return
            defaultTheme_;
        }
        void setDefaultTheme(std::shared_ptr<themes::theme> _theme);

        void postContactsThemesToCore() const;
        std::shared_ptr<themes::theme> themeForContact(QString _aimId);
        int contactOpenned(QString _aimId);
        void setThemeIdForContact(int _themeId, const QString& _aimId, const bool _saveContactsThemes = true);
        std::shared_ptr<themes::theme> themeForId(int _themeId);
        void flushThemesToLoad();
        void themeSelected(int _themeId, const QString& _aimId);
        std::unordered_map<int, int> getThemeCounts();

        bool getIsLoaded() const { return isLoaded_; };
        void setIsLoaded(bool _isLoaded) { isLoaded_ = _isLoaded; };


    private:
        bool setThemeToWindow(std::shared_ptr<themes::theme> _theme);
        void requestThemeImage(int _themeId, std::function<void(themes::themePtr, bool)> _requestThemeCallback);
        void processNoDefaultThemeCase();
        QString serializedContactsThemes() const;
        void unloadUnusedThemesImages();

        void saveThemesMapping();
        void restoreThemesMapping();
        themes::themePtr initialTheme();
        bool settingInProcess_;
        bool isLoaded_;

    private Q_SLOTS:
        void onThemeImageArrived(int, bool);
    };

    qt_theme_settings* get_qt_theme_settings();
}
