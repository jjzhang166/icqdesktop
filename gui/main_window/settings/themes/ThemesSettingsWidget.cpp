#include "stdafx.h"
#include "ThemesSettingsWidget.h"

#include "ThemesWidget.h"
#include "../../../utils/utils.h"

namespace Ui
{
    ThemesSettingsWidget::ThemesSettingsWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        init();
    }
    
    void ThemesSettingsWidget::setBackButton(bool _doSet)
    {
        themesWidget_->setBackButton(_doSet);
    }
    
    void ThemesSettingsWidget::setTargetContact(QString _aimId)
    {
        themesWidget_->setTargetContact(_aimId);
    }
    
    void ThemesSettingsWidget::init()
    {        
        setStyleSheet("border: none; background-color: white;");
        
        auto mainLayout_ = new QVBoxLayout(this);
        mainLayout_->setSpacing(0);
        mainLayout_->setContentsMargins(0, 0, 0, 0);
        
        themesWidget_ = new ThemesWidget(this, Utils::scale_value(8));
        
        QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->addWidget(themesWidget_);

        mainLayout_->addLayout(horizontalLayout);
        
        Utils::grabTouchWidget(this);
    }
}