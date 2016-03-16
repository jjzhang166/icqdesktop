#pragma once

#include "../../utils/gui_coll_helper.h"
#include "search_params.h"

namespace Ui
{
	class SearchFilters;
	class SearchResults;

	class SearchContactsWidget : public QWidget
	{
		Q_OBJECT

		QVBoxLayout*			root_layout_;
		SearchFilters*			filters_widget_;
		SearchResults*			results_widget_;

		search_params			active_filters_;

		bool					request_in_progress_;
		bool					no_more_items_;

		std::shared_ptr<bool>	ref_;

		void on_search_result(gui_coll_helper _coll);
        void on_search_result2(gui_coll_helper _coll);
		void search(const search_params& _filters);
        void search2(const std::string& keyword, const std::string& phonenumber, const std::string& tag);

	private Q_SLOTS:

		void on_search(search_params _filters);
        void on_search2(search_params _filters);
		void on_need_more_results(int _skip_count);
		void on_add_contact(QString _contact);
		void on_msg_contact(QString _contact);
		void on_call_contact(QString _contact);
        void on_contact_info(QString _contact);

	protected:

		virtual void paintEvent(QPaintEvent* _e) override;

	public:

		void on_focus();

		SearchContactsWidget(QWidget* _parent);
		virtual ~SearchContactsWidget(void);
	};

}

