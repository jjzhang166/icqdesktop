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

        void addContact();
        void deleteContact();
        void spamContact();

    private:

        std::shared_ptr<bool>	ref_;

    protected:

        const QString			aimid_;

        TextEmojiWidget*		name_;
        TextEmojiWidget*		info_;

        virtual void paintEvent(QPaintEvent* _e) override;

        void initFromProfile(Logic::profile_ptr _profile);

    public:

        ContactInfoWidget(QWidget* _parent, const QString& _aimid);
    };


    class AuthWidget : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:

        void avatarClicked();

    Q_SIGNALS:

        void addContact(QString _contact);
        void deleteContact(QString _contact);
        void spamContact(QString _contact);

    protected:

        const QString			aimid_;

        QVBoxLayout*			rootLayout_;
        ContactAvatarWidget*	avatar_;
        ContactInfoWidget*		infoWidget_;

        virtual void resizeEvent(QResizeEvent * _e) override;



    public:

        void placeAvatar();

        AuthWidget(QWidget* _parent, const QString& _aimid);
        virtual ~AuthWidget();
    };
}
