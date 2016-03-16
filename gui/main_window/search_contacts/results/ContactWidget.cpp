#include "stdafx.h"
#include "ContactWidget.h"
#include "../../../utils/utils.h"
#include "../../contact_list/contact_profile.h"
#include "../../../controls/TextEditEx.h"
#include "../../../controls/LineEditEx.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../controls/ContactAvatarWidget.h"


const int32_t widget_height	= 100;
const int32_t widget_width	= 260;


namespace Ui
{
	QString get_contact_info(std::shared_ptr<Logic::contact_profile> _profile, const std::map<QString, QString>& _countries)
	{
		QString info;
		QTextStream result(&info);

		QString gender;
		if (_profile->get_gender() == "male")
			gender = QT_TRANSLATE_NOOP("search_widget", "M");
		else if (_profile->get_gender() == "female")
			gender = QT_TRANSLATE_NOOP("search_widget", "F");

		if (!gender.isEmpty())
			result << gender;

        if (_profile->get_birthdate() != 0)
        {
            QDateTime birthdate = QDateTime::fromMSecsSinceEpoch((qint64) _profile->get_birthdate() * 1000, Qt::LocalTime);
            
            int age = Utils::calc_age(birthdate);

            if (age > 0)
            {
                if (!info.isEmpty())
                    result << ", ";

                result << age;

                result << " " << Utils::GetTranslator()->getNumberString(
                    age, 
                    QT_TRANSLATE_NOOP3("search_widget", "year", "1"), 
                    QT_TRANSLATE_NOOP3("search_widget", "years", "2"),
                    QT_TRANSLATE_NOOP3("search_widget", "years", "5"),
                    QT_TRANSLATE_NOOP3("search_widget", "years", "21"));
            }
        }
		
		auto get_address = [_profile, &_countries]()->QString
		{
			if (!_profile->get_home_address().get_country().isEmpty())
			{
				auto iter_country = _countries.find(_profile->get_home_address().get_country().toLower());
				if (_countries.end() != iter_country)
					return iter_country->second;

				return _profile->get_home_address().get_country();
			}

			if (!_profile->get_home_address().get_state().isEmpty())
				return _profile->get_home_address().get_state();

			if (!_profile->get_home_address().get_city().isEmpty())
				return _profile->get_home_address().get_city();

			return "";
		};

		

		QString loc = get_address();
		if (!loc.isEmpty())
		{
			if (!info.isEmpty())
				result << ", ";

			result << loc;
		}


		return info;
	}

	ContactWidget::ContactWidget(QWidget* _parent, std::shared_ptr<Logic::contact_profile> _profile, const std::map<QString, QString>& _countries)
		:	QWidget(_parent), 
			profile_(_profile), 
			name_(new TextEmojiWidget(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor(0x28, 0x28, 0x28))),
			info_(new TextEmojiWidget(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14), QColor(0x69, 0x69, 0x69))),
			add_button_(new QPushButton(this)),
			msg_button_(new QPushButton(this)),
			call_button_(new QPushButton(this))
	{
		Utils::grabTouchWidget(this);

		setFixedHeight(Utils::scale_value(widget_height));
		setFixedWidth(Utils::scale_value(widget_width));

		QHBoxLayout* root_layout = new QHBoxLayout();
		root_layout->setContentsMargins(0, 0, 0, 0);
		root_layout->setSpacing(0);
		root_layout->setAlignment(Qt::AlignLeft);

		const QString display_name = _profile->get_contact_name();

		ContactAvatarWidget* avatar_widget = new ContactAvatarWidget(
			this, 
			_profile->get_aimid(),
			display_name,
			Utils::scale_value(widget_height));

        avatar_widget->setCursor(QCursor(Qt::PointingHandCursor));

		root_layout->addWidget(avatar_widget);

		Utils::grabTouchWidget(avatar_widget);

		QVBoxLayout* info_layout = new QVBoxLayout();
		info_layout->setContentsMargins(Utils::scale_value(12), 0, 0, 0);
		info_layout->setSpacing(0);
		info_layout->setAlignment(Qt::AlignTop);
		
		name_->setObjectName("contact_name");
        name_->set_ellipsis(true);
		name_->setText(_profile->get_contact_name());
		info_layout->addWidget(name_);
		Utils::grabTouchWidget(name_);

		info_->setObjectName("contact_info");
		info_->setText(get_contact_info(_profile, _countries));
		info_layout->addWidget(info_);
		Utils::grabTouchWidget(info_);

		QHBoxLayout* buttons_layout = new QHBoxLayout();
		buttons_layout->setSpacing(Utils::scale_value(12));
		buttons_layout->setAlignment(Qt::AlignLeft);
		
		Logic::ContactItem* contact_item = Logic::GetContactListModel()->getContactItem(profile_->get_aimid());

		add_button_->setObjectName("add_contact_button");
		add_button_->setVisible(!contact_item);
		add_button_->setCursor(QCursor(Qt::PointingHandCursor));
		buttons_layout->addWidget(add_button_);
		Utils::grabTouchWidget(add_button_);

		msg_button_->setVisible(!!contact_item);
		msg_button_->setObjectName("msg_contact_button");
		msg_button_->setCursor(QCursor(Qt::PointingHandCursor));
		buttons_layout->addWidget(msg_button_);
		Utils::grabTouchWidget(msg_button_);

		call_button_->setVisible(!!contact_item);
		call_button_->setObjectName("call_contact_button");
		call_button_->setCursor(QCursor(Qt::PointingHandCursor));
		buttons_layout->addWidget(call_button_);
		Utils::grabTouchWidget(call_button_);
				
		info_layout->addLayout(buttons_layout);

		root_layout->addLayout(info_layout);

		setLayout(root_layout);

		connect(add_button_, &QPushButton::clicked, [this]()
		{
			emit add_contact(profile_->get_aimid());
		});

		connect(msg_button_, &QPushButton::clicked, [this]()
		{
			emit msg_contact(profile_->get_aimid());
		});

		connect(call_button_, &QPushButton::clicked, [this]()
		{
			emit call_contact(profile_->get_aimid());
		});

        connect(avatar_widget, &QPushButton::clicked, [this]()
        {
            emit contact_info(profile_->get_aimid());
        });
	}


	ContactWidget::~ContactWidget()
	{		
	}

	void ContactWidget::on_add_result(bool _res)
	{
		if (_res)
		{
			add_button_->setVisible(false);
			msg_button_->setVisible(true);
			call_button_->setVisible(true);
		}
	}
}

