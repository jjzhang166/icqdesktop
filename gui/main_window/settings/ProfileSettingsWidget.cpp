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
        QHBoxLayout *mainLayout_;
        
        QWidget *backButtonWidget_;
        QVBoxLayout *backButtonLayout_;
            BackButton *backButton_;
            QSpacerItem *verticalSpacer;
        
        QWidget *avatarWidget_;
        QVBoxLayout *avatarWidgetLayout_;
            QWidget *avatar_;
                QVBoxLayout *avatarLayout_;
            QWidget *actionsWidget_;
            QHBoxLayout *actionsLayout_;
                QPushButton *messageButton_;
                QPushButton *voiceButton_;
                QPushButton *videoButton_;
            QPushButton *ignoreButton_;
            QPushButton *spamButton_;
            QSpacerItem *verticalSpacer_2;
        
        QScrollArea *infoScrollArea_;
        QWidget *infoWidget_;
        QVBoxLayout *infoLayout_;
            QWidget *headWidget_;
            QHBoxLayout *headLayout_;
                TextEmojiWidget *fullName_;
                QPushButton *optionsButton_;
                    FlatMenu *optionsMenu_;
            QWidget *statusWidget_;
            QHBoxLayout *statusLayout_;
                QPushButton *statusButton_;
                    FlatMenu *statusMenu_;
                QLabel *statusLabel_;
                QSpacerItem *horizontalSpacer;

            QWidget *infoValuesWidget_;
            QVBoxLayout *infoValuesLayout_;

                TextEmojiWidget *uinHead_;
                TextEmojiWidget *uin_;
            
                TextEmojiWidget *phoneHead_;
                TextEmojiWidget *phone_;
                TextEmojiWidget *phoneBottom_;
            
                TextEmojiWidget *firstNameHead_;
                TextEmojiWidget *firstName_;
            
                TextEmojiWidget *lastNameHead_;
                TextEmojiWidget *lastName_;
            
                TextEmojiWidget *birthdateHead_;
                TextEmojiWidget *birthdate_;
            
                TextEmojiWidget *genderHead_;
                TextEmojiWidget *gender_;
            
                TextEmojiWidget *countryHead_;
                TextEmojiWidget *country_;
            
                TextEmojiWidget *cityHead_;
                TextEmojiWidget *city_;
            
                TextEmojiWidget *aboutHead_;
                TextEmojiWidget *about_;

                TextEmojiWidget *ignoreListLabel_;
                TextEmojiWidget *attach_phone_label;
                TextEmojiWidget *attach_uin_label;
        
        friend class ProfileSettingsWidget;
        
    public:
        void init(QWidget *_profileWidget)
        {
            _profileWidget->setObjectName(QStringLiteral("_profileWidget"));
            _profileWidget->setStyleSheet(Utils::LoadStyle(":/main_window/settings/profile_settings.qss", Utils::get_scale_coefficient(), true));
            _profileWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            _profileWidget->setMinimumSize(QSize(0, 0));
            _profileWidget->setBaseSize(QSize(0, 0));
            
            mainLayout_ = new QHBoxLayout(_profileWidget);
            mainLayout_->setSpacing(0);
            mainLayout_->setObjectName(QStringLiteral("mainLayout_"));
            mainLayout_->setContentsMargins(0, 0, 0, 0);
            
            backButtonWidget_ = new QWidget(_profileWidget);
            backButtonWidget_->setObjectName(QStringLiteral("backButtonWidget_"));
            backButtonWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
            backButtonLayout_ = new QVBoxLayout(backButtonWidget_);
            backButtonLayout_->setObjectName(QStringLiteral("backButtonLayout_"));
            backButtonLayout_->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

            backButton_ = new BackButton(backButtonWidget_);
            backButton_->setObjectName(QStringLiteral("backButton_"));
            backButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            backButton_->setFlat(true);
            backButton_->setFocusPolicy(Qt::NoFocus);
            backButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            backButtonLayout_->addWidget(backButton_);
            
            verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
            backButtonLayout_->addItem(verticalSpacer);
            
            mainLayout_->addWidget(backButtonWidget_);
            
            avatarWidget_ = new QWidget(_profileWidget);
            avatarWidget_->setObjectName(QStringLiteral("avatarWidget_"));
            avatarWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
            avatarWidgetLayout_ = new QVBoxLayout(avatarWidget_);
            avatarWidgetLayout_->setSpacing(0);
            avatarWidgetLayout_->setObjectName(QStringLiteral("verticalLayout"));
            avatarWidgetLayout_->setContentsMargins(Utils::scale_value(48), Utils::scale_value(24), Utils::scale_value(32), 0);

            avatar_ = new QWidget(avatarWidget_);
            avatar_->setObjectName(QStringLiteral("avatar_"));
            avatar_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            avatarLayout_ = new QVBoxLayout(avatar_);
            avatarLayout_->setObjectName(QStringLiteral("avatarLayout_"));
            avatarLayout_->setContentsMargins(0, 0, 0, 0);
            avatarWidgetLayout_->addWidget(avatar_);
            
            actionsWidget_ = new QWidget(avatarWidget_);
            actionsWidget_->setObjectName(QStringLiteral("actionsWidget_"));
            actionsWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            actionsLayout_ = new QHBoxLayout(actionsWidget_);
            actionsLayout_->setObjectName(QStringLiteral("actionsLayout_"));
            actionsLayout_->setSpacing(0);
            actionsLayout_->setContentsMargins(0, 0, 0, 0);
            
            messageButton_ = new QPushButton(actionsWidget_);
            messageButton_->setObjectName(QStringLiteral("messageButton_"));
            messageButton_->setFlat(true);
            messageButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            messageButton_->setFocusPolicy(Qt::NoFocus);
            messageButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            actionsLayout_->addWidget(messageButton_);
            
            voiceButton_ = new QPushButton(actionsWidget_);
            voiceButton_->setObjectName(QStringLiteral("voiceButton_"));
            voiceButton_->setFlat(true);
            voiceButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            voiceButton_->setFocusPolicy(Qt::NoFocus);
            voiceButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            actionsLayout_->addWidget(voiceButton_);
            
#ifdef STRIP_VOIP
            voiceButton_->hide();
            actionsLayout_->setContentsMargins(Utils::scale_value(20), 0, 0, 0);
#endif //STRIP_VOIP

            videoButton_ = new QPushButton(actionsWidget_);
            videoButton_->setObjectName(QStringLiteral("videoButton_"));
            videoButton_->setFlat(true);
            videoButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            videoButton_->setFocusPolicy(Qt::NoFocus);
            videoButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            actionsLayout_->addWidget(videoButton_);

#ifdef STRIP_VOIP
            videoButton_->hide();
#endif //STRIP_VOIP
            
            avatarWidgetLayout_->addWidget(actionsWidget_);
            avatarWidgetLayout_->addWidget(ignoreListLabel_);
            avatarWidgetLayout_->addWidget(attach_uin_label);
			
            ignoreButton_ = new QPushButton(avatarWidget_);
            ignoreButton_->setObjectName(QStringLiteral("ignoreButton_"));
            ignoreButton_->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            ignoreButton_->setFlat(true);
            ignoreButton_->setFocusPolicy(Qt::NoFocus);
            ignoreButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            avatarWidgetLayout_->addWidget(ignoreButton_);
            
            spamButton_ = new QPushButton(avatarWidget_);
            spamButton_->setObjectName(QStringLiteral("spamButton_"));
            spamButton_->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            spamButton_->setFlat(true);
            spamButton_->setFocusPolicy(Qt::NoFocus);
            spamButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            avatarWidgetLayout_->addWidget(spamButton_);
            
            verticalSpacer_2 = new QSpacerItem(Utils::scale_value(20), Utils::scale_value(40), QSizePolicy::Minimum, QSizePolicy::Expanding);
            
            avatarWidgetLayout_->addItem(verticalSpacer_2);
            
            mainLayout_->addWidget(avatarWidget_);
            
            infoScrollArea_ = new QScrollArea(_profileWidget);
            infoScrollArea_->setObjectName(QStringLiteral("infoScrollArea_"));
            infoScrollArea_->setWidgetResizable(true);
            Utils::grabTouchWidget(infoScrollArea_->viewport(), true);
            
            infoWidget_ = new QWidget(infoScrollArea_);
            infoWidget_->setObjectName(QStringLiteral("infoWidget_"));
            infoWidget_->setGeometry(QRect(0, 0, Utils::scale_value(847), Utils::scale_value(707)));
            Utils::grabTouchWidget(infoWidget_);

            infoLayout_ = new QVBoxLayout(infoWidget_);
            infoLayout_->setSpacing(0);
            infoLayout_->setObjectName(QStringLiteral("infoLayout_"));
            infoLayout_->setContentsMargins(0, Utils::scale_value(19), Utils::scale_value(48), Utils::scale_value(48));
            
            headWidget_ = new QWidget(infoScrollArea_);
            Utils::grabTouchWidget(headWidget_);
            headWidget_->setObjectName(QStringLiteral("headWidget_"));
            headWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            headWidget_->setMaximumHeight(Utils::scale_value(72));
            headLayout_ = new QHBoxLayout(headWidget_);
            headLayout_->setSpacing(0);
            headLayout_->setObjectName(QStringLiteral("headLayout_"));
            headLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, Utils::scale_value(10));
            
            fullName_ = new TextEmojiWidget(headWidget_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(fullName_);
            fullName_->set_ellipsis(true);
            fullName_->set_selectable(true);
            headLayout_->addWidget(fullName_);
            
            optionsButton_ = new QPushButton(headWidget_);
            optionsButton_->setObjectName(QStringLiteral("optionsButton_"));
            optionsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            optionsButton_->setFlat(true);
            optionsButton_->setIconSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
            optionsButton_->setFocusPolicy(Qt::NoFocus);
            optionsButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            optionsMenu_ = new FlatMenu(optionsButton_);
            optionsMenu_->setExpandDirection(Qt::AlignLeft);
            optionsMenu_->setObjectName(QStringLiteral("optionsMenu_"));
            headLayout_->addWidget(optionsButton_);
            
            infoLayout_->addWidget(headWidget_);
            
            statusWidget_ = new QWidget(infoScrollArea_);
            Utils::grabTouchWidget(statusWidget_);
            statusWidget_->setObjectName(QStringLiteral("statusWidget_"));
            statusWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            statusWidget_->setMinimumSize(QSize(0, Utils::scale_value(45)));
            statusWidget_->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
            statusLayout_ = new QHBoxLayout(statusWidget_);
            statusLayout_->setSpacing(0);
            statusLayout_->setObjectName(QStringLiteral("info_state_area_ayout"));
            statusLayout_->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            
            statusButton_ = new QPushButton(statusWidget_);
            statusButton_->setObjectName(QStringLiteral("statusButton_"));
            statusButton_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            statusButton_->setMinimumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
            statusButton_->setMaximumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
            statusButton_->setFlat(true);
            statusButton_->setIconSize(QSize(Utils::scale_value(24), Utils::scale_value(24)));
            statusButton_->setFocusPolicy(Qt::NoFocus);
            statusButton_->setCursor(Qt::CursorShape::PointingHandCursor);
            statusMenu_ = new FlatMenu(statusButton_);
            statusMenu_->setObjectName(QStringLiteral("statusMenu_"));
            statusLayout_->addWidget(statusButton_);
            
            statusLabel_ = new QLabel(statusWidget_);
            Utils::grabTouchWidget(statusLabel_);
            statusLabel_->setObjectName(QStringLiteral("statusLabel_"));
            statusLayout_->addWidget(statusLabel_);
            
            horizontalSpacer = new QSpacerItem(Utils::scale_value(40), Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);
            statusLayout_->addItem(horizontalSpacer);
            
            infoLayout_->addWidget(statusWidget_);

            infoValuesWidget_ = new QWidget(infoScrollArea_);
            Utils::grabTouchWidget(infoValuesWidget_);
            infoValuesWidget_->setObjectName(QStringLiteral("infoValuesWidget_"));
            infoValuesWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
            infoValuesLayout_ = new QVBoxLayout(infoValuesWidget_);
            infoValuesLayout_->setAlignment(Qt::AlignTop);
            infoValuesLayout_->setObjectName(QStringLiteral("infoValuesLayout_"));
            infoValuesLayout_->setSpacing(0);
            infoValuesLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, 0);

            auto fieldRoutine1 = [](QWidget* _parent, QLayout* _parentLayout, TextEmojiWidget*& _fieldHead, TextEmojiWidget*& _field, int _dy)
            {
                _fieldHead = new TextEmojiWidget(_parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14), QColor("#7f282828"), Utils::scale_value(36 + _dy));
                _fieldHead->set_ellipsis(true);
                Utils::grabTouchWidget(_fieldHead);
                _parentLayout->addWidget(_fieldHead);
                
                _field = new TextEmojiWidget(_parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#282828"), Utils::scale_value(20));
                _field->set_ellipsis(true);
                _field->set_selectable(true);
                Utils::grabTouchWidget(_field);
                _parentLayout->addWidget(_field);
            };

            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, uinHead_, uin_, -10);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, phoneHead_, phone_, 0);
			
            attach_phone_label = new TextEmojiWidget(infoValuesWidget_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#579e1c"), Utils::scale_value(20));

            Utils::grabTouchWidget(attach_phone_label);
            attach_phone_label->setCursor(Qt::PointingHandCursor);
            attach_phone_label->setText(QT_TRANSLATE_NOOP("profile_page", "Attach phone number"));
            attach_phone_label->setObjectName(QStringLiteral("attach_phone_label"));
            attach_phone_label->set_ellipsis(true);
            infoValuesLayout_->addWidget(attach_phone_label);  
			
            phoneBottom_ = new TextEmojiWidget(infoValuesWidget_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14), QColor("#7f282828"), Utils::scale_value(18));
            phoneBottom_->set_ellipsis(true);
            Utils::grabTouchWidget(phoneBottom_);
            infoValuesLayout_->addWidget(phoneBottom_);

            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, firstNameHead_, firstName_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, lastNameHead_, lastName_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, birthdateHead_, birthdate_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, genderHead_, gender_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, countryHead_, country_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, cityHead_, city_, 0);
            fieldRoutine1(infoValuesWidget_, infoValuesLayout_, aboutHead_, about_, 0);

            about_->set_ellipsis(false);
            about_->set_multiline(true);
            
            ignoreListLabel_ = new TextEmojiWidget(infoValuesWidget_, Utils::FontsFamily::SEGOE_UI,
                Utils::scale_value(18), QColor("#e30f04"), Utils::scale_value(44 - 18 + 10));
            Utils::grabTouchWidget(ignoreListLabel_);
            infoValuesLayout_->addWidget(ignoreListLabel_);
            ignoreListLabel_->setCursor(Qt::PointingHandCursor);
            ignoreListLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Ignore list"));
            ignoreListLabel_->setObjectName(QStringLiteral("ignoreListLabel_"));
            ignoreListLabel_->set_ellipsis(true);
            
            attach_uin_label = new TextEmojiWidget(infoValuesWidget_, Utils::FontsFamily::SEGOE_UI,
            Utils::scale_value(18), QColor("#579e1c"), Utils::scale_value(44 - 18 + 10));
            Utils::grabTouchWidget(attach_uin_label);
            infoValuesLayout_->addWidget(attach_uin_label);            
            attach_uin_label->setCursor(Qt::PointingHandCursor);
            attach_uin_label->setText(QT_TRANSLATE_NOOP("profile_page", "Connect to ICQ account"));
            attach_uin_label->setObjectName(QStringLiteral("attach_uin_label"));
            attach_uin_label->set_ellipsis(true);
			
            infoLayout_->addWidget(infoValuesWidget_);

            mainLayout_->addWidget(infoScrollArea_);
            
            infoScrollArea_->setWidget(infoWidget_);
            
            retranslateUi(_profileWidget);
            
            QMetaObject::connectSlotsByName(_profileWidget);
        }
        
        void retranslateUi(QWidget *_profileWidget)
        {
            _profileWidget->setWindowTitle(QT_TRANSLATE_NOOP("profile_page", "Profile"));
            backButton_->setText("");
            messageButton_->setText("");
            voiceButton_->setText("");
            videoButton_->setText("");
            ignoreButton_->setText(QT_TRANSLATE_NOOP("profile_page", "Ignore"));
            spamButton_->setText(QT_TRANSLATE_NOOP("profile_page", "Report spam"));
            fullName_->setText("");
            optionsButton_->setText(QT_TRANSLATE_NOOP("profile_page", "Edit profile"));
            statusButton_->setText("");
            statusLabel_->setText("");
            uinHead_->setText(QT_TRANSLATE_NOOP("profile_page", "UIN"));
            uin_->setText("");
            phoneHead_->setText(QT_TRANSLATE_NOOP("profile_page", "Phone number"));
            phone_->setText("");
            phoneBottom_->setText(QT_TRANSLATE_NOOP("profile_page", "Only visible for those, who has it in the phone book"));
            firstNameHead_->setText(QT_TRANSLATE_NOOP("profile_page", "First name"));
            firstName_->setText("");
            lastNameHead_->setText(QT_TRANSLATE_NOOP("profile_page", "Last name"));
            lastName_->setText("");
            birthdateHead_->setText(QT_TRANSLATE_NOOP("profile_page", "Birthdate"));
            birthdate_->setText("");
            genderHead_->setText(QT_TRANSLATE_NOOP("profile_page", "Gender"));
            gender_->setText("");
            countryHead_->setText(QT_TRANSLATE_NOOP("profile_page", "Country"));
            country_->setText("");
            cityHead_->setText(QT_TRANSLATE_NOOP("profile_page", "City"));
            city_->setText("");
            aboutHead_->setText(QT_TRANSLATE_NOOP("profile_page", "About me"));
            about_->setText("");
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
        
        connect(Ui_->backButton_, &QPushButton::clicked, [this]() { emit Utils::InterConnector::instance().profileSettingsBack(); });

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(onAvatarLoaded(QString)), Qt::QueuedConnection);
        
        Ui_->statusMenu_->addAction(QIcon(":/resources/content_status_online_200.png"), QT_TRANSLATE_NOOP("profile_page", "Online"), this, SLOT(menuStateOnline()));
        Ui_->statusMenu_->addAction(QIcon(":/resources/content_status_dnd_200.png"), QT_TRANSLATE_NOOP("profile_page", "Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
        Ui_->statusMenu_->addAction(QIcon(":/resources/content_status_invisible_200.png"), QT_TRANSLATE_NOOP("profile_page", "Invisible"), this, SLOT(menuStateInvisible()));

        /*
        Ui_->optionsMenu_->addAction(QT_TRANSLATE_NOOP("profile_page", "Add to Favorites"));
        Ui_->optionsMenu_->addSeparator();
        */
        Ui_->optionsMenu_->addAction(QT_TRANSLATE_NOOP("profile_page", "Ignore"), this, SLOT(contactIgnore()));
        Ui_->optionsMenu_->addAction(QT_TRANSLATE_NOOP("profile_page", "Report Spam"), this, SLOT(contactSpam()));

        connect(Ui_->ignoreButton_, SIGNAL(clicked()), this, SLOT(contactIgnore()));
        connect(Ui_->spamButton_, SIGNAL(clicked()), this, SLOT(contactSpam()));

        Ui_->ignoreListLabel_->disconnect();
        Ui_->attach_phone_label->disconnect();
        Ui_->attach_uin_label->disconnect();

        Testing::setAccessibleName(Ui_->ignoreListLabel_, "IgnoreList");

        connect(Ui_->ignoreListLabel_, &TextEmojiWidget::clicked, [this]()
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

        connect(Ui_->attach_phone_label, &TextEmojiWidget::clicked, [this]()
        {
            emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachPhone);
        });

        connect(Ui_->attach_uin_label, &TextEmojiWidget::clicked, [this]()
        {
            emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachUin);
        });

        disconnector_->add("login_complete", QWidget::connect(GetDispatcher(), &core_dispatcher::login_complete, [this]
        {
            if (needRequestAgain_ && isVisible())
                updateInterface(uin_);
        }));
        
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsUpdateInterface, this, &ProfileSettingsWidget::updateProfile, Qt::QueuedConnection);

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
    }

    void ProfileSettingsWidget::contactSpam()
    {
        emit Utils::InterConnector::instance().profileSettingsUnknownSpam(uin_);
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
    
    void ProfileSettingsWidget::updateProfile()
    {
        updateInterface(uin_);
    }

    void ProfileSettingsWidget::updateInterface(const QString &_uin)
    {
        needRequestAgain_ = false;
        
        if (_uin.length())
        {
            disconnect(SIGNAL(myInfo()));
            disconnect(SIGNAL(needLogin()));
            disconnect(SIGNAL(recvFlags(int)));
        }
        else
        {
            myInfo();
            core::coll_helper helper(GetDispatcher()->create_collection(), true);
            Ui_->attach_uin_label->setVisible(false);
            GetDispatcher()->post_message_to_core("load_flags", helper.get());
            connect(GetDispatcher(), SIGNAL(myInfo()), this, SLOT(myInfo()));
            connect(GetDispatcher(), SIGNAL(needLogin()), this, SLOT(setStateOffline()));
            connect(GetDispatcher(), SIGNAL(recvFlags(int)), this, SLOT(recvFlags(int)));
        }
        
        uin_ = _uin;

        Ui_->backButtonWidget_->setVisible(_uin.length());
        Ui_->actionsWidget_->setVisible(_uin.length());
        Ui_->ignoreButton_->setVisible(false);
        Ui_->spamButton_->setVisible(false);
        Ui_->statusWidget_->setVisible(false);
        Ui_->ignoreListLabel_->setVisible(_uin.isEmpty());
		Ui_->attach_phone_label->setVisible(false);
        Ui_->attach_uin_label->setVisible(false);

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

        actionButtonState_ = _uin.length() ? USER_ACTIONS : EDIT_PROFILE;
        updateActionButton();

        if (USER_ACTIONS)
        {
            Logic::GetContactListModel()->get_contact_profile(_uin, [this](Logic::profile_ptr _profile, int32_t /*error*/)
            {
                if (_profile)
                    parse(_profile);
                else
                    needRequestAgain_ = true;
            });
        }
    }

    void ProfileSettingsWidget::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent(_event);
        
        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(geometry().x() - 1, geometry().y() - 1, visibleRegion().boundingRect().width() + 2, visibleRegion().boundingRect().height() + 2);
    }

    bool ProfileSettingsWidget::event(QEvent* _event)
    {
        if (_event->type() == QMouseEvent::Wheel)
        {
            auto barValue = Ui_->infoScrollArea_->verticalScrollBar()->value();
            if (!Ui_->infoScrollArea_->geometry().contains(mapFromGlobal(QCursor::pos())))
            {
                QWheelEvent* wheel = static_cast< QWheelEvent* >(_event);
                auto nval = (barValue - wheel->delta());
                nval = std::max(nval, Ui_->infoScrollArea_->verticalScrollBar()->minimum());
                nval = std::min(nval, Ui_->infoScrollArea_->verticalScrollBar()->maximum());
                Ui_->infoScrollArea_->verticalScrollBar()->setValue(nval);
            }
        }
        return QWidget::event(_event);
    }

    void ProfileSettingsWidget::parse(Logic::profile_ptr _profile)
    {
        auto contact = Logic::GetContactListModel()->getContactItem(_profile->get_aimid());

        auto contactIsSelf = (actionButtonState_ == EDIT_PROFILE);

        // Set common values
        {
            if (contact && contact->Get()->GetDisplayName().length())
                setFullName(contact->Get()->GetDisplayName());
            else if (_profile->get_contact_name().length())
                setFullName(_profile->get_contact_name());
            else if (_profile->get_displayid().length())
                setFullName(_profile->get_displayid());
            else if (_profile->get_friendly().length())
                setFullName(_profile->get_friendly());
            else
                setFullName(QString("%1%2%3").arg(_profile->get_first_name()).arg(_profile->get_first_name().length() ? " " : "").arg(_profile->get_last_name()));
            
            setICQNumber(_profile->get_aimid());

            if (!contactIsSelf)
            {
                setPhone(!_profile->get_phones().empty() ? _profile->get_phones().front().get_phone() : "", contactIsSelf);
            }
            else
            {
                if (MyInfo()->phoneNumber().isEmpty())
                {
                    setPhone(MyInfo()->phoneNumber(), contactIsSelf);
                }
                else
                {
                    setPhone("+" + MyInfo()->phoneNumber(), contactIsSelf);
                }
            }

            setFirstName(_profile->get_first_name());
            setLastName(_profile->get_last_name());
            setBirthdate(_profile->get_birthdate() ? Utils::GetTranslator()->formatDate(QDateTime::fromMSecsSinceEpoch(_profile->get_birthdate() * 1000, Qt::LocalTime).date(), false) : "");
            setGender(_profile->get_gender());
            setCountry(_profile->get_origin_address().get_country());
            setCity(_profile->get_origin_address().get_city());
            setAbout(_profile->get_about());
        }
        
        auto uin = _profile->get_aimid();
        
        Ui_->statusButton_->setCursor(contactIsSelf ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);

        if (actionButtonState_ == EDIT_PROFILE)
        {
            Ui_->statusWidget_->setVisible(true);
            Utils::ApplyPropertyParameter(Ui_->statusLabel_, "green", true);

            GetDispatcher()->disconnect(SIGNAL(signedUrl(QString)));
            connect(GetDispatcher(), &core_dispatcher::signedUrl, [](QString url)
            {
                //puts(url.toStdString().c_str());
                
                ((Utils::Application *)qApp)->unsetUrlHandler();
                QDesktopServices::openUrl(url);
                ((Utils::Application *)qApp)->setUrlHandler();
            });
            Ui_->optionsButton_->disconnect();
            connect(Ui_->optionsButton_, &QPushButton::clicked, [uin]()
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
           
            Ui_->messageButton_->disconnect();
            Ui_->voiceButton_->disconnect();
            Ui_->videoButton_->disconnect();
        }
        else if (actionButtonState_ == USER_ACTIONS)
        {
            auto contactIsFriend = ((actionButtonState_ == USER_ACTIONS) && (contact && !contact->is_not_auth()));
            Ui_->statusWidget_->setVisible(contactIsFriend);

            auto contactIsUnknown = ((actionButtonState_ == USER_ACTIONS) && !contactIsSelf && !contactIsFriend);
            auto contactIsOnline = (contact && contact->is_online());
            GetDispatcher()->disconnect(SIGNAL(signedUrl(QString)));
            
            Ui_->statusLabel_->setText("");
            if (contactIsFriend && contact && contact->Get())
            {
                Ui_->statusButton_->setVisible(contactIsOnline);
                if (!contact->is_online())
                {
                    QString state = QT_TRANSLATE_NOOP("profile_page", "Seen ");
                    QDateTime lastSeen = contact->Get()->GetLastSeen();
                    if (lastSeen.isValid())
                    {
                        const auto current = QDateTime::currentDateTime();
                        const auto days = lastSeen.daysTo(current);
                        if (days == 0)
                            state += QT_TRANSLATE_NOOP("profile_page", "today");
                        else if (days == 1)
                            state += QT_TRANSLATE_NOOP("profile_page", "yesterday");
                        else
                            state += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
                        if (lastSeen.date().year() == current.date().year())
                        {
                            state += QT_TRANSLATE_NOOP("profile_page", " at ");
                            state += lastSeen.time().toString(Qt::SystemLocaleShortDate);
                        }

                        Utils::ApplyPropertyParameter(Ui_->statusLabel_, "green", true);
                        Ui_->statusLabel_->setText(state);
                    }
                }
            }
            if (Ui_->statusLabel_->text().isEmpty())
            {
                if (contactIsOnline)
                {
                    Utils::ApplyPropertyParameter(Ui_->statusLabel_, "green", true);
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
                    Utils::ApplyPropertyParameter(Ui_->statusLabel_, "green", false);
                    Ui_->statusLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Offline"));
                }
            }
            
            Ui_->statusButton_->setVisible(false);
            
            Ui_->messageButton_->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::PointingHandCursor);
            Utils::ApplyPropertyParameter(Ui_->messageButton_, "known", contactIsFriend);

            Ui_->voiceButton_->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);
            Utils::ApplyPropertyParameter(Ui_->voiceButton_, "known", contactIsFriend);

            Ui_->videoButton_->setCursor(contactIsFriend ? Qt::CursorShape::PointingHandCursor : Qt::CursorShape::ArrowCursor);
            Utils::ApplyPropertyParameter(Ui_->videoButton_, "known", contactIsFriend);
            
            Ui_->ignoreButton_->setVisible(contactIsUnknown);
            Ui_->spamButton_->setVisible(contactIsUnknown);

            if (contactIsUnknown)
            {
                auto uin = uin_;
                Ui_->messageButton_->disconnect();
                connect(Ui_->messageButton_, &QPushButton::clicked, [uin]()
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
                Ui_->voiceButton_->disconnect();
                Ui_->videoButton_->disconnect();
            }
            else
            {
                Ui_->messageButton_->disconnect();
                connect(Ui_->messageButton_, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();
                });
                Ui_->voiceButton_->disconnect();
                connect(Ui_->voiceButton_, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();
                    
                    QTimer::singleShot(500, [uin]() { Ui::GetDispatcher()->getVoipController().setStartA(uin.toUtf8(), false); });
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_call);
                });
                Ui_->videoButton_->disconnect();
                connect(Ui_->videoButton_, &QPushButton::clicked, [uin]()
                {
                    Logic::GetContactListModel()->setCurrent(uin, true);
                    emit Utils::InterConnector::instance().profileSettingsBack();

                    QTimer::singleShot(500, [uin]() { Ui::GetDispatcher()->getVoipController().setStartV(uin.toUtf8(), false); });
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_video_call);
                });
            }
        }
        updateActionButton();

        auto dname = Ui_->fullName_->text();
        if (!avatar_)
        {
            avatar_ = new ContactAvatarWidget(Ui_->avatar_, uin, dname, Utils::scale_value(180));
            Ui_->avatarLayout_->addWidget(avatar_);
        }
        else
        {
            avatar_->UpdateParams(uin_, dname);
        }
    }
    
    void ProfileSettingsWidget::onAvatarLoaded(QString _uin)
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
        Ui_->statusButton_->setVisible(true);
        Ui_->statusButton_->setIcon(QIcon(":/resources/content_status_online_200.png"));
        Ui_->statusLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Online"));
    }
    
    void ProfileSettingsWidget::setStateOffline()
    {
        Ui_->statusButton_->setVisible(true);
        Ui_->statusButton_->setIcon(QIcon(":/resources/content_status_offline_200.png"));
        Ui_->statusLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Offline"));
    }
    
    void ProfileSettingsWidget::setStateDoNotDisturb()
    {
        Ui_->statusButton_->setVisible(true);
        Ui_->statusButton_->setIcon(QIcon(":/resources/content_status_dnd_200.png"));
        Ui_->statusLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Do not disturb"));
    }
    
    void ProfileSettingsWidget::setStateInvisible()
    {
        Ui_->statusButton_->setVisible(true);
        Ui_->statusButton_->setIcon(QIcon(":/resources/content_status_invisible_200.png"));
        Ui_->statusLabel_->setText(QT_TRANSLATE_NOOP("profile_page", "Invisible"));
    }
    
    void ProfileSettingsWidget::setFullName(const QString& _val)
    {
        Ui_->fullName_->setText(_val);
    }

    void ProfileSettingsWidget::setICQNumber(const QString& _val)
    {
        Ui_->uin_->setText(_val);
        Ui_->uin_->setVisible(_val.length());
        Ui_->uinHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setPhone(const QString& _val, bool _forSelf)
    {
        Ui_->phone_->setText(_val);
        Ui_->phone_->setVisible(_val.length());
        Ui_->phoneHead_->setVisible(_forSelf || !_val.isEmpty());
        Ui_->attach_phone_label->setVisible(_forSelf && _val.isEmpty());

        if (_forSelf)
        {
            QString phone_text = "";
            if (_val.length())
            {
                phone_text = QT_TRANSLATE_NOOP("profile_page", "Only visible for those, who has it in the phone book");
            }
            else
            {
                phone_text = QT_TRANSLATE_NOOP("profile_page", "for safety and spam protection");
            }

            Ui_->phoneBottom_->setText(phone_text);
        }

        Ui_->phoneBottom_->setVisible(_forSelf);
    }

    void ProfileSettingsWidget::setFirstName(const QString& _val)
    {
        Ui_->firstName_->setText(_val);
        Ui_->firstName_->setVisible(_val.length());
        Ui_->firstNameHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setLastName(const QString& _val)
    {
        Ui_->lastName_->setText(_val);
        Ui_->lastName_->setVisible(_val.length());
        Ui_->lastNameHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setBirthdate(const QString& _val)
    {
        Ui_->birthdate_->setText(_val);
        Ui_->birthdate_->setVisible(_val.length());
        Ui_->birthdateHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setGender(const QString& _val)
    {
        if (_val.toLower() == "male")
            Ui_->gender_->setText(QT_TRANSLATE_NOOP("profile_page", "Male"));
        else if (_val.toLower() == "female")
            Ui_->gender_->setText(QT_TRANSLATE_NOOP("profile_page", "Female"));
        else
            Ui_->gender_->setText("");
        Ui_->gender_->setVisible(Ui_->gender_->text().length());
        Ui_->genderHead_->setVisible(Ui_->gender_->text().length());
    }

    void ProfileSettingsWidget::setCountry(const QString& _val)
    {
        Ui_->country_->setText(_val);
        Ui_->country_->setVisible(_val.length());
        Ui_->countryHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setCity(const QString& _val)
    {
        Ui_->city_->setText(_val);
        Ui_->city_->setVisible(_val.length());
        Ui_->cityHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::setAbout(const QString& _val)
    {
        Ui_->about_->setText(_val);
        Ui_->about_->setVisible(_val.length());
        Ui_->aboutHead_->setVisible(_val.length());
    }

    void ProfileSettingsWidget::updateActionButton()
    {
        if (actionButtonState_ == EDIT_PROFILE)
        {
            Ui_->optionsButton_->setText(QT_TRANSLATE_NOOP("profile_page", "Edit profile"));
            Ui_->optionsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            Ui_->optionsButton_->setMenu(nullptr);

            Ui_->avatarWidget_->setMinimumWidth(Utils::scale_value(260));
            Ui_->avatarWidgetLayout_->setContentsMargins(Utils::scale_value(48), Utils::scale_value(24), Utils::scale_value(32), 0);

            // temporary disabled
            Ui_->statusButton_->setMenu(Ui_->statusMenu_);
            Ui_->statusLayout_->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            // remove lines below after set_state is implemented.
            /*
            Ui_->statusButton_->setMenu(nullptr);
            Ui_->statusButton_->setVisible(false);
            Ui_->statusLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
            */
        }
        else if (actionButtonState_ == USER_ACTIONS)
        {
            Ui_->optionsButton_->setText("");
            Ui_->optionsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            Ui_->optionsButton_->setMenu(Ui_->optionsMenu_);

            Ui_->avatarWidget_->setMinimumWidth(Utils::scale_value(236));
            Ui_->avatarWidgetLayout_->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), Utils::scale_value(32), 0);

            Ui_->statusButton_->setMenu(nullptr);
            if (Ui_->statusButton_->isVisible())
                Ui_->statusLayout_->setContentsMargins(Utils::scale_value(2), 0, 0, 0);
            else
                Ui_->statusLayout_->setContentsMargins(Utils::scale_value(16), 0, 0, 0);
        }
        Utils::ApplyPropertyParameter(Ui_->optionsButton_, "self", actionButtonState_ == EDIT_PROFILE);
        Utils::ApplyPropertyParameter(Ui_->statusButton_, "self", actionButtonState_ == EDIT_PROFILE);
        Utils::ApplyPropertyParameter(Ui_->statusLabel_, "self", actionButtonState_ == EDIT_PROFILE);
    }

    void ProfileSettingsWidget::recvFlags(int _flags)
    {
        if (actionButtonState_ == EDIT_PROFILE)
        {
            Ui_->attach_uin_label->setVisible(_flags & 0x1000);
        }
    }
}
