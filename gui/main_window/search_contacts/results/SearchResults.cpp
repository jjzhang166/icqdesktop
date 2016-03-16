#include "stdafx.h"
#include "SearchResults.h"
#include "NoResultsWidget.h"
#include "../../contact_list/contact_profile.h"
#include "FoundContacts.h"

namespace Ui
{
	SearchResults::SearchResults(QWidget* _parent)
		:	QWidget(_parent),
			pages_(new QStackedWidget(this)),
			root_layout_(new QVBoxLayout()),
			no_results_widget_(new NoResultsWidget(this)),
			contacts_widget_(new FoundContacts(this))
	{
		setLayout(root_layout_);
		root_layout_->setContentsMargins(0, 0, 0, 0);
		root_layout_->setSpacing(0);

		root_layout_->addWidget(pages_);

		pages_->addWidget(no_results_widget_);
		pages_->addWidget(contacts_widget_);

		pages_->setCurrentWidget(contacts_widget_);
		
		connect(contacts_widget_, SIGNAL(add_contact(QString)), this, SLOT(on_add_contact(QString)), Qt::QueuedConnection);
		connect(contacts_widget_, SIGNAL(msg_contact(QString)), this, SLOT(on_msg_contact(QString)), Qt::QueuedConnection);
		connect(contacts_widget_, SIGNAL(call_contact(QString)), this, SLOT(on_call_contact(QString)), Qt::QueuedConnection);
        connect(contacts_widget_, SIGNAL(contact_info(QString)), this, SLOT(on_contact_info(QString)), Qt::QueuedConnection);

		connect(contacts_widget_, SIGNAL(need_more(int)), this, SLOT(on_need_more(int)), Qt::QueuedConnection);
	}


	SearchResults::~SearchResults(void)
	{
	}
	
	
	void SearchResults::insert_items(const profiles_list& _profiles)
	{
		if (!_profiles.empty())
		{
			pages_->setCurrentWidget(contacts_widget_);
			contacts_widget_->insert_items(_profiles);	
		}
		else
		{
			if (contacts_widget_->empty())
			{
				pages_->setCurrentWidget(no_results_widget_);
			}
		}
	}

	void SearchResults::contact_add_result(const QString& _contact, bool _res)
	{
		contacts_widget_->contact_add_result(_contact, _res);
	}

	void SearchResults::clear()
	{
		contacts_widget_->clear();
	}

	void SearchResults::on_add_contact(QString _contact)
	{
		emit add_contact(_contact);
	}

	void SearchResults::on_msg_contact(QString _contact)
	{
		emit msg_contact(_contact);
	}

	void SearchResults::on_call_contact(QString _contact)
	{
		emit call_contact(_contact);
	}

	void SearchResults::on_need_more(int _skip_count)
	{
		emit need_more(_skip_count);
	}

    void SearchResults::on_contact_info(QString _contact)
    {
        emit contact_info(_contact);
    }
}
