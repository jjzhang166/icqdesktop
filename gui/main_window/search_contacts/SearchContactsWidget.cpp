#include "stdafx.h"
#include "SearchContactsWidget.h"
#include "SearchFilters.h"
#include "results/SearchResults.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/ContactListModel.h"
#include "../../../corelib/enumerations.h"

//#include "mac_support.h"

namespace Ui
{
	SearchContactsWidget::SearchContactsWidget(QWidget* _parent)
		:	QWidget(_parent)
			, root_layout_(new QVBoxLayout(this))
			, filters_widget_(new SearchFilters(this))
			, results_widget_(new SearchResults(this))
			, request_in_progress_(false)
			, no_more_items_(false)
			, ref_(new bool(false))
	{
		setStyleSheet(Utils::LoadStyle(":/main_window/search_contacts/search_contacts.qss", Utils::get_scale_coefficient(), true));
		
		root_layout_->setContentsMargins(Utils::scale_value(48), 0, 0, 0);
		root_layout_->setSpacing(0);
		root_layout_->setAlignment(Qt::AlignTop);
        
		QLineEdit* caption = new QLineEdit(this);
        
		caption->setObjectName("caption");
		caption->setText(QT_TRANSLATE_NOOP("search_widget","Add contact"));
		root_layout_->addWidget(caption);

		root_layout_->addWidget(filters_widget_);
		filters_widget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		root_layout_->addWidget(results_widget_);
		root_layout_->setStretch(root_layout_->count() - 1, 1);
				
		results_widget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

		setLayout(root_layout_);

		connect(filters_widget_, SIGNAL(onSearch(search_params)), this, SLOT(on_search(search_params)));
        //connect(filters_widget_, SIGNAL(onSearch(search_params)), this, SLOT(on_search2(search_params)));
        
		connect(results_widget_, SIGNAL(need_more(int)), this, SLOT(on_need_more_results(int)), Qt::QueuedConnection);
		connect(results_widget_, SIGNAL(add_contact(QString)), this, SLOT(on_add_contact(QString)), Qt::QueuedConnection);
		connect(results_widget_, SIGNAL(msg_contact(QString)), this, SLOT(on_msg_contact(QString)), Qt::QueuedConnection);
		connect(results_widget_, SIGNAL(call_contact(QString)), this, SLOT(on_call_contact(QString)), Qt::QueuedConnection);
        connect(results_widget_, SIGNAL(contact_info(QString)), this, SLOT(on_contact_info(QString)), Qt::QueuedConnection);
	}

	
	SearchContactsWidget::~SearchContactsWidget(void)
	{
	}

	void SearchContactsWidget::on_focus()
	{
		filters_widget_->on_focus();
	}

	void SearchContactsWidget::paintEvent(QPaintEvent* _e)
	{
		QStyleOption opt;
		opt.init(this);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

		return QWidget::paintEvent(_e);
	}

	void SearchContactsWidget::on_search_result(gui_coll_helper _coll)
	{
        filters_widget_->on_search_results();

		int32_t err = _coll.get_value_as_int("error");

		profiles_list profiles;

		if (err == 0)
		{
			core::ifptr<core::iarray> res_array(_coll.get_value_as_array("results"), false);

			if (active_filters_.get_count() > res_array->size())
				no_more_items_ = true;

			for (int32_t i = 0; i < res_array->size(); ++i)
			{
				gui_coll_helper coll_profile(res_array->get_at(i)->get_as_collection(), false);

				auto profile = std::make_shared<Logic::contact_profile>();

				if (profile->unserialize(coll_profile))
				{
					profiles.push_back(profile);
				}
			}
		}

		results_widget_->insert_items(profiles);
	}

    void SearchContactsWidget::on_search_result2(gui_coll_helper _coll)
    {
        filters_widget_->on_search_results();

        int32_t error = _coll.get_value_as_int("error");
        profiles_list profiles;
        if (error == 0)
        {
            core::ifptr<core::iarray> res_array(_coll.get_value_as_array("data"), false);
            
            if (active_filters_.get_count() > res_array->size())
                no_more_items_ = true;
            
            for (int32_t i = 0; i < res_array->size(); ++i)
            {
                gui_coll_helper coll_profile(res_array->get_at(i)->get_as_collection(), false);
                
                auto profile = std::make_shared<Logic::contact_profile>();
                
                if (profile->unserialize2(coll_profile))
                {
                    profiles.push_back(profile);
                }
            }
        }
        
        results_widget_->insert_items(profiles);
    }

	void SearchContactsWidget::on_search(search_params _filters)
	{
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::search);
		active_filters_ = _filters;

		no_more_items_ = false;

		search(active_filters_);

		results_widget_->clear();
	}

    void SearchContactsWidget::on_search2(search_params _filters)
    {
        active_filters_ = _filters;
        
        no_more_items_ = false;
        
        search2(active_filters_.get_keyword().toStdString(), "", "");
        
        results_widget_->clear();
    }

	void SearchContactsWidget::search(const search_params& _filters)
	{
		Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

		_filters.serialize(collection);

		request_in_progress_ = true;

		std::weak_ptr<bool> wr_ref = ref_;

		Ui::GetDispatcher()->post_message_to_core("contacts/search", collection.get(), [this, wr_ref](core::icollection* _coll)
		{
			auto ref = wr_ref.lock();
			if (!ref)
				return;

			request_in_progress_ = false;

			gui_coll_helper coll(_coll, false);

			on_search_result(coll);
		});
	}

    void SearchContactsWidget::search2(const std::string& keyword, const std::string& phonenumber, const std::string& tag)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("keyword", keyword);
        collection.set_value_as_string("phonenumber", phonenumber);
        collection.set_value_as_string("tag", tag);
        request_in_progress_ = true;
        std::weak_ptr<bool> wr_ref = ref_;
        Ui::GetDispatcher()->post_message_to_core("contacts/search2", collection.get(), [this, wr_ref](core::icollection* _coll)
        {
            auto ref = wr_ref.lock();
            if (!ref)
                return;
            request_in_progress_ = false;
            gui_coll_helper coll(_coll, false);
            on_search_result2(coll);
        });
    }

	void SearchContactsWidget::on_need_more_results(int32_t _skip_count)
	{
		if (request_in_progress_ || no_more_items_)
			return;

		active_filters_.set_skip_count(_skip_count);

		search(active_filters_);
	}

	void SearchContactsWidget::on_add_contact(QString _contact)
	{
        std::weak_ptr<bool> wr_ref = ref_;
		Logic::GetContactListModel()->add_contact_to_contact_list(_contact, [this, wr_ref, _contact](bool _res)
		{
            auto ref = wr_ref.lock();
            if (!ref)
                return;

			results_widget_->contact_add_result(_contact, _res);

			Logic::GetContactListModel()->setCurrent(_contact, true);			
		});
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_search_results);
	}

	void SearchContactsWidget::on_msg_contact(QString _contact)
	{
		Logic::GetContactListModel()->setCurrent(_contact, true);
	}

	void SearchContactsWidget::on_call_contact(QString _contact)
	{
        Logic::GetContactListModel()->setCurrent(_contact, true);
        Ui::GetDispatcher()->getVoipController().setStartA(_contact.toUtf8(), false);
	}

    void SearchContactsWidget::on_contact_info(QString _contact)
    {
        emit Utils::InterConnector::instance().profileSettingsShow(_contact);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_search_results);
    }
}