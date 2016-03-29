#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "RecentMessagesAlert.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../../utils/utils.h"
#include "../../controls/LabelEx.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

namespace
{
	const int max_alerts_count = 3;
	const int alert_visible_time = 5000; //5 sec
	const int alert_hide_animation_time = 2000; //2 sec
	const int view_all_widget_height = 40;
	const int bottom_space_height = 16;
	const int header_height = 32;
}

namespace Ui
{
	RecentMessagesAlert::RecentMessagesAlert(Logic::RecentItemDelegate* delegate)
		: QWidget(0, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
		, Delegate_(delegate)
		, Layout_(new QVBoxLayout())
		, CloseButton_(new QPushButton(this))
		, AlertsCount_(0)
		, Timer_(new QTimer(this))
		, ViewAllWidget_(new QWidget(this))
		, Height_(0)
		, CursorIn_(false)
		, ViewAllWidgetVisible_(false)
		, Animation_(new QPropertyAnimation(this, "windowOpacity"))
	{
		init();
	}

	RecentMessagesAlert::~RecentMessagesAlert()
	{

	}

	void RecentMessagesAlert::enterEvent(QEvent* e)
	{
		CursorIn_ = true;
		setWindowOpacity(1);
		Animation_->stop();
		Timer_->start();
		return QWidget::enterEvent(e);
	}

	void RecentMessagesAlert::leaveEvent(QEvent* e)
	{
		CursorIn_ = false;
		return QWidget::leaveEvent(e);
	}

    void RecentMessagesAlert::mouseReleaseEvent(QMouseEvent* e)
    {
        QPoint p = e->pos();
        if (p.y() >= CloseButton_->y() && p.y() <= CloseButton_->y() + CloseButton_->height())
        {
            QLayoutItem* item = Layout_->itemAt(1);
            if (item->widget())
            {
                if (Ui::MessageAlertWidget* alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget()))
                {
                    messageAlertClicked(alert->id());
                }
            }
        }
        return QWidget::mouseReleaseEvent(e);
    }

	void RecentMessagesAlert::init()
	{
		setAttribute(Qt::WA_ShowWithoutActivating);
		QVBoxLayout* topLayout = new QVBoxLayout();
		QWidget* topWidget = new QWidget(this);
		setLayout(topLayout);
		topLayout->setMargin(0);
		topLayout->setSpacing(0);
		topLayout->addWidget(topWidget);

		topWidget->setProperty("RecentMessageAlert", true);
		topWidget->setStyle(QApplication::style());
		topWidget->setLayout(Layout_);

		QHBoxLayout* layout = new QHBoxLayout();
		CloseButton_->setProperty("TrayCloseButton", true);
		CloseButton_->setStyle(QApplication::style());
		CloseButton_->setFixedHeight(Utils::scale_value(header_height));
		layout->setAlignment(Qt::AlignRight);
		layout->addWidget(CloseButton_);
		layout->setMargin(0);
		layout->setSpacing(0);

		Layout_->addLayout(layout);
		Layout_->setSpacing(0);
		Layout_->setMargin(0);
		Height_ += CloseButton_->height();

		Height_ += Ui::get_gui_settings()->get_shadow_width() * 2;

		QVBoxLayout* viewAllLayout = new QVBoxLayout();
		viewAllLayout->setSpacing(0);
		viewAllLayout->setMargin(0);
		viewAllLayout->addWidget(ViewAllWidget_);
		ViewAllWidget_->setFixedSize(Delegate_->sizeHintForAlert().width(), Utils::scale_value(view_all_widget_height));
		ViewAllWidget_->setContentsMargins(QMargins(Utils::scale_value(48 + 32), 0, Utils::scale_value(16), 0));

		QWidget* lineWidget = new QWidget();
		lineWidget->setFixedSize(ViewAllWidget_->contentsRect().width(), Utils::scale_value(1));
		lineWidget->setProperty("Line", true);
		lineWidget->setStyle(QApplication::style());

		QVBoxLayout* widgetLayout = new QVBoxLayout();
		widgetLayout->setAlignment(Qt::AlignTop);
		widgetLayout->setMargin(0);
		widgetLayout->setSpacing(Utils::scale_value(15));
		widgetLayout->addWidget(lineWidget);

		LabelEx* viewAllLabel = new LabelEx(this);
		viewAllLabel->setProperty("ViewAllLink", true);
		viewAllLabel->setStyle(QApplication::style());
		viewAllLabel->setText(QT_TRANSLATE_NOOP("notifications_alert", "View all"));
		viewAllLabel->setCursor(QCursor(Qt::PointingHandCursor));
		widgetLayout->addWidget(viewAllLabel);
		ViewAllWidget_->setLayout(widgetLayout);
		Layout_->addWidget(ViewAllWidget_);

		QWidget* space = new QWidget(this);
		space->setFixedSize(Delegate_->sizeHintForAlert().width() + Ui::get_gui_settings()->get_shadow_width() * 2, Utils::scale_value(bottom_space_height));
		Layout_->addWidget(space);
		Height_ += Utils::scale_value(bottom_space_height);

		ViewAllWidget_->hide();

		setFixedSize(Delegate_->sizeHintForAlert().width() + Ui::get_gui_settings()->get_shadow_width() * 2, Height_);

		Timer_->setInterval(alert_visible_time);
        Timer_->setSingleShot(true);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(startAnimation()), Qt::QueuedConnection);
		connect(CloseButton_, SIGNAL(clicked()), this, SLOT(closeAlert()), Qt::QueuedConnection);
		connect(CloseButton_, SIGNAL(clicked()), this, SLOT(statsCloseAlert()), Qt::QueuedConnection);

		connect(viewAllLabel, SIGNAL(clicked()), this, SLOT(viewAll()), Qt::QueuedConnection);

		Utils::addShadowToWindow(this);

		Animation_->setDuration(alert_hide_animation_time);
		Animation_->setStartValue(1);
		Animation_->setEndValue(0);
		connect(Animation_, SIGNAL(finished()), this, SLOT(closeAlert()), Qt::QueuedConnection);
	}

	void RecentMessagesAlert::startAnimation()
	{
#ifdef __linux__
        closeAlert();
        return;
#endif //__linux__
		if (CursorIn_)
			Timer_->start();
		else
			Animation_->start();
	}

    void RecentMessagesAlert::messageAlertClicked(QString aimId)
    {
        closeAlert();
        emit messageClicked(aimId);
    }

	void RecentMessagesAlert::closeAlert()
	{
		hide();
		setWindowOpacity(1);
		Animation_->stop();
		markShowed();
	}
    
    void RecentMessagesAlert::statsCloseAlert()
	{
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_close);
	}

	void RecentMessagesAlert::viewAll()
	{
		messageAlertClicked(QString());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_viewall);
	}

	void RecentMessagesAlert::addAlert(const Data::DlgState& state)
	{
		bool showViewAll = (AlertsCount_ == max_alerts_count);
		int i = 0;
		while (QLayoutItem* item = Layout_->itemAt(i))
		{
			if (item->widget())
			{
				if (Ui::MessageAlertWidget* alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget()))
				{
					if (alert->id() == state.AimId_)
					{
						Height_ -= Delegate_->sizeHintForAlert().height();
						Layout_->removeWidget(item->widget());
						alert->deleteLater();
						showViewAll = ViewAllWidget_->isVisible() && AlertsCount_ == max_alerts_count;
						--AlertsCount_;
					}
				}
			}
			++i;
		}

		if (AlertsCount_ == max_alerts_count)
		{
			QLayoutItem* item = Layout_->itemAt(AlertsCount_);
			QWidget* w = item ? item->widget() : 0;
			if (w)
			{
				Height_ -= Delegate_->sizeHintForAlert().height();
				Layout_->removeWidget(w);
				w->deleteLater();
				--AlertsCount_;
			}
		}

		MessageAlertWidget* widget = new MessageAlertWidget(state, Delegate_, this);
		connect(widget, SIGNAL(clicked(QString)), this, SLOT(messageAlertClicked(QString)), Qt::DirectConnection);
		Height_ +=Delegate_->sizeHintForAlert().height();
		Layout_->insertWidget(1, widget);
		++AlertsCount_;

		if (showViewAll)
		{
			if (!ViewAllWidgetVisible_)
			{
				ViewAllWidgetVisible_ = true;
				ViewAllWidget_->show();
				Height_ += Utils::scale_value(view_all_widget_height);
			}
		}
		else
		{
			if (ViewAllWidgetVisible_)
			{
				ViewAllWidgetVisible_ = false;
				ViewAllWidget_->hide();
				Height_ -= Utils::scale_value(view_all_widget_height);
			}
		}

		setFixedHeight(Height_);
		setWindowOpacity(1);
		Animation_->stop();
		Timer_->start();
	}

	void RecentMessagesAlert::markShowed()
	{
		int i = 0;
		while (QLayoutItem* item = Layout_->itemAt(i))
		{
			if (item->widget())
			{
				if (Ui::MessageAlertWidget* alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget()))
				{
					Height_ -= Delegate_->sizeHintForAlert().height();
					Layout_->removeWidget(item->widget());
					alert->deleteLater();
					--AlertsCount_;
				}
			}
			++i;
		}
		
		if (ViewAllWidgetVisible_)
		{
			ViewAllWidgetVisible_ = false;
			ViewAllWidget_->hide();
			Height_ -= Utils::scale_value(view_all_widget_height);
		}

		setFixedHeight(Height_);
	}
}
