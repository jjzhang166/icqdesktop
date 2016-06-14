#pragma once

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
    
    class SettingsTab : public QWidget
    {
        class UI;

        Q_OBJECT

    public Q_SLOTS:
        void settingsProfileClicked();
        
    private Q_SLOTS:
        void settingsGeneralClicked();
        void settingsVoiceVideoClicked();
        void settingsNotificationsClicked();
        void settingsThemesClicked();
        void settingsAboutClicked();
        void settingsContactUsClicked();
        void settingsSignoutClicked();

    private:
        std::unique_ptr< UI > Ui_;
        Utils::CommonSettingsType CurrentSettingsItem_;

    public:
        SettingsTab(QWidget* _parent);
        ~SettingsTab() throw();
        
        void cleanSelection();
        
    private:
        void updateSettingsState();
    };
}