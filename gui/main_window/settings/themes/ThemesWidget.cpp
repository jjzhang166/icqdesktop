#include "stdafx.h"
#include "ThemesWidget.h"

#include "ThemeWidget.h"
#include "ThemesModel.h"
#include "../../../theme_settings.h"
#include "../../../controls/FlowLayout.h"
#include "../../../controls/TransparentScrollBar.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/BackButton.h"

#include "../../contact_list/RecentsModel.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/utils.h"

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
            Utils::grabTouchWidget(spinnerHolder);
            waiterLayout->addWidget(spinnerHolder);
        }
        void showWaiter(bool _show)
        {
            if (waiter_)
                waiter_->setVisible(_show);
        }
        void resizeWaiter(QSize _size)
        {
            if (waiter_)
                waiter_->setFixedSize(_size);
        }
    }

    ThemesWidget::ThemesWidget(QWidget* _parent, int _spacing)
        : QWidget(_parent)
        , themesModel_(new ThemesModel(this))
        , flowLayout_(new FlowLayout(_spacing, _spacing, _spacing))
        , themesList_(themes::themesList())
        , loadedThemes_(QMap<int, bool>())
        , firstThemeAdded_(false)
    {
        flowLayout_->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));
        QVBoxLayout *rootLayout = new QVBoxLayout();
        auto area = CreateScrollAreaAndSetTrScrollBar(this);
        QWidget* scrollArea = new QWidget(area);
        Utils::grabTouchWidget(area->viewport(), true);
        Utils::grabTouchWidget(area);
        Utils::grabTouchWidget(scrollArea);
        connect(QScroller::scroller(area->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        area->setWidget(scrollArea);
        area->setWidgetResizable(true);
        auto layout = new QVBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignTop);
        scrollArea->setLayout(layout);
        rootLayout->addWidget(area);
        setLayout(rootLayout);

        rootLayout->setSpacing(0);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        auto themesCaption = new TextEmojiWidget(this, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(24), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(48));
        themesCaption->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
        themesCaption->setFixedHeight(Utils::scale_value(80));

        backButton_ = new BackButton(this);
        backButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        backButton_->setFlat(true);
        backButton_->setFocusPolicy(Qt::NoFocus);
        backButton_->setCursor(Qt::PointingHandCursor);

        connect(backButton_, SIGNAL(clicked()), this, SLOT(backPressed()));

        auto themeCaptionLayout_ = new QHBoxLayout();
        themeCaptionLayout_->setContentsMargins(Utils::scale_value(24), Utils::scale_value(0), 0, 0);
        themeCaptionLayout_->setSpacing(0);

        captionWithoutBackButtonSpacer_ = new QWidget(this);
        captionWithoutBackButtonSpacer_->setFixedWidth(Utils::scale_value(24));
        captionWithoutBackButtonSpacer_->setFixedHeight(Utils::scale_value(80));
        captionWithoutBackButtonSpacer_->setStyleSheet("background-color: white;");
        captionWithoutBackButtonSpacer_->setVisible(false);

        Utils::grabTouchWidget(captionWithoutBackButtonSpacer_);
        themeCaptionLayout_->addWidget(captionWithoutBackButtonSpacer_);
        themeCaptionLayout_->addWidget(backButton_);

        backButtonAndCaptionSpacer_ = new QWidget(this);
        backButtonAndCaptionSpacer_->setFixedWidth(Utils::scale_value(15));
        backButtonAndCaptionSpacer_->setFixedHeight(Utils::scale_value(80));
        backButtonAndCaptionSpacer_->setStyleSheet("background-color: white;");

        Utils::grabTouchWidget(backButtonAndCaptionSpacer_);
        themeCaptionLayout_->addWidget(backButtonAndCaptionSpacer_);
        Utils::grabTouchWidget(themesCaption);
        themeCaptionLayout_->addWidget(themesCaption);

        layout->addLayout(themeCaptionLayout_);
        layout->addLayout(flowLayout_);

        initWaiter(this);
        showWaiter(true);
    }

    ThemesWidget::~ThemesWidget()
    {
        waiterInited_ = false;
    }

    void ThemesWidget::setBackButton(bool _doSet)
    {
        backButton_->setVisible(_doSet);
        backButtonAndCaptionSpacer_->setVisible(_doSet);
        captionWithoutBackButtonSpacer_->setVisible(!_doSet);
    }

    void ThemesWidget::resizeEvent(QResizeEvent* _e)
    {
        QWidget::resizeEvent(_e);
        resizeWaiter(_e->size());
    }

    void ThemesWidget::touchScrollStateChanged(QScroller::State _st)
    {
        for (int i = 0; i < flowLayout_->count(); ++i)
        {
            QLayoutItem *layoutItem = flowLayout_->itemAt(i);
            if (QWidget* w = layoutItem->widget())
            {
                w->setAttribute(Qt::WA_TransparentForMouseEvents, _st != QScroller::Inactive);
            }
        }
    }

    void ThemesWidget::onThemeGot(themes::themePtr _theme)
    {
        showWaiter(false);

        checkFirstTheme_(_theme);
        int themeId = _theme->get_id();
        bool themeLoaded = loadedThemes_[themeId];
        if (!themeLoaded)
        {
            QPixmap p = _theme->getThumb();
            ThemeWidget *themeWidget = new ThemeWidget(this, p, themesModel_, _theme->get_id());
            Utils::grabTouchWidget(themeWidget);
            flowLayout_->addWidget(themeWidget);
            update();
            loadedThemes_[themeId] = true;
        }
    }

    void ThemesWidget::setTargetContact(QString _aimId)
    {
        themesModel_->setTargetContact(_aimId);
        int themeId = get_qt_theme_settings()->themeIdForContact(_aimId);

        for (int i = 0; i < flowLayout_->count(); ++i)
        {
            QLayoutItem *layoutItem = flowLayout_->itemAt(i);
            if (layoutItem->widget())
            {
                ThemeWidget *themeWidget = qobject_cast<ThemeWidget *>(layoutItem->widget());
                bool shouldSelect = themeWidget->get_id() == themeId;
                themeWidget->setBorder(shouldSelect);
            }
        }
    }

    void ThemesWidget::checkFirstTheme_(themes::themePtr _theme)
    {
        if (!firstThemeAdded_)
        {
            firstThemeAdded_ = true;

            QSize properSize = _theme->getThumb().size();
            auto initialTheme = std::make_shared<themes::theme>();
            auto initialThemeThumb = initialTheme->getThumb();
            auto correctedInitialThemeThumb = initialThemeThumb.scaled(properSize, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
            initialTheme->setThumb(correctedInitialThemeThumb);
            onThemeGot(initialTheme);
        }
    }

    void ThemesWidget::backPressed()
    {
        Logic::getRecentsModel()->sendLastRead();
        emit Utils::InterConnector::instance().themesSettingsBack();
    }
};