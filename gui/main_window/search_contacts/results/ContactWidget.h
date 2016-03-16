#pragma once

namespace Logic
{
	class contact_profile;
}

namespace Ui
{
	class TextEditEx;
	class LineEditEx;
	class TextEmojiWidget;
		
	class ContactWidget : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
		
		void add_contact(QString _contact);
		void msg_contact(QString _contact);
		void call_contact(QString _contact);
        void contact_info(QString _contact);

	protected:

		std::shared_ptr<Logic::contact_profile>	profile_;

		TextEmojiWidget*	name_;
		TextEmojiWidget*	info_;
		QPushButton*		add_button_;
		QPushButton*		call_button_;
		QPushButton*		msg_button_;
		
	public:

		void on_add_result(bool _res);

		ContactWidget(QWidget* _parent, std::shared_ptr<Logic::contact_profile> _profile, const std::map<QString, QString>& _countries);
		virtual ~ContactWidget();
	};
}


