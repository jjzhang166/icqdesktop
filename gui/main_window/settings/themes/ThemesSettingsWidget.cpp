#include "stdafx.h"
#include "ThemesSettingsWidget.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/utils.h"
#include "../../../controls/CustomButton.h"
#include "../../../utils/InterConnector.h"
#include "ThemesWidget.h"

namespace Ui
{
    ThemesSettingsWidget::ThemesSettingsWidget(QWidget* /*_parent*/)
    {
        init(this);
    }
    
    void ThemesSettingsWidget::setBackButton(bool _do_set)
    {
        backBackButtonWidget_->setVisible(_do_set);
    }
    
    void ThemesSettingsWidget::setTargetContact(QString _aimId)
    {
        themesWidget_->set_target_contact(_aimId);
    }
    
    void ThemesSettingsWidget::init(QWidget *parent)
    {
        setStyleSheet("border: none; background-color: white;");
        
        main_layout_ = new QVBoxLayout(parent);
        main_layout_->setSpacing(0);
        main_layout_->setObjectName(QStringLiteral("main_layout"));
        main_layout_->setContentsMargins(0, 0, 0, 0);
        
        themesWidget_ = new ThemesWidget(this, Utils::scale_value(15));
        
        QWidget *v_spacer1 = new QWidget();
        v_spacer1->setFixedHeight(Utils::scale_value(10));
        v_spacer1->setStyleSheet("background-color: white;");
        main_layout_->addWidget(v_spacer1);
        
        auto themesCaption = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(24));
        themesCaption->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpapers"));
        
        theme_caption_layout_ = new QHBoxLayout(parent);
        theme_caption_layout_->setContentsMargins(0, 0, 0, 0);
        theme_caption_layout_->setSpacing(0);
        
        QWidget *h_spacer1 = new QWidget();
        h_spacer1->setFixedWidth(Utils::scale_value(20));
        h_spacer1->setStyleSheet("background-color: white;");
        theme_caption_layout_->addWidget(h_spacer1);
        
        backBackButtonWidget_ = new QWidget(this);
        backBackButtonWidget_->setFixedHeight(Utils::scale_value(29));
        backBackButtonWidget_->setFixedWidth(Utils::scale_value(30));
        backBackButtonWidget_->setStyleSheet("background-color: white;");
        backButton_ = new CustomButton(backBackButtonWidget_, ":/resources/contr_back_100.png");
        backButton_->setFixedHeight(Utils::scale_value(29));
        Utils::ApplyStyle(backButton_, "background-color: #f1f1f1; border-width: 0dip; border-style: solid; border-radius: 14dip;");
        
        theme_caption_layout_->addWidget(backBackButtonWidget_);
        
        QWidget *h_spacer2 = new QWidget();
        h_spacer2->setFixedWidth(Utils::scale_value(20));
        h_spacer2->setStyleSheet("background-color: white;");
        theme_caption_layout_->addWidget(h_spacer2);
        
        theme_caption_layout_->addWidget(themesCaption);
        
        QHBoxLayout *horizontal_layout = new QHBoxLayout(parent);
        horizontal_layout->setContentsMargins(0, 0, 0, 0);
        horizontal_layout->addWidget(themesWidget_);
        main_layout_->addLayout(theme_caption_layout_);
        
        QWidget *v_spacer2 = new QWidget();
        v_spacer2->setFixedHeight(Utils::scale_value(10));
        v_spacer2->setStyleSheet("background-color: white;");
        main_layout_->addWidget(v_spacer2);
        
        main_layout_->addLayout(horizontal_layout);
        
        connect(backButton_, SIGNAL(clicked()), this, SLOT(backPressed()));
    }
    
    void ThemesSettingsWidget::backPressed()
    {
        emit Utils::InterConnector::instance().themesSettingsBack();
    }
}