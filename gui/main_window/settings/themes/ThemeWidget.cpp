#include "stdafx.h"
#include "ThemeWidget.h"
#include "ThemesModel.h"
#include "../../../controls/CustomButton.h"
#include "../../../utils/utils.h"

namespace Ui
{
    ThemeWidget::ThemeWidget(QWidget* _parent, QPixmap& _pixmap, ThemesModel* _themesModel, int _themeId)
        : QWidget(_parent)
        , pixmap_(QPixmap(_pixmap))
        , themesModel_(_themesModel)
        , themeId_(_themeId)
    {
        CustomButton *themeButton = new CustomButton(this, pixmap_);
        
        borderWidget_ = new QWidget(this);
        int w = _pixmap.width() / Utils::scale_bitmap(1);
        int h = _pixmap.height() / Utils::scale_bitmap(1);
        setFixedWidth(w);
        setFixedHeight(h);
        themeButton->setFixedWidth(w);
        themeButton->setFixedHeight(h);
        themeButton->setCursor(Qt::PointingHandCursor);
        borderWidget_->setFixedWidth(w);
        borderWidget_->setFixedHeight(h);
        
        connect(themeButton, SIGNAL(clicked()), this, SLOT(onThemePressed()));
        Utils::ApplyStyle(borderWidget_, "background-color: transparent; border-style: solid; border-color: #579e1c; border-width: 4dip;");
        
        CustomButton *mark = new CustomButton(this, ":/resources/contr_wallpaper_select_100.png");
        Utils::ApplyStyle(mark, "background-color: transparent; border-color: transparent;");
        mark->setFixedWidth(Utils::scale_value(32));
        mark->setFixedHeight(Utils::scale_value(32));
        QVBoxLayout* vlayout = new QVBoxLayout();
        QHBoxLayout* hlayout = new QHBoxLayout();
        vlayout->setContentsMargins(0, 0, 0, 0);
        hlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->setSpacing(0);
        hlayout->setSpacing(0);
        vlayout->setAlignment(Qt::AlignTop);
        hlayout->setAlignment(Qt::AlignRight);
        vlayout->addLayout(hlayout);
        hlayout->addWidget(mark);
        borderWidget_->setLayout(vlayout);
        borderWidget_->setAttribute( Qt::WA_TransparentForMouseEvents);

        Utils::grabTouchWidget(themeButton);
        Utils::grabTouchWidget(mark);
        
        setBorder(false);
    }
    
    void ThemeWidget::onThemePressed()
    {
        themesModel_->themeSelected(themeId_);
    }
    
    void ThemeWidget::setBorder(const bool _visible)
    {
        borderWidget_->setVisible(_visible);
    }
}