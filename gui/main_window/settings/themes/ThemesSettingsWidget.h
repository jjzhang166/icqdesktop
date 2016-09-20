#pragma once

namespace Ui
{
    class ThemesWidget;
    class ThemesSettingsWidget: public QWidget
    {
        Q_OBJECT

        ThemesWidget *themesWidget_;

        void init();
        
    public:
        ThemesSettingsWidget(QWidget* _parent);

        void setBackButton(bool _doSet);
        void setTargetContact(QString _aimId);
    };
    
    
}

