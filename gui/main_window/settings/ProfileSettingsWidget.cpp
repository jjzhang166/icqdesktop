#include "stdafx.h"
#include "ProfileSettingsWidget.h"

#include "../contact_list/ContactListModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/Common.h"
#include "../contact_list/SearchMembersModel.h"

#include "../../utils/utils.h"
#include "../../utils/application.h"
#include "../../utils/translator.h"
#include "../../utils/InterConnector.h"

#include "../../core_dispatcher.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/BackButton.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/Alert.h"

#include "../../gui_settings.h"

namespace Ui
{    
    class ProfileSettingsWidget::UI
    {
        QHBoxLayout *main_layout;
        
        QWidget *backbutton_area;
        QVBoxLayout *backbutton_area_layout;
            BackButton *back_button;
            QSpacerItem *verticalSpacer;
        
        QWidget *avatar_area;
        QVBoxLayout *avatar_area_layout;
            QWidget *avatar;
                QVBoxLayout *avatar_layout;
            QWidget *avatar_buttons_area;
            QHBoxLayout *avatar_buttons_area_layout;
                QPushButton *do_text_message;
                QPushButton *do_voice_message;
                QPushButton *do_video_message;
            QPushButton *ignore_button;
            QPushButton *report_spam_button;
            QSpacerItem *verticalSpacer_2;
        
        QScrollArea *info_area;
        QWidget *info_area_scroll_contents;
        QVBoxLayout *info_area_layout;
            QWidget *info_head_area;
            QHBoxLayout *info_head_area_layout;
                TextEmojiWidget *full_name;
                QPushButton *action_button;
                    FlatMenu *action_button_menu;
            QWidget *info_state_area;
            QHBoxLayout *info_state_area_layout;
                QPushButton *state_button;
                    FlatMenu *state_button_menu;
                QLabel *state_sign;
                QSpacerItem *horizontalSpacer;

            QWidget *info_values_area;
            QVBoxLayout *info_values_area_layout;

                TextEmojiWidget *icq_number_head;
                TextEmojiWidget *icq_number;
            
                TextEmojiWidget *phone_head;
                TextEmojiWidget *phone;
                TextEmojiWidget *phone_bottom;
            
                TextEmojiWidget *first_name_head;
                TextEmojiWidget *first_name;
            
                TextEmojiWidget *last_name_head;
                TextEmojiWidget *last_name;
            
                TextEmojiWidget *birthdate_head;
                TextEmojiWidget *birthdate;
            
                TextEmojiWidget *gender_head;
                TextEmojiWidget *gender;
            
                TextEmojiWidget *country_head;
                TextEmojiWidget *country;
            
                TextEmojiWidget *city_head;
                TextEmojiWidget *city;
            
                TextEmojiWidget *about_head;
                TextEmojiWidget *about;

                TextEmojiWidget *show_ignore_list_label;
        
        friend class ProfileSettingsWidget;
        
    public:
        void init(QWidget *profile_settings_widget)
        {
            profile_settings_widget->setObjectName(QStringLiteral("profile_settings_widget"));
            profile_settings_widget->setStyleSheet(Utils::LoadStyle(":/main_window/settings/profile_settings.qss", Utils::get_scale_coefficient(), true));
            profile_settings_widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            profile_settings_widget->setMinimumSize(QSize(0, 0));
            profile_settings_widget->setBaseSize(QSize(0, 0));
            
            main_layout = new QHBoxLayout(profile_settings_widget);
            main_layout->setSpacing(0);
            main_layout->setObjectName(QStringLiteral("main_layout"));
            main_layout->setContentsMargins(0, 0, 0, 0);
            
            backbutton_area = new QWidget(profile_settings_widget);
            backbutton_area->setObjectName(QStringLiteral("backbutton_area"));
            backbutton_area->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
            backbutton_area_layout = new QVBoxLayout(backbutton_area);
            backbutton_area_layout->setObjectName(QStringLiteral("backbutton_area_layout"));
            backbutton_area_layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

            back_button = new BackButton(backbutton_area);
            back_button->setObjectName(QStringLiteral("back_button"));
            back_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            back_button->setFlat(true);
			back_button->setFocusPolicy(Qt::NoFocus);
			back_button->setCursor(Qt::CursorShape::PointingHandCursor);
            backbutton_area_layout->addWidget(back_button);
            
            verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
            backbutton_area_layout->addItem(verticalSpacer);
            
            main_layout->addWidget(backbutton_area);
            
            avatar_area = new QWidget(profile_settings_widget);
            avatar_area->setObjectName(QStringLiteral("avatar_area"));
            avatar_area->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
            avatar_area_layout = new QVBoxLayout(avatar_area);
            avatar_area_layout->setSpacing(0);
            avatar_area_layout->setObjectName(QStringLiteral("verticalLayout"));
            avatar_area_layout->setContentsMargins(Utils::scale_value(48), Utils::scale_value(24), Utils::scale_value(32), 0);

            avatar = new QWidget(avatar_area);
            avatar->setObjectName(QStringLiteral("avatar"));
            avatar->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            avatar_layout = new QVBoxLayout(avatar);
            avatar_layout->setObjectName(QStringLiteral("avatar_layout"));
            avatar_layout->setContentsMargins(0, 0, 0, 0);
            avatar_area_layout->addWidget(avatar);
            
            avatar_buttons_area = new QWidget(avatar_area);
            avatar_buttons_area->setObjectName(QStringLiteral("avatar_buttons_area"));
            avatar_buttons_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            avatar_buttons_area_layout = new QHBoxLayout(avatar_buttons_area);
            avatar_buttons_area_layout->setObjectName(QStringLiteral("avatar_buttons_area_layout"));
            avatar_buttons_area_layout->setSpacing(0);
            avatar_buttons_area_layout->setContentsMargins(0, 0, 0, 0);
            
            do_text_message = new QPushButton(avatar_buttons_area);
            do_text_message->setObjectName(QStringLiteral("do_text_message"));
            do_text_message->setFlat(true);
            do_text_message->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			do_text_message->setFocusPolicy(Qt::NoFocus);
			do_text_message->setCursor(Qt::CursorShape::PointingHandCursor);
            avatar_buttons_area_layout->addWidget(do_text_message);
            
            do_voice_message = new QPushButton(avatar_buttons_area);
            do_voice_message->setObjectName(QStringLiteral("do_voice_message"));
            do_voice_message->setFlat(true);
            do_voice_message->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			do_voice_message->setFocusPolicy(Qt::NoFocus);
			do_voice_message->setCursor(Qt::CursorShape::PointingHandCursor);
            avatar_buttons_area_layout->addWidget(do_voice_message);
            
#ifdef STRIP_VOIP
            do_voice_message->hide();
            avatar_buttons_area_layout->setContentsMargins(Utils::scale_value(20), 0, 0, 0);
#endif //STRIP_VOIP

            do_video_message = new QPushButton(avatar_buttons_area);
            do_video_message->setObjectName(QStringLiteral("do_video_message"));
            do_video_message->setFlat(true);
            do_video_message->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			do_video_message->setFocusPolicy(Qt::NoFocus);
			do_video_message->setCursor(Qt::CursorShape::PointingHandCursor);
            avatar_buttons_area_layout->addWidget(do_video_message);

#ifdef STRIP_VOIP
            do_video_message->hide();
#endif //STRIP_VOIP
            
            avatar_area_layout->addWidget(avatar_buttons_area);
            avatar_area_layout->addWidget(show_ignore_list_label);
            
            ignore_button = new QPushButton(avatar_area);
            ignore_button->setObjectName(QStringLiteral("ignore_button"));
            ignore_button->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            ignore_button->setFlat(true);
			ignore_button->setFocusPolicy(Qt::NoFocus);
			ignore_button->setCursor(Qt::CursorShape::PointingHandCursor);
            avatar_area_layout->addWidget(ignore_button);
            
            report_spam_button = new QPushButton(avatar_area);
            report_spam_button->setObjectName(QStringLiteral("report_spam_button"));
            report_spam_button->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            report_spam_button->setFlat(true);
			report_spam_button->setFocusPolicy(Qt::NoFocus);
			report_spam_button->setCursor(Qt::CursorShape::PointingHandCursor);
            avatar_area_layout->addWidget(report_spam_button);
            
            verticalSpacer_2 = new QSpacerItem(Utils::scale_value(20), Utils::scale_value(40), QSizePolicy::Minimum, QSizePolicy::Expanding);
            
            avatar_area_layout->addItem(verticalSpacer_2);
            
            main_layout->addWidget(avatar_area);
            
            info_area = new QScrollArea(profile_settings_widget);
            info_area->setObjectName(QStringLiteral("info_area"));
            info_area->setWidgetResizable(true);
            Utils::grabTouchWidget(info_area->viewport(), true);
            
            info_area_scroll_contents = new QWidget(info_area);
            info_area_scroll_contents->setObjectName(QStringLiteral("info_area_scroll_contents"));
            info_area_scroll_contents->setGeometry(QRect(0, 0, Utils::scale_value(847), Utils::scale_value(707)));
            Utils::grabTouchWidget(info_area_scroll_contents);

            info_area_layout = new QVBoxLayout(info_area_scroll_contents);
            info_area_layout->setSpacing(0);
            info_area_layout->setObjectName(QStringLiteral("info_area_layout"));
            info_area_layout->setContentsMargins(0, Utils::scale_value(19), Utils::scale_value(48), Utils::scale_value(48));
            
            info_head_area = new QWidget(info_area);
            Utils::grabTouchWidget(info_head_area);
            info_head_area->setObjectName(QStringLiteral("info_head_area"));
            info_head_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            info_head_area->setMaximumHeight(Utils::scale_value(72));
            info_head_area_layout = new QHBoxLayout(info_head_area);
            info_head_area_layout->setSpacing(0);
            info_head_area_layout->setObjectName(QStringLiteral("info_head_area_layout"));
            info_head_area_layout->setContentsMargins(Utils::scale_value(16), 0, 0, Utils::scale_value(10));
            
            full_name = new TextEmojiWidget(info_head_area, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(full_name);
            full_name->set_ellipsis(true);
            full_name->set_selectable(true);
            info_head_area_layout->addWidget(full_name);
            
            action_button = new QPushButton(info_head_area);
            action_button->setObjectName(QStringLiteral("action_button"));
            action_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            action_button->setFlat(true);
            action_button->setIconSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
			action_button->setFocusPolicy(Qt::NoFocus);
			action_button->setCursor(Qt::CursorShape::PointingHandCursor);
            action_button_menu = new FlatMenu(action_button);
            action_button_menu->setExpandDirection(Qt::AlignLeft);
            action_button_menu->setObjectName(QStringLiteral("action_button_menu"));
            info_head_area_layout->addWidget(action_button);
            
            info_area_layout->addWidget(info_head_area);
            
            info_state_area = new QWidget(info_area);
            Utils::grabTouchWidget(info_state_area);
            info_state_area->setObjectName(QStringLiteral("info_state_area"));
            info_state_area->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            info_state_area->setMinimumSize(QSize(0, Utils::scale_value(45)));
            info_state_area->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            info_state_area_layout = new QHBoxLayout(info_state_area);
            info_state_area_layout->setSpacing(0);
            info_state_area_layout->setObjectName(QStringLiteral("info_state_area_ayout"));
            info_state_area_layout->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            
            state_button = new QPushButton(info_state_area);
            state_button->setObjectName(QStringLiteral("state_button"));
            state_button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            state_button->setMinimumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
            state_button->setMaximumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
            state_button->setFlat(true);
            state_button->setIconSize(QSize(Utils::scale_value(24), Utils::scale_value(24)));
			state_button->setFocusPolicy(Qt::NoFocus);
			state_button->setCursor(Qt::CursorShape::PointingHandCursor);
            state_button_menu = new FlatMenu(state_button);
            state_button_menu->setObjectName(QStringLiteral("state_button_menu"));
            info_state_area_layout->addWidget(state_button);
            
            state_sign = new QLabel(info_state_area);
            Utils::grabTouchWidget(state_sign);
            state_sign->setObjectName(QStringLiteral("state_sign"));
            info_state_area_layout->addWidget(state_sign);
            
            horizontalSpacer = new QSpacerItem(Utils::scale_value(40), Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);
            info_state_area_layout->addItem(horizontalSpacer);
            
            info_area_layout->addWidget(info_state_area);

            info_values_area = new QWidget(info_area);
            Utils::grabTouchWidget(info_values_area);
            info_values_area->setObjectName(QStringLiteral("info_values_area"));
            info_values_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            info_values_area_layout = new QVBoxLayout(info_values_area);
            info_values_area_layout->setAlignment(Qt::AlignTop);
            info_values_area_layout->setObjectName(QStringLiteral("info_values_area_layout"));
            info_values_area_layout->setSpacing(0);
            info_values_area_layout->setContentsMargins(Utils::scale_value(16), 0, 0, 0);

            auto fieldRoutine1 = [](QWidget* parent, QLayout* parentLayout, TextEmojiWidget*& fieldHead, TextEmojiWidget*& field, int dy)
            {
                fieldHead = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14), QColor("#7f282828"), Utils::scale_value(36 + dy));
                fieldHead->set_ellipsis(true);
                Utils::grabTouchWidget(fieldHead);
                parentLayout->addWidget(fieldHead);
                
                field = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#282828"), Utils::scale_value(20));
                field->set_ellipsis(true);
                field->set_selectable(true);
                Utils::grabTouchWidget(field);
                parentLayout->addWidget(field);
            };
            auto fieldRoutine2 = [fieldRoutine1](QWidget* parent, QLayout* parentLayout, TextEmojiWidget*& fieldHead, TextEmojiWidget*& field, TextEmojiWidget*& fieldBottom, int dy)
            {
                fieldRoutine1(parent, parentLayout, fieldHead, field, dy);
                fieldBottom = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14), QColor("#7f282828"), Utils::scale_value(18));
                fieldBottom->set_ellipsis(true);
                Utils::grabTouchWidget(fieldBottom);
                parentLayout->addWidget(fieldBottom);
            };
            fieldRoutine1(info_values_area, info_values_area_layout, icq_number_head, icq_number, -10);
            fieldRoutine2(info_values_area, info_values_area_layout, phone_head, phone, phone_bottom, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, first_name_head, first_name, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, last_name_head, last_name, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, birthdate_head, birthdate, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, gender_head, gender, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, country_head, country, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, city_head, city, 0);
            fieldRoutine1(info_values_area, info_values_area_layout, about_head, about, 0);

            about->set_ellipsis(false);
            about->set_multiline(true);
            
            show_ignore_list_label = new TextEmojiWidget(info_values_area, Utils::FontsFamily::SEGOE_UI,
                Utils::scale_value(18), QColor("#e30f04"), Utils::scale_value(44 - 18 + 10));
            Utils::grabTouchWidget(show_ignore_list_label);
            info_values_area_layout->addWidget(show_ignore_list_label);            
            show_ignore_list_label->setCursor(Qt::PointingHandCursor);
            show_ignore_list_label->setText(QT_TR_NOOP("Ignore list"));
            show_ignore_list_label->setObjectName(QStringLiteral("show_ignore_list_label"));
            show_ignore_list_label->set_ellipsis(true);
            
            info_area_layout->addWidget(info_values_area);

            main_layout->addWidget(info_area);
            
            info_area->setWidget(info_area_scroll_contents);
            
            retranslateUi(profile_settings_widget);
            
            QMetaObject::connectSlotsByName(profile_settings_widget);
        }
        
        void retranslateUi(QWidget *profile_settings_widget)
        {
            profile_settings_widget->setWindowTitle(QT_TR_NOOP("Profile"));
            back_button->setText("");
            do_text_message->setText("");
            do_voice_message->setText("");
            do_video_message->setText("");
            ignore_button->setText(QT_TR_NOOP("Ignore"));
            report_spam_button->setText(QT_TR_NOOP("Report spam"));
            full_name->setText("");
            action_button->setText(QT_TR_NOOP("Edit profile"));
            state_button->setText("");
            state_sign->setText("");
            icq_number_head->setText(QT_TR_NOOP("UIN"));
            icq_number->setText("");
            phone_head->setText(QT_TR_NOOP("Phone number"));
            phone->setText("");
            phone_bottom->setText(QT_TR_NOOP("Only visible for those, who has it in the phone book"));
            first_name_head->setText(QT_TR_NOOP("First name"));
            first_name->setText("");
            last_name_head->setText(QT_TR_NOOP("Last name"));
            last_name->setText("");
            birthdate_head->setText(QT_TR_NOOP("Birthdate"));
            birthdate->setText("");
            gender_head->setText(QT_TR_NOOP("Gender"));
            gender->setText("");
            country_head->setText(QT_TR_NOOP("Country"));
            country->setText("");
            city_head->setText(QT_TR_NOOP("City"));
            city->setText("");
            about_head->setText(QT_TR_NOOP("About"));
            about->setText("");
        }
    };
    
    ProfileSettingsWidget::ProfileSettingsWidget(QWidget* _parent):
        QWidget(_parent),
        Ui_(new UI()),
        avatar_(nullptr),
        actionButtonState_(EDIT_PROFILE),
        uin_(""),
        needRequestAgain_(false),
        disconnector_(new Utils::SignalsDisconnector)
    {
        Ui_->init(this);
        
        connect(Ui_->back_button, &QPushButton::clicked, [this]() { emit Utils::InterConnector::instance().profileSettingsBack(); });

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(onAvatarLoaded(QString)), Qt::QueuedConnection);
        
        Ui_->state_button_menu->addAction(QIcon(":/resources/content_status_online_200.png"), QT_TR_NOOP("Online"), this, SLOT(menuStateOnline()));
        Ui_->state_button_menu->addAction(QIcon(":/resources/content_status_dnd_200.png"), QT_TR_NOOP("Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
        Ui_->state_button_menu->addAction(QIcon(":/resources/content_status_invisible_200.png"), QT_TR_NOOP("Invisible"), this, SLOT(menuStateInvisible()));

        /*
        Ui_->action_button_menu->addAction(QT_TR_NOOP("Add to Favorites"));
        Ui_->action_button_menu->addSeparator();
        */
        Ui_->action_button_menu->addAction(QT_TR_NOOP("Ignore"), this, SLOT(contactIgnore()));
        Ui_->action_button_menu->addAction(QT_TR_NOOP("Report Spam"), this, SLOT(contactSpam()));

        connect(Ui_->ignore_button, SIGNAL(clicked()), this, SLOT(contactIgnore()));
        connect(Ui_->report_spam_button, SIGNAL(clicked()), this, SLOT(contactSpam()));

        Ui_->show_ignore_list_label->disconnect();
        Testing::setAccessibleName(Ui_->show_ignore_list_label, "IgnoreList");

        connect(Ui_->show_ignore_list_label, &TextEmojiWidget::clicked, [this]()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_open);
            QVector<QString> temp;
            Logic::UpdateIgnoredModel(temp);
            Logic::GetSearchMemberModel()->SetChatMembersModel(Logic::GetIgnoreModel());

            SelectContactsWidget select_members_dialog_(Logic::GetIgnoreModel(), Logic::MembersWidgetRegim::IGNORE_LIST,
                "", QT_TRANSLATE_NOOP("groupchat_pages", "Done"), Ui::get_gui_settings(), this);
            auto connectId = connect(GetDispatcher(), SIGNAL(recv_permit_deny(bool)), &select_members_dialog_, SLOT(UpdateView(bool)), Qt::QueuedConnection);

            Logic::ContactListModel::get_ignore_list();
            select_members_dialog_.setView(false);
            select_members_dialog_.show(-1, -1);
            
            disconnect(connectId);
        });

        disconnector_->add("login_complete", QWidget::connect(GetDispatcher(), &core_dispatcher::login_complete, [this]
        {
            if (needRequestAgain_ && isVisible())
                updateInterface(uin_);
        }));
        
        updateActionButton();
    }
    
    ProfileSettingsWidget::~ProfileSettingsWidget()
    {
        //
    }
    
    void ProfileSettingsWidget::contactAdd()
    {
        //
    }

    void ProfileSettingsWidget::contactIgnore()
    {
        emit Utils::InterConnector::instance().profileSettingsUnknownIgnore(uin_);
        emit Utils::InterConnector::instance().profileSettingsBack();
    }

    void ProfileSettingsWidget::contactSpam()
    {
        emit Utils::InterConnector::instance().profileSettingsUnknownSpam(uin_);
        emit Utils::InterConnector::instance().profileSettingsBack();
    }
    
    void ProfileSettingsWidget::myInfo()
    {
        auto state = MyInfo()->state().toLower();
        if (state == "invisible")
            setStateInvisible();
        else if (state == "dnd")
            setStateDoNotDisturb();
        else if (state == "offline")
            setStateOffline();
        else
            setStateOnline();
    }
    
    void ProfileSettingsWidget::updateInterface(const QString &uin)
    {
        needRequestAgain_ = false;
        
        if (uin.length())
        {
            disconnect(SIGNAL(myInfo()));
            disconnect(SIGNAL(needLogin()));
        }
        else
        {
            myInfo();
            connect(GetDispatcher(), SIGNAL(myInfo()), this, SLOT(myInfo()));
            connect(GetDispatcher(), SIGNAL(needLogin()), this, SLOT(setStateOffline()));
        }
        
        uin_ = uin;
        
        Ui_->backbutton_area->setVisible(uin.length());
        Ui_->avatar_buttons_area->setVisible(uin.length());
        Ui_->ignore_button->setVisible(false);
        Ui_->report_spam_button->setVisible(false);
        Ui_->info_state_area->setVisible(false);
        Ui_->show_ignore_list_label->setVisible(uin.isEmpty());
        setFullName("");
        setICQNumber("");
        setPhone("", false);
        setFirstName("");
        setLastName("");
        setBirthdate("");
        setGender("");
        setCountry("");
        setCity("");
        setAbout("");
        if (avatar_)
        {
            delete avatar_;
            avatar_ = nullptr;
        }

        actionButtonState_ = uin.length() ? USER_ACTIONS : EDIT_PROFILE;
        updateActionButton();

        Logic::GetContactListModel()->get_contact_profile(uin, [this](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            if (_profile)
                parse(_profile);
            else
                needRequestAgain_ = true;
        });
    }

    void ProfileSettingsWidget::paintEvent(QPaintEvent* event)
    {
        QWidget::paintEvent(event);
        
        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(geometry().x() - 1, geometry().y() - 1, visibleRegion().boundingRect().width() + 2, visibleRegion().boundingRect().height() + 2);
    }

    bool ProfileSettingsWidget::event(QEvent* event)
    {
        if (event->type() == QMouseEvent::Wheel)
        {
            auto barValue = Ui_->info_area->verticalScrollBar()->value();
            if (!Ui_->info_area->geometry().contains(mapFromGlobal(QCursor::pos())))
            {
                QWheelEvent* wheel = static_cast< QWheelEvent* >(event);
                auto nval = (barValue - wheel->delta());
                nval = std::max(nval, Ui_->info_area->verticalScrollBar()->minimum());
                nval = std::min(nval, Ui_->info_area->verticalScrollBar()->maximum());
                Ui_->info_area->verticalScrollBar()->setValue(nval);
            }
        }
        return QWidget::event(event);
    }

    void ProfileSettingsWidget::parse(Logic::profile_ptr profile)
    {
        auto contact = Logic::GetContactListModel()->getContactItem(profile->get_aimid());

        auto contactIsSelf = (actionButtonState_ == EDIT_PROFILE);

        // Set common values
        {
            if (contact && contact->Get()->GetDisplayName().length())
                setFullName(contact->Get()->GetDisplayName());
            else if (profile->get_contact_name().length())
                setFullName(profile->get_contact_name());
            else if (profile->get_displayid().length())
                setFullName(profile->get_displayid());
            else if (profile->get_friendly().length())
                setFullName(profile->get_friendly());
            else
                setFullName(QString("%1%2%3").arg(profile->get_first_name()).arg(profile->get_first_name().length() ? " " : "").arg(profile->get_last_name()));
            
            setICQNumber(profile->get_aimid());
            setPhone(!profile->get_phones().empty() ? profile->get_phones().front().get_phone() : "", contactIsSelf);
            setFirstName(profile->get_first_name());
            setLastName(profile->get_last_name());
            setBirthdate(profile->get_birthdate() ? Utils::GetTranslator()->formatDate(QDateTime::fromMSecsSinceEpoch(profile->get_birthdate() * 1000, Qt::LocalTime).date(), false) : "");
            setGender(profile->get_gender());
            setCountry(profile->get_origin_address().get_country());
            setCity(profile->get_origin_address().get_city());
            setAbout(profile->get_about());
        }

        auto contactIsFriend = ((actionButtonState_ == USER_ACTIONS) && (contact && !contact->is_not_auth()));
        auto contactIsUnknown = ((actionButtonState_ == USER_ACTIONS) && !contactIsSelf && !contactIsFriend);
        
        auto contactIsOnline = (contact && contact->is_online());
        
        auto uin = profile->get_aimid();
        auto dname = Ui_->full_name->text();
        
        Ui_->info_state_area->setVisible(contactIsSelf || contactIsFriend);
        Ui_->state_button->setCursor(contactIsSelf ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);

        if (actionButtonState_ == EDIT_PROFILE)
        {
            Utils::ApplyPropertyParameter(Ui_->state_sign, "green", true);
            GetDispatcher()->disconnect(SIGNAL(signedUrl(QString)));
            connect(GetDispatcher(), &core_dispatcher::signedUrl, [](QString url)
            {
                //puts(url.toStdString().c_str());
                
                ((Utils::Application *)qApp)->unsetUrlHandler();
                QDesktopServices::openUrl(url);
                ((Utils::Application *)qApp)->setUrlHandler();
            });
            Ui_->action_button->disconnect();
            connect(Ui_->action_button, &QPushButton::clicked, [uin]()
            {
                if (uin.length())
                {
                    auto url = QString("https://icq.com/people/%1/edit").arg(uin);
                    core::coll_helper helper(GetDispatcher()->create_collection(), true);
                    helper.set_value_as_string("url", url.toStdString());
                    GetDispatcher()->post_message_to_core("sign_url", helper.get());
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_edit);
                }
            });
           
            Ui_->do_text_message->disconnect();
            Ui_->do_voice_message->disconnect();
            Ui_->do_video_message->disconnect();
        }
        else if (actionButtonState_ == USER_ACTIONS)
        {
            GetDispatcher()->disconnect(SIGNAL(signedUrl(QString)));
            
            Ui_->state_sign->setText("");
            if (contactIsFriend && contact && contact->Get())
            {
                Ui_->state_button->setVisible(contactIsOnline);
                if (!contact->is_online())
                {
                    QString state = QT_TR_NOOP("Seen ");
                    QDateTime lastSeen = contact->Get()->GetLastSeen();
                    if (lastSeen.isValid())
                    {
                        const auto current = QDateTime::currentDateTime();
                        const auto days = lastSeen.daysTo(current);
                        if (days == 0)
                            state += QT_TR_NOOP("today");
                        else if (days == 1)
                            state += QT_TR_NOOP("yesterday");
                        else
                            state += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
                        if (lastSeen.date().year() == current.date().year())
                        {
                            state += QT_TR_NOOP(" at ");
                            state += lastSeen.time().toString(Qt::SystemLocaleShortDate);
                        }

                        Utils::ApplyPropertyParameter(Ui_->state_sign, "green", true);
                        Ui_->state_sign->setText(state);
                    }
                }
            }
            if (Ui_->state_sign->text().isEmpty())
            {
                if (contactIsOnline)
                {
                    Utils::ApplyPropertyParameter(Ui_->state_sign, "green", true);
                    auto s = contact->Get()->State_.toLower();
                    if (s == "dnd")
                        setStateDoNotDisturb();
                    else if (s == "invisible")
                        setStateInvisible();
                    else
                        setStateOnline();
                }
                else
                {
                    Utils::ApplyPropertyParameter(Ui_->state_sign, "green", false);
                    Ui_->state_sign->setText(QT_TR_NOOP("Offline"));
                }
            }
            
            Ui_->state_button->setVisible(false);
            
            Ui_->do_text_message->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::PointingHandCursor);
            Utils::ApplyPropertyParameter(Ui_->do_text_message, "known", contactIsFriend);

            Ui_->do_voice_message->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);
            Utils::ApplyPropertyParameter(Ui_->do_voice_message, "known", contactIsFriend);

            Ui_->do_video_message->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);
            Utils::ApplyPropertyParameter(Ui_->do_video_message, "known", contactIsFriend);
            
            Ui_->ignore_button->setVisible(contactIsUnknown);
            Ui_->report_spam_button->setVisible(contactIsUnknown);

            if (contactIsUnknown)
            {
                auto uin = uin_;
                Ui_->do_text_message->disconnect();
                connect(Ui_->do_text_message, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->add_contact_to_contact_list(uin, [uin](bool result)
                    {
                        if (result)
                        {
                            Logic::GetContactListModel()->setCurrent(uin, true);
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_profile_page);
                            emit Utils::InterConnector::instance().profileSettingsBack();
                        }
                    });
                });
                Ui_->do_voice_message->disconnect();
                Ui_->do_video_message->disconnect();
            }
            else
            {
                Ui_->do_text_message->disconnect();
                connect(Ui_->do_text_message, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();
                });
                Ui_->do_voice_message->disconnect();
                connect(Ui_->do_voice_message, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();
                    
                    QTimer::singleShot(500, [uin]() { Ui::GetDispatcher()->getVoipController().setStartA(uin.toUtf8(), false); });
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_call);
                });
                Ui_->do_video_message->disconnect();
                connect(Ui_->do_video_message, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();

                    QTimer::singleShot(500, [uin]() { Ui::GetDispatcher()->getVoipController().setStartV(uin.toUtf8(), false); });
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_video_call);
                });
            }
        }
        updateActionButton();

        if (avatar_)
        {
            // just in case. it must be deleted when a profile requested.
            delete avatar_;
            avatar_ = nullptr;
        }
        if (!avatar_)
        {
            avatar_ = new ContactAvatarWidget(Ui_->avatar, uin, dname, Utils::scale_value(180));
            Ui_->avatar_layout->addWidget(avatar_);
        }
    }
    
    void ProfileSettingsWidget::onAvatarLoaded(QString uin)
    {
        if (avatar_)
            avatar_->update();
    }

    void ProfileSettingsWidget::menuStateOnline()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "online");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
		GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_online);
    }
    void ProfileSettingsWidget::menuStateDoNotDisturb()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "dnd");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_dnd);
    }
    void ProfileSettingsWidget::menuStateInvisible()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "invisible");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_invisible);
    }

    void ProfileSettingsWidget::setStateOnline()
    {
        Ui_->state_button->setVisible(true);
        Ui_->state_button->setIcon(QIcon(":/resources/content_status_online_200.png"));
        Ui_->state_sign->setText(QT_TR_NOOP("Online"));
    }
    
    void ProfileSettingsWidget::setStateOffline()
    {
        Ui_->state_button->setVisible(true);
        Ui_->state_button->setIcon(QIcon(":/resources/content_status_offline_200.png"));
        Ui_->state_sign->setText(QT_TR_NOOP("Offline"));
    }
    
    void ProfileSettingsWidget::setStateDoNotDisturb()
    {
        Ui_->state_button->setVisible(true);
        Ui_->state_button->setIcon(QIcon(":/resources/content_status_dnd_200.png"));
        Ui_->state_sign->setText(QT_TR_NOOP("Do not disturb"));
    }
    
    void ProfileSettingsWidget::setStateInvisible()
    {
        Ui_->state_button->setVisible(true);
        Ui_->state_button->setIcon(QIcon(":/resources/content_status_invisible_200.png"));
        Ui_->state_sign->setText(QT_TR_NOOP("Invisible"));
    }
    
    void ProfileSettingsWidget::setFullName(const QString& val)
    {
        Ui_->full_name->setText(val);
    }

    void ProfileSettingsWidget::setICQNumber(const QString& val)
    {
        Ui_->icq_number->setText(val);
        Ui_->icq_number->setVisible(val.length());
        Ui_->icq_number_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setPhone(const QString& val, bool forSelf)
    {
        Ui_->phone->setText(val);
        Ui_->phone->setVisible(val.length());
        Ui_->phone_head->setVisible(val.length());
        Ui_->phone_bottom->setVisible(forSelf && val.length());
    }

    void ProfileSettingsWidget::setFirstName(const QString& val)
    {
        Ui_->first_name->setText(val);
        Ui_->first_name->setVisible(val.length());
        Ui_->first_name_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setLastName(const QString& val)
    {
        Ui_->last_name->setText(val);
        Ui_->last_name->setVisible(val.length());
        Ui_->last_name_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setBirthdate(const QString& val)
    {
        Ui_->birthdate->setText(val);
        Ui_->birthdate->setVisible(val.length());
        Ui_->birthdate_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setGender(const QString& val)
    {
        if (val.toLower() == "male")
            Ui_->gender->setText(QT_TR_NOOP("Male"));
        else if (val.toLower() == "female")
            Ui_->gender->setText(QT_TR_NOOP("Female"));
        else
            Ui_->gender->setText("");
        Ui_->gender->setVisible(Ui_->gender->text().length());
        Ui_->gender_head->setVisible(Ui_->gender->text().length());
    }

    void ProfileSettingsWidget::setCountry(const QString& val)
    {
        Ui_->country->setText(val);
        Ui_->country->setVisible(val.length());
        Ui_->country_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setCity(const QString& val)
    {
        Ui_->city->setText(val);
        Ui_->city->setVisible(val.length());
        Ui_->city_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::setAbout(const QString& val)
    {
        Ui_->about->setText(val);
        Ui_->about->setVisible(val.length());
        Ui_->about_head->setVisible(val.length());
    }

    void ProfileSettingsWidget::updateActionButton()
    {
        if (actionButtonState_ == EDIT_PROFILE)
        {
            Ui_->action_button->setText(QT_TR_NOOP("Edit profile"));
            Ui_->action_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            Ui_->action_button->setMenu(nullptr);

            Ui_->avatar_area->setMinimumWidth(Utils::scale_value(260));
            Ui_->avatar_area_layout->setContentsMargins(Utils::scale_value(48), Utils::scale_value(24), Utils::scale_value(32), 0);

            // temporary disabled
            Ui_->state_button->setMenu(Ui_->state_button_menu);
            Ui_->info_state_area_layout->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            // remove lines below after set_state is implemented.
            /*
            Ui_->state_button->setMenu(nullptr);
            Ui_->state_button->setVisible(false);
            Ui_->info_state_area_layout->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
            */
        }
        else if (actionButtonState_ == USER_ACTIONS)
        {
            Ui_->action_button->setText("");
            Ui_->action_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            Ui_->action_button->setMenu(Ui_->action_button_menu);

            Ui_->avatar_area->setMinimumWidth(Utils::scale_value(236));
            Ui_->avatar_area_layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), Utils::scale_value(32), 0);

            Ui_->state_button->setMenu(nullptr);
            if (Ui_->state_button->isVisible())
                Ui_->info_state_area_layout->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            else
                Ui_->info_state_area_layout->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
        }
        Utils::ApplyPropertyParameter(Ui_->action_button, "self", actionButtonState_ == EDIT_PROFILE);
        Utils::ApplyPropertyParameter(Ui_->state_button, "self", actionButtonState_ == EDIT_PROFILE);
        Utils::ApplyPropertyParameter(Ui_->state_sign, "self", actionButtonState_ == EDIT_PROFILE);
    }
}
