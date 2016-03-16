#pragma once

namespace Logic
{
    class contact_profile;
}

namespace Ui
{
    class FlowLayout;
    class ContactWidget;

    typedef std::list<std::shared_ptr<Logic::contact_profile>>	profiles_list;

    class FoundContacts : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:

        void need_more(int _skip_count);
        void add_contact(QString _contact);
        void msg_contact(QString _contact);
        void call_contact(QString _contact);
        void contact_info(QString _contact);

    private Q_SLOTS:

        void on_avatar_loaded(QString _aimid);

        void on_add_contact(QString _contact);
        void on_msg_contact(QString _contact);
        void on_call_contact(QString _contact);
        void on_contact_info(QString _contact);

    private:

        QScrollArea*							area_;
        FlowLayout*								contacts_layout_;
        QVBoxLayout*							root_layout_;

        int										prev_scroll_value_;

        std::map<QString, ContactWidget*>		items_;
        std::map<QString, QString>				countries_;

        void hook_scroll();

    public:

        void insert_items(const profiles_list& _profiles);
        void contact_add_result(const QString& _contact, bool _res);
        void clear();
        bool empty();

        FoundContacts(QWidget* _parent);
        virtual ~FoundContacts(void);
    };
}
