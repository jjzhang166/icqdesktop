#include "stdafx.h"
#include "SettingsTab.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CustomButton.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/log/log.h"

namespace
{
    const QString CL_STYLE = QString(
        "border-right-style: solid; border-right-color: #dadada; border-right-width: 1dip;"
    );
}

namespace Ui
{
    class SettingsTab::UI
    {
    private:
        QWidget *settingsView;
        QLabel *topicLabel;
        CustomButton *myProfileButton;
        CustomButton *generalButton;
        CustomButton *voipButton;
        CustomButton *notificationsButton;
        CustomButton *themesButton;
        QWidget *horLineWidget;
        QHBoxLayout *horLineLayout;
        QFrame *line;
        CustomButton *aboutButton;
        CustomButton *contactUsButton;
        CustomButton *signoutButton;
        
        friend class SettingsTab;

    public:
        void init(QWidget* _p, QLayout* _pl)
        {
            topicLabel = new QLabel(_p);
            Utils::grabTouchWidget(topicLabel);
            topicLabel->setObjectName(QStringLiteral("settings_topic_label"));
            topicLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            _pl->addWidget(topicLabel);
            
            myProfileButton = new CustomButton(settingsView, ":/resources/contr_profile_100.png");
            Testing::setAccessibleName(myProfileButton, "settings_my_profile_button");

            Utils::grabTouchWidget(myProfileButton);
            myProfileButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            myProfileButton->setMouseTracking(true);
            myProfileButton->setFlat(true);
            myProfileButton->setCheckable(true);
            myProfileButton->setAlign(Qt::AlignLeft);
            myProfileButton->setOffsets(Utils::scale_value(24), 0);
            myProfileButton->setFocusPolicy(Qt::NoFocus);
            myProfileButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(myProfileButton);
            
            generalButton = new CustomButton(settingsView, ":/resources/tabs_settings_100.png");
            Utils::grabTouchWidget(generalButton);
            generalButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            generalButton->setMouseTracking(true);
            generalButton->setFlat(true);
            generalButton->setCheckable(true);
            generalButton->setAlign(Qt::AlignLeft);
            generalButton->setOffsets(Utils::scale_value(24), 0);
            generalButton->setFocusPolicy(Qt::NoFocus);
            generalButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(generalButton);

            themesButton = new CustomButton(settingsView, ":/resources/contr_themes_100.png");
            themesButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            themesButton->setMouseTracking(true);
            themesButton->setFlat(true);
            themesButton->setCheckable(true);
            themesButton->setAlign(Qt::AlignLeft);
            themesButton->setOffsets(Utils::scale_value(24), 0);
            themesButton->setFocusPolicy(Qt::NoFocus);
            themesButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(themesButton);

            notificationsButton = new CustomButton(settingsView, ":/resources/contr_notifysettings_100.png");
            Utils::grabTouchWidget(notificationsButton);
            notificationsButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            notificationsButton->setMouseTracking(true);
            notificationsButton->setFlat(true);
            notificationsButton->setCheckable(true);
            notificationsButton->setAlign(Qt::AlignLeft);
            notificationsButton->setOffsets(Utils::scale_value(24), 0);
            notificationsButton->setFocusPolicy(Qt::NoFocus);
            notificationsButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(notificationsButton);
            
            voipButton = new CustomButton(settingsView, ":/resources/contr_video_100.png");
            Utils::grabTouchWidget(voipButton);
            voipButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            voipButton->setMouseTracking(true);
            voipButton->setFlat(true);
            voipButton->setCheckable(true);
            voipButton->setAlign(Qt::AlignLeft);
            voipButton->setOffsets(Utils::scale_value(24), 0);
            voipButton->setFocusPolicy(Qt::NoFocus);
            voipButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(voipButton);
            
            horLineWidget = new QWidget(settingsView);
            Utils::grabTouchWidget(horLineWidget);
            horLineLayout = new QHBoxLayout(horLineWidget);
            horLineLayout->setContentsMargins(Utils::scale_value(15), Utils::scale_value(15), Utils::scale_value(15), Utils::scale_value(15));
            line = new QFrame(horLineWidget);
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            horLineLayout->addWidget(line);
            _pl->addWidget(horLineWidget);
            
            aboutButton = new CustomButton(settingsView, ":/resources/contr_about_100.png");
            Utils::grabTouchWidget(aboutButton);
            aboutButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            aboutButton->setMouseTracking(true);
            aboutButton->setFlat(true);
            aboutButton->setCheckable(true);
            aboutButton->setAlign(Qt::AlignLeft);
            aboutButton->setOffsets(Utils::scale_value(24), 0);
            aboutButton->setFocusPolicy(Qt::NoFocus);
            aboutButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(aboutButton);

            contactUsButton = new CustomButton(settingsView, ":/resources/contr_contact_us_100.png");
            Utils::grabTouchWidget(contactUsButton);
            contactUsButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            contactUsButton->setMouseTracking(true);
            contactUsButton->setFlat(true);
            contactUsButton->setCheckable(true);
            contactUsButton->setAlign(Qt::AlignLeft);
            contactUsButton->setOffsets(Utils::scale_value(24), 0);
            contactUsButton->setFocusPolicy(Qt::NoFocus);
            contactUsButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(contactUsButton);
            Testing::setAccessibleName(contactUsButton, "settings_contact_us_button");

            signoutButton = new CustomButton(settingsView, ":/resources/contr_signout_100.png");
            Utils::grabTouchWidget(signoutButton);
            signoutButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            signoutButton->setMouseTracking(true);
            signoutButton->setFlat(true);
            signoutButton->setCheckable(true);
            signoutButton->setAlign(Qt::AlignLeft);
            signoutButton->setOffsets(Utils::scale_value(24), 0);
            signoutButton->setFocusPolicy(Qt::NoFocus);
            signoutButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(signoutButton);
            Testing::setAccessibleName(signoutButton, "settings_signout_button");
            
            topicLabel->setText(QT_TRANSLATE_NOOP("settings_pages","Settings"));
            myProfileButton->setText(QT_TRANSLATE_NOOP("settings_pages", "My profile"));
            generalButton->setText(QT_TRANSLATE_NOOP("settings_pages", "General"));
            themesButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
            voipButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
            notificationsButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
            aboutButton->setText(QT_TRANSLATE_NOOP("settings_pages", "About"));
            contactUsButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Contact Us"));
            signoutButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Change account"));
        }
        
        void retranslateUi(QWidget* _settingsPage)
        {
            _settingsPage->setWindowTitle("");
            topicLabel->setText(QT_TRANSLATE_NOOP("settings_pages", "Settings"));
            myProfileButton->setText(QT_TRANSLATE_NOOP("settings_pages", "My profile"));
            generalButton->setText(QT_TRANSLATE_NOOP("settings_pages", "General"));
            themesButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
            voipButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
            notificationsButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
            aboutButton->setText(QT_TRANSLATE_NOOP("settings_pages", "About"));
            signoutButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Change account"));
        }
    };
    
    SettingsTab::SettingsTab(QWidget* _parent):
        QWidget(_parent),
        ui_(new UI()),
        currentSettingsItem_(Utils::CommonSettingsType::CommonSettingsType_None)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/contact_list/SettingsTab.qss"));

        auto scrollArea = CreateScrollAreaAndSetTrScrollBar(this);
        Utils::ApplyStyle(scrollArea, CL_STYLE);
        scrollArea->setObjectName(QStringLiteral("scroll_area"));
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::grabTouchWidget(scrollArea->viewport(), true);
        
        auto scrollAreaContent = new QWidget(scrollArea);
        scrollAreaContent->setObjectName(QStringLiteral("settings_view"));
        scrollAreaContent->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scrollAreaContent);
        
        auto scrollAreaContentLayout = new QVBoxLayout(scrollAreaContent);
        scrollAreaContentLayout->setSpacing(0);
        scrollAreaContentLayout->setAlignment(Qt::AlignTop);
        scrollAreaContentLayout->setContentsMargins(Utils::scale_value(0), 0, 0, Utils::scale_value(0));
        
        scrollArea->setWidget(scrollAreaContent);
        
        auto layout = new QVBoxLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scrollArea);
        
        ui_->init(scrollAreaContent, scrollAreaContentLayout);
                
        connect(ui_->myProfileButton, SIGNAL(toggled(bool)), this, SLOT(settingsProfileClicked()), Qt::QueuedConnection);
        connect(ui_->generalButton, SIGNAL(toggled(bool)), this, SLOT(settingsGeneralClicked()), Qt::QueuedConnection);
        connect(ui_->voipButton, SIGNAL(toggled(bool)), this, SLOT(settingsVoiceVideoClicked()), Qt::QueuedConnection);
        connect(ui_->notificationsButton, SIGNAL(toggled(bool)), this, SLOT(settingsNotificationsClicked()), Qt::QueuedConnection);
        connect(ui_->themesButton, SIGNAL(toggled(bool)), this, SLOT(settingsThemesClicked()), Qt::QueuedConnection);
        connect(ui_->aboutButton, SIGNAL(toggled(bool)), this, SLOT(settingsAboutClicked()), Qt::QueuedConnection);
        connect(ui_->contactUsButton, SIGNAL(toggled(bool)), this, SLOT(settingsContactUsClicked()), Qt::QueuedConnection);
        connect(ui_->signoutButton, SIGNAL(toggled(bool)), this, SLOT(settingsSignoutClicked()), Qt::QueuedConnection);

#ifdef STRIP_VOIP
        ui_->voipButton->hide();
#endif //STRIP_VOIP
    }
    
    SettingsTab::~SettingsTab() throw()
    {
        //
    }
    
    void SettingsTab::cleanSelection()
    {
        emit Utils::InterConnector::instance().popPagesToRoot();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_None;
        updateSettingsState();
    }
    
    void SettingsTab::settingsProfileClicked()
    {
        if (currentSettingsItem_ != Utils::CommonSettingsType::CommonSettingsType_Profile
            && currentSettingsItem_ != Utils::CommonSettingsType::CommonSettingsType_None)
            emit Utils::InterConnector::instance().generalSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Profile;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().profileSettingsShow("");
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_open);
    }
    
    void SettingsTab::settingsGeneralClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_General;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
    }
    
    void SettingsTab::settingsVoiceVideoClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_VoiceVideo;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }
    
    void SettingsTab::settingsNotificationsClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Notifications;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Notifications);
    }
    
    void SettingsTab::settingsThemesClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();
        
        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Themes;
        updateSettingsState();

        emit Utils::InterConnector::instance().themesSettingsShow(false, "");
    }
    
    void SettingsTab::settingsAboutClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_About;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_About);
    }

    void SettingsTab::settingsContactUsClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();
        
        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_ContactUs;
        updateSettingsState();
        
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_ContactUs);
    }

    void SettingsTab::settingsSignoutClicked()
    {
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Sign out"),
            NULL);

        if (confirm)
        {
            disconnect(ui_->signoutButton, SIGNAL(toggled(bool)), this, SLOT(settingsSignoutClicked()));

            if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
                emit Utils::InterConnector::instance().profileSettingsBack();
            else
                emit Utils::InterConnector::instance().generalSettingsBack();
            currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_None;
            updateSettingsState();
            get_gui_settings()->set_value(settings_feedback_email, QString(""));
            GetDispatcher()->post_message_to_core("logout", nullptr);
        }
        else
        {
            updateSettingsState();
        }
    }
    
    void SettingsTab::updateSettingsState()
    {
        ui_->myProfileButton->blockSignals(true);
        ui_->generalButton->blockSignals(true);
        ui_->voipButton->blockSignals(true);
        ui_->notificationsButton->blockSignals(true);
        ui_->themesButton->blockSignals(true);
        ui_->aboutButton->blockSignals(true);
        ui_->contactUsButton->blockSignals(true);
        ui_->signoutButton->blockSignals(true);
        
        ui_->myProfileButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile);
        ui_->generalButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_General);
        ui_->voipButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        ui_->notificationsButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Notifications);
        ui_->themesButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Themes);
        ui_->aboutButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_About);
        ui_->contactUsButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_ContactUs);
        ui_->signoutButton->setChecked(false);
        
        ui_->myProfileButton->blockSignals(false);
        ui_->generalButton->blockSignals(false);
        ui_->voipButton->blockSignals(false);
        ui_->notificationsButton->blockSignals(false);
        ui_->themesButton->blockSignals(false);
        ui_->aboutButton->blockSignals(false);
        ui_->contactUsButton->blockSignals(false);
        ui_->signoutButton->blockSignals(false);
    }

}
