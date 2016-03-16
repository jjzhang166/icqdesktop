#pragma once

namespace Logic
{
	class contact_profile;
}

namespace Ui
{
	class NoResultsWidget;
	class FoundContacts;

	typedef std::list<std::shared_ptr<Logic::contact_profile>>	profiles_list;

	class SearchResults : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:

		void need_more(int _skip_count);
		void add_contact(QString _contact);
		void msg_contact(QString _contact);
		void call_contact(QString _contact);
        void contact_info(QString _contact);
        
	private Q_SLOTS:

		void on_add_contact(QString _contact);
		void on_msg_contact(QString _contact);
		void on_call_contact(QString _contact);
        void on_contact_info(QString _contact);
		void on_need_more(int _skip_count);

	private:

		QStackedWidget*							pages_;
		
		QVBoxLayout*							root_layout_;

		NoResultsWidget*						no_results_widget_;
		FoundContacts*							contacts_widget_;
									
	public:

		SearchResults(QWidget* _parent);
		virtual ~SearchResults();

		void insert_items(const profiles_list& profiles);
		void contact_add_result(const QString& _contact, bool _res);
		void clear();
	};

}


