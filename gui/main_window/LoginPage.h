#pragma once

namespace core
{
    namespace stats
    {
        enum class stats_event_names;
    }
}

namespace Ui
{
	class LineEditEx;
	class CountrySearchCombobox;
    class CustomButton;
    class BackButton;

	class LoginPage : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void country(QString);
		void loggedIn();
        void attached();

	private Q_SLOTS:
		void countrySelected(QString);
		void nextPage();
		void updateTimer();
		void sendCode();
		void setPhoneFocusIn();
		void setPhoneFocusOut();
		void getSmsResult(int64_t, int);
		void loginResult(int64_t, int);
		void loginResultAttachUin(int64_t, int);
		void loginResultAttachPhone(int64_t, int);
		void clearErrors();
		void codeEditChanged(QString);
		void countryCodeChanged(QString);
		void redrawCountryCode();
		void emptyPhoneRemove();
        void stats_edit_phone();
        void stats_resend_sms();

    public Q_SLOTS:
		void prevPage();
		void switchLoginType();

	public:
		LoginPage(QWidget* parent, bool is_login);
		~LoginPage();
        
        void enableKeepLogedIn();

	protected:
		virtual void keyPressEvent(QKeyEvent* event);
		virtual void paintEvent(QPaintEvent* event);

	private:
		void init();
		void initLoginSubPage(int index);
		void setErrorText(int result);
        void updateErrors(int result);

	private:
		QTimer*						timer_;
		LineEditEx*					country_code_;
		LineEditEx*					phone_;
		CountrySearchCombobox*		combobox_;
		unsigned int				remaining_seconds_;
		QString						prev_country_code_;

		QStackedWidget*				login_staked_widget_;
		QPushButton*				next_page_link_;
		QWidget*					country_search_widget_;
        BackButton*                 prev_page_link_;
		QPushButton*				edit_phone_button_;
		QPushButton*				switch_login_link_;
		QPushButton*				resend_button_;
		QFrame*						phone_widget_;
		QLineEdit*					uin_login_edit_;
		QLineEdit*					uin_password_edit_;
        QCheckBox*                  keep_logged_;
		QLineEdit*					code_edit_;
		QLabel*						error_label_;
		QLabel*						entered_phone_;
		QLabel*						hint_label_;
        bool                        is_login_;
        int64_t                     send_seq_;
	};
}