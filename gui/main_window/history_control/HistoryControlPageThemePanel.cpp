#include "stdafx.h"
#include "HistoryControlPageThemePanel.h"
#include "../../controls/BackButton.h"
#include "../../controls/CustomButton.h"
#include "../../utils/utils.h"
#include "HistoryControlPage.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    HistoryControlPageThemePanel::HistoryControlPageThemePanel(HistoryControlPage* _parent) : QWidget(_parent), historyControlPage_(_parent), settingThemeToAll_(false)
    {
        QHBoxLayout *main_layout = new QHBoxLayout(this);
        main_layout->setSpacing(0);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        backFromThemeButton_ = new BackButton(this);
        connect(backFromThemeButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        backFromThemeButton_->setCursor(Qt::PointingHandCursor);
        main_layout->addWidget(backFromThemeButton_);
        
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        int buttonExtraSpace = Utils::scale_value(65);
        
        previewButton_ = new CustomButton(this, QString());
        connect(previewButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        Utils::ApplyStyle(previewButton_, "font-family: \"%FONT_FAMILY%\"; color: #282828; font-size: 17dip; text-align: left;");
        previewButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        previewButton_->setFlat(true);
        previewButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Preview"));
        previewButton_->adjustSize();
        previewButton_->setFixedWidth(previewButton_->sizeHint().width() + Utils::scale_value(10));
        main_layout->addWidget(previewButton_, 0, Qt::AlignLeft);
        previewButton_->setVisible(true);
        
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        
        cancelButton_ = new CustomButton(this, QString());
        Utils::ApplyStyle(cancelButton_, grey_button_style);
        cancelButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Cancel"));
        cancelButton_->setFixedWidth(cancelButton_->sizeHint().width() + buttonExtraSpace);
        cancelButton_->adjustSize();
        cancelButton_->setCursor(Qt::PointingHandCursor);
        cancelButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        cancelButton_->setCursor(Qt::PointingHandCursor);
        main_layout->addWidget(cancelButton_, 0, Qt::AlignRight);
        connect(cancelButton_, SIGNAL(clicked()), this, SLOT(cancelThemePressed()), Qt::QueuedConnection);
        
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));

        setToAllButton_ = new CustomButton(this, QString());
        Utils::ApplyStyle(setToAllButton_, main_button_noborder_style);
        setToAllButton_->setCursor(Qt::PointingHandCursor);
        setToAllButton_->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Expanding);
        setToAllButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Set to all"));
        setToAllButton_->setFixedWidth(setToAllButton_->sizeHint().width() + buttonExtraSpace + Utils::scale_value(30));
        main_layout->addWidget(setToAllButton_, 0, Qt::AlignRight);
        connect(setToAllButton_, SIGNAL(clicked()), this, SLOT(setToAllThemePressed()), Qt::QueuedConnection);
        
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        setButton_ = new CustomButton(this, "");
        Utils::ApplyStyle(setButton_, main_button_noborder_style);
        setButton_->setCursor(Qt::PointingHandCursor);
        setButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        setButton_->setText(QT_TRANSLATE_NOOP("top_theme_widget", "Set"));
        setButton_->adjustSize();
        setButton_->setFixedWidth(setButton_->sizeHint().width() + buttonExtraSpace);
        main_layout->addWidget(setButton_);
        connect(setButton_, SIGNAL(clicked()), this, SLOT(setThemePressed()), Qt::QueuedConnection);
        
        main_layout->addSpacerItem(new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));

        setVisible(false);
        setObjectName(QStringLiteral("top_widget"));
        setFixedHeight(Utils::scale_value(64));
        setLayout(main_layout);
    }
    
    HistoryControlPageThemePanel::~HistoryControlPageThemePanel()
    {
    }
    
    void HistoryControlPageThemePanel::updateTopThemeButtonsVisibility()
    {
        int cancelLeft = cancelButton_->geometry().topLeft().x();
        int setToAllLeft = setToAllButton_->geometry().topLeft().x();
        if (cancelLeft && setToAllLeft)
        {
            bool showPreview = setToAllLeft - cancelButton_->sizeHint().width()  > Utils::scale_value(175) + previewButton_->sizeHint().width();
            bool showCancel = setToAllLeft - cancelButton_->sizeHint().width() > Utils::scale_value(165);
            previewButton_->setVisible(showPreview);
            cancelButton_->setVisible(showCancel);
        }
    }

    void HistoryControlPageThemePanel::timerUpdateTopThemeButtonsVisibility()
    {
        updateTopThemeButtonsVisibility();
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
        painter.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        painter.drawLine(contentsRect().bottomLeft(), contentsRect().bottomRight());
    }
    
    void HistoryControlPageThemePanel::resizeEvent(QResizeEvent *)
    {
        updateTopThemeButtonsVisibility();
    }
}