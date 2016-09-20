#include "stdafx.h"
#include "HistoryControlPage.h"
#include "HistoryControlPageThemePanel.h"
#include "../../controls/BackButton.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    HistoryControlPageThemePanel::HistoryControlPageThemePanel(HistoryControlPage* _parent) : QWidget(_parent), historyControlPage_(_parent), settingThemeToAll_(false)
    {
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        backFromThemeButton_ = new BackButton(this);
        connect(backFromThemeButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        backFromThemeButton_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(backFromThemeButton_);
        
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        int buttonExtraSpace = Utils::scale_value(65);
        
        previewButton_ = new CustomButton(this, QString());
        connect(previewButton_, SIGNAL(clicked()), this, SLOT(backFromThemePressed()), Qt::QueuedConnection);
        Utils::ApplyStyle(
            previewButton_,
            QString("color: %1; font-size: 17dip; text-align: left;")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor()))
        );
        previewButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        previewButton_->setFlat(true);
        previewButton_->setText(QT_TRANSLATE_NOOP("chat_page", "Preview"));
        previewButton_->adjustSize();
        previewButton_->setFixedWidth(previewButton_->sizeHint().width() + Utils::scale_value(10));
        mainLayout->addWidget(previewButton_, 0, Qt::AlignLeft);
        previewButton_->setVisible(true);
        
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        
        cancelButton_ = new CustomButton(this, QString());
        Utils::ApplyStyle(cancelButton_, CommonStyle::getGrayButtonStyle());
        cancelButton_->setText(QT_TRANSLATE_NOOP("chat_page", "Cancel"));
        cancelButton_->setFixedWidth(cancelButton_->sizeHint().width() + buttonExtraSpace);
        cancelButton_->adjustSize();
        cancelButton_->setCursor(Qt::PointingHandCursor);
        cancelButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        cancelButton_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(cancelButton_, 0, Qt::AlignRight);
        connect(cancelButton_, SIGNAL(clicked()), this, SLOT(cancelThemePressed()), Qt::QueuedConnection);
        
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));

        setToAllButton_ = new CustomButton(this, QString());
        Utils::ApplyStyle(setToAllButton_, CommonStyle::getGreenButtonStyleNoBorder());
        setToAllButton_->setCursor(Qt::PointingHandCursor);
        setToAllButton_->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Expanding);
        setToAllButton_->setText(QT_TRANSLATE_NOOP("chat_page", "Set to all"));
        setToAllButton_->setFixedWidth(setToAllButton_->sizeHint().width() + buttonExtraSpace + Utils::scale_value(30));
        mainLayout->addWidget(setToAllButton_, 0, Qt::AlignRight);
        connect(setToAllButton_, SIGNAL(clicked()), this, SLOT(setToAllThemePressed()), Qt::QueuedConnection);
        
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(15), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        setButton_ = new CustomButton(this, "");
        Utils::ApplyStyle(setButton_, CommonStyle::getGreenButtonStyleNoBorder());
        setButton_->setCursor(Qt::PointingHandCursor);
        setButton_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        setButton_->setText(QT_TRANSLATE_NOOP("chat_page", "Set"));
        setButton_->adjustSize();
        setButton_->setFixedWidth(setButton_->sizeHint().width() + buttonExtraSpace);
        mainLayout->addWidget(setButton_);
        connect(setButton_, SIGNAL(clicked()), this, SLOT(setThemePressed()), Qt::QueuedConnection);
        
        mainLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(10), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));

        setVisible(false);
        setFixedHeight(Utils::scale_value(64));
        setLayout(mainLayout);
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
        historyControlPage_->showMainTopPanel();
    }
    
    void HistoryControlPageThemePanel::setThemePressed()
    {
        if (callback_)
        {
            callback_(ThemePanelSet);
            callback_ = nullptr;
        }
        historyControlPage_->showMainTopPanel();
    }
    
    void HistoryControlPageThemePanel::setToAllThemePressed()
    {
        if (callback_)
        {
            callback_(ThemePanelSetToAll);
            callback_ = nullptr;
        }
        historyControlPage_->showMainTopPanel();
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
        historyControlPage_->showMainTopPanel();
    }
    
    void HistoryControlPageThemePanel::setSelectionToAll(bool _settingThemeToAll)
    {
        settingThemeToAll_ = _settingThemeToAll;
    }
    
    void HistoryControlPageThemePanel::paintEvent(QPaintEvent *_e)
    {
        QWidget::paintEvent(_e);

        QPainter painter(this);
        painter.fillRect(rect(), CommonStyle::getFrameTransparency());
        painter.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        painter.drawLine(contentsRect().bottomLeft(), contentsRect().bottomRight());
    }
    
    void HistoryControlPageThemePanel::resizeEvent(QResizeEvent *)
    {
        updateTopThemeButtonsVisibility();
    }
}