#include "stdafx.h"
#include "ThemesWidget.h"
#include "../../../controls/FlowLayout.h"
#include "ThemesModel.h"
#include "ThemeWidget.h"
#include "../../../utils/utils.h"
#include "../../../theme_settings.h"

namespace Ui
{
    namespace
    {
        const bool DISABLE_WAITER = true;
        
        QWidget *waiter_ = nullptr;
        bool waiterInited_ = false;

        void initWaiter(QWidget *parent)
        {
            if (waiterInited_ || DISABLE_WAITER)
                return;
            waiterInited_ = true;
            
            waiter_ = new QWidget(parent);
            waiter_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            waiter_->setStyleSheet("background: rgba(255,255,255,80%);");

            waiter_->setVisible(false);
            waiter_->setFixedSize(parent->size());
            
            auto spinnerHolder = new QLabel(waiter_);
            spinnerHolder->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            spinnerHolder->setContentsMargins(0, 0, 0, 0);
            spinnerHolder->setFixedSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));

            auto spinner = new QMovie(":/resources/gifs/r_spiner200.gif");
            spinner->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
            spinner->setSpeed(200);
            spinner->start();

            spinnerHolder->setMovie(spinner);

            auto waiterLayout = new QVBoxLayout(waiter_);
            waiterLayout->setContentsMargins(0, 0, 0, 0);
            waiterLayout->setSpacing(0);
            waiterLayout->setAlignment(Qt::AlignCenter);
            waiterLayout->addWidget(spinnerHolder);
        }
        void showWaiter(bool show)
        {
            if (waiter_)
                waiter_->setVisible(show);
        }
        void resizeWaiter(QSize size)
        {
            if (waiter_)
                waiter_->setFixedSize(size);
        }
    }
    
    ThemesWidget::ThemesWidget(QWidget* _parent, int _spacing) 
        : QWidget(_parent)
        , themesModel_(new ThemesModel(this))
        , flowLayout_(new FlowLayout(_spacing, _spacing, _spacing))
        , themes_list_(themes::themes_list())
        , loaded_themes_(QMap<int, bool>())
        , firstThemeAdded_(false)
    {
        flowLayout_->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));
        QVBoxLayout *root_layout = new QVBoxLayout();
        QScrollArea* area = new QScrollArea(this);
        QWidget* scroll_area = new QWidget(area);
        Utils::grabTouchWidget(scroll_area);
        area->setWidget(scroll_area);
        area->setWidgetResizable(true);
        grid_layout_ = new QGridLayout(this);
        grid_layout_->setSpacing(0);
        grid_layout_->setContentsMargins(0, 0, 0, 0);
        grid_layout_->addLayout(flowLayout_, 1, 0);
        grid_layout_->setAlignment(Qt::AlignTop);
        scroll_area->setLayout(grid_layout_);
        root_layout->addWidget(area);
        setLayout(root_layout);
        
        root_layout->setSpacing(0);
        root_layout->setContentsMargins(0, 0, 0, 0);
        
        initWaiter(this);
        showWaiter(true);
        
//        auto theme = std::make_shared<themes::theme>();
//        onThemeGot(theme);
    }
    
    ThemesWidget::~ThemesWidget()
    {
        waiterInited_ = false;
    }
    
    void ThemesWidget::resizeEvent(QResizeEvent *e)
    {
        QWidget::resizeEvent(e);
        resizeWaiter(e->size());
    }
    
    void ThemesWidget::addCaptionLayout(QLayout* _layout)
    {
        grid_layout_->addLayout(_layout, 0, 0);
    }

    void ThemesWidget::onThemeGot(themes::themePtr theme)
    {
        showWaiter(false);
        
        checkFirstTheme_(theme);
        int theme_id = theme->get_id();
        bool theme_loaded = loaded_themes_[theme_id];
        if (!theme_loaded)
        {
            QPixmap p = theme->get_thumb();
            ThemeWidget *themeWidget = new ThemeWidget(this, p, themesModel_, theme->get_id());
            flowLayout_->addWidget(themeWidget);
            update();
            loaded_themes_[theme_id] = true;
        }
    }
    
    void ThemesWidget::set_target_contact(QString _aimId)
    {
        themesModel_->set_target_contact(_aimId);
        int theme_id = get_qt_theme_settings()->themeIdForContact(_aimId);
        
        for (int i = 0; i < flowLayout_->count(); ++i)
        {
            QLayoutItem *layoutItem = flowLayout_->itemAt(i);
            if (dynamic_cast<QWidgetItem*>(layoutItem))
            {
                ThemeWidget *themeWidget = (ThemeWidget *)layoutItem->widget();
                bool shouldSelect = themeWidget->get_id() == theme_id;
                themeWidget->setBorder(shouldSelect);
            }
        }
    }
    
    void ThemesWidget::checkFirstTheme_(themes::themePtr theme)
    {
        if (!firstThemeAdded_)
        {
            firstThemeAdded_ = true;
            
            QSize properSize = theme->get_thumb().size();
            auto initialTheme = std::make_shared<themes::theme>();
            auto initialThemeThumb = initialTheme->get_thumb();
            auto correctedInitialThemeThumb = initialThemeThumb.scaled(properSize, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
            initialTheme->set_thumb(correctedInitialThemeThumb);
            onThemeGot(initialTheme);
        }
    }
};