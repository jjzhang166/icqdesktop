#pragma once


namespace Logic
{
    class contact_profile;
    typedef std::shared_ptr<contact_profile> profile_ptr;
}


namespace Ui
{
    class ContactAvatarWidget;
    class TextEmojiWidget;

    class ContactInfoWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void add_contact();
        void delete_contact();
        void spam_contact();

    private:

        std::shared_ptr<bool>	ref_;

    protected:

        const QString			aimid_;

        TextEmojiWidget*		name_;
        TextEmojiWidget*		info_;

        virtual void paintEvent(QPaintEvent* _e) override;

        void init_from_profile(Logic::profile_ptr _profile);

    public:

        ContactInfoWidget(QWidget* _parent, const QString& _aimid);
    };


    class AuthWidget : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:

        void on_avatar_loaded(QString _aimid);
        void avatar_clicked();

    Q_SIGNALS:

        void add_contact(QString _contact);
        void delete_contact(QString _contact);
        void spam_contact(QString _contact);

    protected:

        const QString			aimid_;

        QVBoxLayout*			root_layout_;
        ContactAvatarWidget*	avatar_;
        ContactInfoWidget*		info_widget_;

        virtual void resizeEvent(QResizeEvent * _e) override;



    public:

        void place_avatar();

        AuthWidget(QWidget* _parent, const QString& _aimid);
        virtual ~AuthWidget();
    };
}
