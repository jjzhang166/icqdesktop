#include "stdafx.h"
#include "HistoryControl.h"

#include "HistoryControlPage.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"

namespace Ui
{
	HistoryControl::HistoryControl(QWidget* parent)
		: QWidget(parent)
		, timer_(new QTimer(this))
	{
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
        this->setSizePolicy(sizePolicy);
        vertical_layout_ = new QVBoxLayout(this);
        vertical_layout_->setSpacing(0);
        vertical_layout_->setContentsMargins(0, 0, 0, 0);
        stacked_widget_ = new QStackedWidget(this);
        sizePolicy.setHeightForWidth(stacked_widget_->sizePolicy().hasHeightForWidth());
        stacked_widget_->setSizePolicy(sizePolicy);
        page_ = new QWidget();
        sizePolicy.setHeightForWidth(page_->sizePolicy().hasHeightForWidth());
        page_->setSizePolicy(sizePolicy);
        vertical_layout_2_ = new QVBoxLayout(page_);
        vertical_layout_2_->setSpacing(0);
        vertical_layout_2_->setContentsMargins(0, 0, 0, 0);
        stacked_widget_->addWidget(page_);
        vertical_layout_->addWidget(stacked_widget_);
        QMetaObject::connectSlotsByName(this);

		timer_->setInterval(60000);
		timer_->setSingleShot(false);
		connect(timer_, SIGNAL(timeout()), this, SLOT(updatePages()), Qt::QueuedConnection);
		timer_->start();

        connect(Logic::getContactListModel(), SIGNAL(leave_dialog(QString)), this, SLOT(leave_dialog(QString)), Qt::QueuedConnection);
		connect(Logic::getContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(close_dialog(QString)), Qt::QueuedConnection);
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

    void HistoryControl::updateCurrentPage()
    {
        auto page = getCurrentPage();
        if (page)
        {
            page->updateMoreButton();
        }
    }

    void HistoryControl::notifyApplicationWindowActive(const bool isActive)
    {
        for (auto &page : pages_)
        {
            assert(page);

            if (isActive)
            {
                page->resumeVisibleItems();
            }
            else
            {
                page->suspendVisisbleItems();
            }
        }
    }

    void HistoryControl::scrollHistoryToBottom(QString _contact)
    {
        auto page = getHistoryPage(_contact);
        if (page)
            page->scrollToBottom();
    }

    void HistoryControl::mouseReleaseEvent(QMouseEvent *e)
    {
        QWidget::mouseReleaseEvent(e);
        HistoryControlPage* page = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
        if (page)
            emit clicked();
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
        if (!Logic::getContactListModel()->getContactItem(_aimId))
            return;

        HistoryControlPage* oldPage = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
		if (oldPage)
		{
            if (oldPage->aimId() == _aimId)
                return;

			oldPage->updateNewPlate(true);
		}

        emit Utils::InterConnector::instance().historyControlReady(_aimId);
		auto iter = pages_.find(_aimId);
        const auto createNewPage = (iter == pages_.end());
		if (createNewPage)
		{
            auto newPage = new HistoryControlPage(this, _aimId);

			iter = pages_.insert(_aimId, newPage);
			connect((*iter), SIGNAL(quote(QList<Data::Quote>)), this, SIGNAL(quote(QList<Data::Quote>)), Qt::QueuedConnection);
            //connect((*iter), SIGNAL(forward(QList<Data::Quote>)), this, SIGNAL(forward(QList<Data::Quote>)), Qt::QueuedConnection);

			stacked_widget_->addWidget(*iter);

            Logic::getContactListModel()->setCurrentCallbackHappened(newPage);
		}

        auto contactPage = *iter;

		stacked_widget_->setCurrentWidget(contactPage);
		contactPage->updateNewPlate(false);
		contactPage->open();

        for (auto page : pages_)
        {
            const auto isBackgroundPage = (contactPage != page);
            if (isBackgroundPage)
            {
                page->suspendVisisbleItems();
            }
        }

		if (!current_.isEmpty())
			times_[current_] = QTime::currentTime();
		current_ = _aimId;

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core("dialogs/add", collection.get());
	}

    void HistoryControl::leave_dialog(const QString& _aimId)
    {
        auto iter_page = pages_.find(_aimId);
        if (iter_page == pages_.end())
        {
            return;
        }
        
        if (current_ == _aimId)
        {
            stacked_widget_->setCurrentIndex(0);
            Ui::MainPage::instance()->hideInput();
        }
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

    HistoryControlPage* HistoryControl::getCurrentPage() const
    {
        assert(stacked_widget_);

        return qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
    }
}