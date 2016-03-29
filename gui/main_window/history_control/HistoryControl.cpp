#include "stdafx.h"
#include "HistoryControl.h"

#include "HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../utils/InterConnector.h"
#include "../MainPage.h"
#include "../MainWindow.h"

namespace Ui
{
	HistoryControl::HistoryControl(QWidget* parent)
		: QWidget(parent)
		, timer_(new QTimer(this))
	{
        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("history_control"));
        this->resize(504, 404);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
        this->setSizePolicy(sizePolicy);
        this->setProperty("HistoryControl", QVariant(true));
        vertical_layout_ = new QVBoxLayout(this);
        vertical_layout_->setSpacing(0);
        vertical_layout_->setObjectName(QStringLiteral("verticalLayout_2"));
        vertical_layout_->setContentsMargins(0, 0, 0, 0);
        stacked_widget_ = new QStackedWidget(this);
        stacked_widget_->setObjectName(QStringLiteral("stackedWidget"));
        sizePolicy.setHeightForWidth(stacked_widget_->sizePolicy().hasHeightForWidth());
        stacked_widget_->setSizePolicy(sizePolicy);
        page_ = new QWidget();
        page_->setObjectName(QStringLiteral("page"));
        sizePolicy.setHeightForWidth(page_->sizePolicy().hasHeightForWidth());
        page_->setSizePolicy(sizePolicy);
        vertical_layout_2_ = new QVBoxLayout(page_);
        vertical_layout_2_->setSpacing(0);
        vertical_layout_2_->setObjectName(QStringLiteral("verticalLayout_3"));
        vertical_layout_2_->setContentsMargins(0, 0, 0, 0);
        stacked_widget_->addWidget(page_);
        vertical_layout_->addWidget(stacked_widget_);
        QMetaObject::connectSlotsByName(this);


		timer_->setInterval(60000);
		timer_->setSingleShot(false);
		connect(timer_, SIGNAL(timeout()), this, SLOT(updatePages()), Qt::QueuedConnection);
		timer_->start();

		connect(Logic::GetContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(close_dialog(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(activeDialogHide(QString)), this, SLOT(close_dialog(QString)), Qt::QueuedConnection);
	}

	HistoryControl::~HistoryControl()
	{
	}

    void HistoryControl::cancelSelection()
    {
        for (auto &page : pages_)
        {
            assert(page);
            page->cancelSelection();
        }
    }

    HistoryControlPage* HistoryControl::getHistoryPage(const QString& aimId) const
    {
        QMap<QString, HistoryControlPage*>::const_iterator iter = pages_.find(aimId);
        if (iter == pages_.end())
        {
            return 0;
        }

        return *iter;
    }

	void HistoryControl::updatePages()
	{
		for (QMap<QString, QTime>::iterator iter = times_.begin(); iter != times_.end(); )
		{
			if (iter.value().secsTo(QTime::currentTime()) >= 300 && iter.key() != current_)
			{
				close_dialog(iter.key());
 				iter = times_.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	void HistoryControl::contactSelected(QString _aimId)
	{
        assert(!_aimId.isEmpty());
        if (!Logic::GetContactListModel()->getContactItem(_aimId))
            return;

        HistoryControlPage* oldPage = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
		if (oldPage)
		{
            if (oldPage->aimId() == _aimId)
                return;

			oldPage->updateNewPlate(true);
		}

		Logic::GetContactListModel()->setCurrent(_aimId);
		QMap<QString, HistoryControlPage*>::iterator iter = pages_.find(_aimId);
		if (iter == pages_.end())
		{
            HistoryControlPage *newPage = new HistoryControlPage(this, _aimId);
			iter = pages_.insert(_aimId, newPage);
			connect((*iter), SIGNAL(quote(QString)), this, SIGNAL(quote(QString)), Qt::QueuedConnection);
			stacked_widget_->addWidget(*iter);
            
            Logic::GetContactListModel()->setCurrentCallbackHappened(newPage);
		}

		stacked_widget_->setCurrentWidget(*iter);
		(*iter)->updateNewPlate(false);
		(*iter)->open();

		if (!current_.isEmpty())
			times_[current_] = QTime::currentTime();
		current_ = _aimId;

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
        
		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core("dialogs/add", collection.get());
	}

	void HistoryControl::close_dialog(const QString& _aimId)
	{
		auto iter_page = pages_.find(_aimId);
		if (iter_page == pages_.end())
		{
			return;
		}

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core("dialogs/remove", collection.get());
		Logic::GetMessagesModel()->removeDialog(_aimId);
		stacked_widget_->removeWidget(iter_page.value());

        if (current_ == _aimId)
        {
            stacked_widget_->setCurrentIndex(0);
            Ui::MainPage::instance()->hideInput();
        }

		delete iter_page.value();
		pages_.erase(iter_page);
	}
    
    const QString & HistoryControl::getCurrent()
    {
        return current_;
    }
}