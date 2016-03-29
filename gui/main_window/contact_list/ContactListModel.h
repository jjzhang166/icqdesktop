#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

#include "ContactItem.h"

namespace core
{
	struct icollection;
}

namespace Ui
{
    class HistoryControlPage;
}

namespace Logic
{
	class contact_profile;
	typedef std::shared_ptr<contact_profile> profile_ptr;

	class ContactListModel : public CustomAbstractListModel
	{
		Q_OBJECT

	Q_SIGNALS:

		void currentDlgStateChanged() const;
		void selectedContactChanged(QString) const;
		void contactChanged(QString) const;
		void select(QString) const;
		void profile_loaded(profile_ptr _profile) const;
		void contact_added(QString _contact, bool _result);
		void contact_removed(QString _contact);
		void results();
        void needSwitchToRecents();

	private Q_SLOTS:

		void contactList(std::shared_ptr<Data::ContactList>, QString);
		void avatarLoaded(QString);
		void presense(Data::Buddy*);
		void groupClicked(int);
		void scrolled(int);
		void dlgState(Data::DlgState);
		void contactRemoved(QString);


	public Q_SLOTS:
		void searchPatternChanged(QString);
		void refresh();
		void searchResult(QStringList);

        void auth_add_contact(QString _aimid);
        void stats_auth_add_contact(QString _aimid);
        void unknown_contact_profile_spam_contact(QString _aimid);
        void auth_spam_contact(QString _aimid);
	    void auth_delete_contact(QString _aimid);
        void auth_ignore_contact(QString _aimid);
        void stats_spam_profile(QString _aimid);
		
	public:
		explicit ContactListModel(QObject *parent);
		ContactItem* getContactItem(const QString& _aimId);
        void setCurrentCallbackHappened(Ui::HistoryControlPage *page);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;

        int GetAbsIndexByVisibleIndex(int cur, int* visibleCount, int iter_limit) const;

        Qt::ItemFlags flags(const QModelIndex &index) const;
		std::vector<ContactItem> GetSearchedContacts(std::list<QString> contacts);
		std::vector<ContactItem> GetSearchedContacts();

		void setFocus();
		void setCurrent(QString aimdId, bool select = false, std::function<void(Ui::HistoryControlPage*)> getPageCallback = nullptr);

		const ContactItem* getContactItem(const QString& _aimId) const;

		QString selectedContact() const;
		QString selectedContactName() const;
		QString selectedContactState() const;

		QString getDisplayName(const QString& aimdId) const;
		QString getState(const QString& aimdId) const;
        QDateTime getLastSeen(const QString& aimdId) const;
		QString getInputText(const QString& aimdId) const;
		void setInputText(const QString& aimId, const QString& _text);
		bool isChat(const QString& aimId) const;
		bool isMuted(const QString& aimId) const;
        QModelIndex contactIndex(const QString& aimId);

		void get_contact_profile(const QString& _aimId, std::function<void(profile_ptr, int32_t)> _call_back = [](profile_ptr, int32_t){});
		void add_contact_to_contact_list(const QString& _aimid, std::function<void(bool)> _call_back = [](bool){});
		void remove_contact_from_contact_list(const QString& _aimid);
        bool block_spam_contact(const QString& _aimid, bool _with_confirmation = true);
		void ignore_contact(const QString& _aimid, bool ignore);
		bool ignore_and_remove_from_cl_contact(const QString& _aimid);
        void static get_ignore_list();
		
		void emitChanged(int first, int last);
		
		void ChangeChecked(const QString &index);
		std::vector<ContactItem> GetCheckedContacts() const;
		void ClearChecked();
		void SetChecked(QString& _aimid);
		void SetIsWithCheckedBox(bool);
		bool GetIsWithCheckedBox();
        int get_service_index() const;
        QString contactToTryOnTheme() const;

        void refreshList();

	private:
		std::shared_ptr<bool>	ref_;
        std::function<void(Ui::HistoryControlPage*)> gotPageCallback_;
		void rebuild_index();
		int addItem(Data::Contact *contact);
		void pushChange(int i);
		void processChanges();
		void sort();
		bool IsShowInSelectMembers(const ContactItem& item);

		std::vector<ContactItem>	contacts_;
		QHash<QString, int>			indexes_;

		int ScrollPosition_;
		mutable int MinVisibleIndex_;
		mutable int MaxVisibleIndex_;
		std::vector<int> UpdatedItems_;

		QString CurrentAimdId_;

		QTimer* Timer_;

		std::vector<ContactItem> Match_;
		QStringList SearchPatterns_;
		bool SearchRequested_;
		bool isSearch_;
		bool IsWithCheckedBox_;
	};

	ContactListModel* GetContactListModel();
    void ResetContactListModel();
}