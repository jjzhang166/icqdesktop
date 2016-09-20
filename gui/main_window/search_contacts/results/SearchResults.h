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

		void needMore(int _skipCount);
		void addContact(QString _contact);
		void msgContact(QString _contact);
		void callContact(QString _contact);
        void contactInfo(QString _contact);
        
	private Q_SLOTS:

		void onAddContact(QString _contact);
		void onMsgContact(QString _contact);
		void onCallContact(QString _contact);
        void onContactInfo(QString _contact);
		void onNeedMore(int _skip_count);

	private:

		QStackedWidget*							pages_;
		
		QVBoxLayout*							rootLayout_;

		NoResultsWidget*						noResultsWidget_;
		FoundContacts*							contactsWidget_;
									
	public:

		SearchResults(QWidget* _parent);
		virtual ~SearchResults();

		int insertItems(const profiles_list& _profiles);
		void contactAddResult(const QString& _contact, bool _res);
		void clear();
	};

}


