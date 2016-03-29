#include "stdafx.h"
#include "HistoryControlPageThemePanel.h"
#include "../../controls/CustomButton.h"
#include "../../utils/utils.h"
#include "HistoryControlPage.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    HistoryControlPageThemePanel::HistoryControlPageThemePanel(HistoryControlPage* _parent) : QWidget(_parent), historyControlPage_(_parent), settingThemeToAll_(false)
    {
        QHBoxLayout *main_layout = new QHBoxLayout(this);
        main_layout->setObjectName(QStringLiteral("horizontalLayout"));
        main_layout->setContentsMargins(0, 0, 0, 0);
        
        
        h_spacer_0_ = new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        main_layout->addSpacerItem(h_spacer_0_);
        
        backFromThemeButton_ = new CustomButton(this, Utils::parse_image_name(":/resources/contr_back_100.png"));
        connect(backFromThemeButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        backFromThemeButton_->setFixedWidth(Utils::scale_value(40));
        backFromThemeButton_->setFixedHeight(Utils::scale_value(40));
        backFromThemeButton_->setCursor(Qt::PointingHandCursor);
        main_layout->addWidget(backFromThemeButton_);
        Utils::ApplyStyle(backFromThemeButton_, "QPushButton {background-color: #f1f1f1; border-width: 0dip; border-style: solid; border-radius: 20dip;} QPushButton:hover {background-color: #eaeaea;} QPushButton:hover {background-color: #e5e5e5;}");
        
        h_spacer_4_ = new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        main_layout->addSpacerItem(h_spacer_4_);
        
        int buttonExtraSpace = Utils::scale_value(65);
        
        previewButton_ = new CustomButton(this, "");
        connect(previewButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        Utils::ApplyStyle(previewButton_, "font-family: \"%FONT_FAMILY%\"; color: #282828; font-size: 17dip; text-align: left;");
        previewButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        previewButton_->setFlat(true);
        previewButton_->setAutoDefault(1);
        previewButton_->setDefault(1);
        previewButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Preview"));
        previewButton_->adjustSize();
        previewButton_->setFixedWidth(previewButton_->sizeHint().width() + 10);
        previewButton_->setObjectName("previewButton");
        main_layout->addWidget(previewButton_, 0, Qt::AlignLeft);
        previewButton_->setVisible(true);
        
        h_spacer_5_ = new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Expanding, QSizePolicy::Preferred);
        main_layout->addSpacerItem(h_spacer_5_);
        
        cancelButton_ = new CustomButton(this, "");
        Utils::ApplyStyle(cancelButton_, grey_button_style);
        cancelButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Cancel"));
        cancelButton_->setFixedWidth(cancelButton_->sizeHint().width() + buttonExtraSpace);
        cancelButton_->adjustSize();
        cancelButton_->setObjectName("cancelButton");
        cancelButton_->setAutoDefault(1);
        cancelButton_->setDefault(1);
        cancelButton_->setCursor(Qt::PointingHandCursor);
        cancelButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        cancelButton_->setCursor(Qt::PointingHandCursor);
        main_layout->addWidget(cancelButton_, 0, Qt::AlignRight);
        connect(cancelButton_, SIGNAL(clicked()), this, SLOT(cancelThemePressed()), Qt::QueuedConnection);
        main_layout->setSpacing(0);
        
        h_spacer_1_ = new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        main_layout->addSpacerItem(h_spacer_1_);

        setToAllButton_ = new CustomButton(this, "");
        Utils::ApplyStyle(setToAllButton_, main_button_noborder_style);
        setToAllButton_->setProperty("default", false);
        setToAllButton_->setObjectName("setToAllThemeButton");
        setToAllButton_->setAutoDefault(1);
        setToAllButton_->setDefault(1);
        setToAllButton_->setCursor(Qt::PointingHandCursor);
        setToAllButton_->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Expanding);
        setToAllButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Set to all"));
        setToAllButton_->setFixedWidth(setToAllButton_->sizeHint().width() + buttonExtraSpace + Utils::scale_value(30));
//        setToAllButton_->adjustSize();
        main_layout->addWidget(setToAllButton_, 0, Qt::AlignRight);
        connect(setToAllButton_, SIGNAL(clicked()), this, SLOT(setToAllThemePressed()), Qt::QueuedConnection);
        
        h_spacer_2_ = new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        main_layout->addSpacerItem(h_spacer_2_);
        
        setButton_ = new CustomButton(this, "");
        Utils::ApplyStyle(setButton_, main_button_noborder_style);
        setButton_->setProperty("default", true);
        setButton_->setObjectName("setThemeButton");
        setButton_->setAutoDefault(1);
        setButton_->setDefault(1);
        setButton_->setCursor(Qt::PointingHandCursor);
        setButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        setButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Set"));
        setButton_->adjustSize();
        setButton_->setFixedWidth(setButton_->sizeHint().width() + buttonExtraSpace);
        main_layout->addWidget(setButton_);
        connect(setButton_, SIGNAL(clicked()), this, SLOT(setThemePressed()), Qt::QueuedConnection);
        
        h_spacer_3_ = new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        main_layout->addSpacerItem(h_spacer_3_);

        setVisible(false);
        setObjectName(QStringLiteral("top_widget"));
        setFixedHeight(Utils::scale_value(64));
        setLayout(main_layout);
    }
    
    void HistoryControlPageThemePanel::updateTopThemeButtonsVisibility()
    {
        int cancelLeft = cancelButton_->geometry().topLeft().x();
        int setToAllLeft = setToAllButton_->geometry().topLeft().x();
        if (cancelLeft && setToAllLeft)
        {
            bool showPreview = setToAllLeft - cancelButton_->sizeHint().width()  > Utils::scale_value(175) + previewButton_->sizeHint().width();
            bool showCancel = setToAllLeft - cancelButton_->sizeHint().width() > Utils::scale_value(165);
            
            if (previewButton_->isVisible() != showPreview || cancelButton_->isVisible() != showCancel)
            {
                QTimer::singleShot(50, [this](){ this->updateTopThemeButtonsVisibility(); });
            }
            previewButton_->setVisible(showPreview);
            cancelButton_->setVisible(showCancel);
        }
    }
    
    void HistoryControlPageThemePanel::setShowSetThemeButton(const bool _show)
    {
        setButton_->setVisible(_show);
    }
    
    void HistoryControlPageThemePanel::setCallback(ThemePanelCallback _callback)
    {
        callback_ = _callback;
    }
    
    void HistoryControlPageThemePanel::cancelThemePressed()
    {
        if (callback_)
        {
            callback_(ThemePanelCancel);
            callback_ = nullptr;
        }
        historyControlPage_->showThemesTopPanel(false, false, nullptr);
    }
    
    void HistoryControlPageThemePanel::setThemePressed()
    {
        if (callback_)
        {
            callback_(ThemePanelSet);
            callback_ = nullptr;
        }
        historyControlPage_->showThemesTopPanel(false, false, nullptr);
    }
    
    void HistoryControlPageThemePanel::setToAllThemePressed()
    {
        if (callback_)
        {
            callback_(ThemePanelSetToAll);
            callback_ = nullptr;
        }
        historyControlPage_->showThemesTopPanel(false, false, nullptr);
    }
    
    void HistoryControlPageThemePanel::backFromThemePressed()
    {
        QString targetContact = settingThemeToAll_ ? "" : historyControlPage_->aimId();
        emit Utils::InterConnector::instance().themesSettingsShow(selectionThemeFromSettings_, targetContact);
        if (callback_)
        {
            callback_(ThemePanelBackToSettings);
            callback_ = nullptr;
        }
        historyControlPage_->showThemesTopPanel(false, false, nullptr);
    }
    
    void HistoryControlPageThemePanel::setSelectionToAll(bool _settingThemeToAll)
    {
        settingThemeToAll_ = _settingThemeToAll;
    }
    
    void HistoryControlPageThemePanel::paintEvent(QPaintEvent *_e)
    {
        QWidget::paintEvent(_e);
        
        QPainter painter(this);
        painter.fillRect(rect(), QColor(255, 255, 255, 0.95 * 255));
    }
    
    void HistoryControlPageThemePanel::resizeEvent(QResizeEvent *_e)
    {
        updateTopThemeButtonsVisibility();
    }
}