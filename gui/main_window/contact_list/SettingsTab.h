#pragma once

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
    class Alert;
    
    class SettingsTab : public QWidget
    {
        class UI;

        Q_OBJECT
        
    private Q_SLOTS:
        void settingsProfileClicked();
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
        std::unique_ptr<Alert> logouter_;

    public:
        SettingsTab(QWidget* _parent);
        ~SettingsTab() throw();
        
        void cleanSelection();
        
    private:
        void updateSettingsState();
    };
}