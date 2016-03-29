#include "stdafx.h"
#include "ThemesSettingsWidget.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/utils.h"
#include "../../../controls/BackButton.h"
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
        back_button_->setVisible(_do_set);
        back_button_and_caption_spacer_->setVisible(_do_set);
        caption_without_back_button_spacer_->setVisible(!_do_set);
    }
    
    void ThemesSettingsWidget::setTargetContact(QString _aimId)
    {
        themes_widget_->set_target_contact(_aimId);
    }
    
    void ThemesSettingsWidget::init(QWidget *parent)
    {
        setStyleSheet("border: none; background-color: white;");
        
        main_layout_ = new QVBoxLayout(parent);
        main_layout_->setSpacing(0);
        main_layout_->setObjectName(QStringLiteral("main_layout"));
        main_layout_->setContentsMargins(0, 0, 0, 0);
        
        themes_widget_ = new ThemesWidget(this, Utils::scale_value(8));
        
        auto themesCaption = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(24));
        themesCaption->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpapers"));
        
        
        back_button_ = new BackButton(this);
        back_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        back_button_->setFlat(true);
        back_button_->setFocusPolicy(Qt::NoFocus);
        back_button_->setCursor(Qt::PointingHandCursor);
        
        theme_caption_layout_ = new QHBoxLayout(this);
        theme_caption_layout_->setContentsMargins(Utils::scale_value(24), 0, 0, 0);
        theme_caption_layout_->setSpacing(0);
        
        caption_without_back_button_spacer_ = new QWidget(this);
        caption_without_back_button_spacer_->setFixedWidth(Utils::scale_value(24));
        caption_without_back_button_spacer_->setFixedHeight(Utils::scale_value(80));
        caption_without_back_button_spacer_->setStyleSheet("background-color: white;");
        caption_without_back_button_spacer_->setVisible(false);
        
        theme_caption_layout_->addWidget(caption_without_back_button_spacer_);
        
        theme_caption_layout_->addWidget(back_button_);
        
        back_button_and_caption_spacer_ = new QWidget(this);
        back_button_and_caption_spacer_->setFixedWidth(Utils::scale_value(15));
        back_button_and_caption_spacer_->setFixedHeight(Utils::scale_value(80));
        back_button_and_caption_spacer_->setStyleSheet("background-color: white;");
        
        theme_caption_layout_->addWidget(back_button_and_caption_spacer_);
        theme_caption_layout_->addWidget(themesCaption);
        
        QHBoxLayout *horizontal_layout = new QHBoxLayout(parent);
        horizontal_layout->setContentsMargins(0, 0, 0, 0);
        horizontal_layout->addWidget(themes_widget_);

        themes_widget_->addCaptionLayout(theme_caption_layout_);
        
        main_layout_->addLayout(horizontal_layout);
        
        connect(back_button_, SIGNAL(clicked()), this, SLOT(backPressed()));
    }
    
    void ThemesSettingsWidget::backPressed()
    {
        emit Utils::InterConnector::instance().themesSettingsBack();
    }
}