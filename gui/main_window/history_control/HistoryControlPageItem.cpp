#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"

namespace Ui
{
	HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
		: QWidget(parent)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , Selected_(false)
	{
	}

    void HistoryControlPageItem::clearSelection()
    {
        if (Selected_)
        {
            update();
        }

        Selected_ = false;
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::select()
    {
        if (!Selected_)
        {
            update();
        }

        Selected_ = true;
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        HasTopMargin_ = value;

        updateGeometry();
    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;

        updateGeometry();
    }

//    void HistoryControlPageItem::resizeEvent(QResizeEvent *e)
//    {
//        QWidget::resizeEvent(e);
//
//        const auto &newSize = e->size();
//
//        if (LastSize_.height() != newSize.height())
//        {
//            emit heightChangedEvent(LastSize_, newSize);
//        }
//
//        LastSize_ = newSize;
//    }
//
//    void HistoryControlPageItem::showEvent(QShowEvent *e)
//    {
//        QWidget::showEvent(e);
//
//        if (LastSize_ != size())
//        {
//            //emit heightChangedEvent(QSize(0, 0), size());
//        }
//
//        LastSize_ = size();
//    }
    
    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }
    
    themes::themePtr HistoryControlPageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(aimId_);  //
    }
}