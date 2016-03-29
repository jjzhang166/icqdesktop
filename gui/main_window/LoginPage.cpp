#include "stdafx.h"
#include "LoginPage.h"

#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../gui_settings.h"
#include "../controls/CountrySearchCombobox.h"
#include "../controls/LineEditEx.h"
#include "../../corelib/core_face.h"
#include "../utils/gui_coll_helper.h"
#include "../controls/BackButton.h"
#include "../controls/CustomButton.h"
#include "../controls/PictureWidget.h"
#include "../controls/BackButton.h"

namespace
{
    enum LoginPagesIndex
    {
        SUBPAGE_PHONE_LOGIN_INDEX = 0,
        SUBPAGE_PHONE_CONF_INDEX = 1,
        SUBPAGE_UIN_LOGIN_INDEX = 2,
    };
}

namespace Ui
{
    LoginPage::LoginPage(QWidget* parent, bool is_login)
        : QWidget(parent)
        , country_code_(new LineEditEx(this))
        , phone_(new LineEditEx(this))
        , combobox_(new CountrySearchCombobox(this))
        , remaining_seconds_(0)
        , timer_(new QTimer(this))
        , is_login_(is_login)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss", Utils::get_scale_coefficient(), true));
        if (objectName().isEmpty())
            setObjectName(QStringLiteral("login_page"));
        setProperty("LoginPageWidget", QVariant(true));
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);

        auto back_button_widget = new QWidget(this);
        auto back_button_layout = new QHBoxLayout(back_button_widget);
        Utils::ApplyStyle(back_button_widget, "background-color: transparent;");
        back_button_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        back_button_layout->setSpacing(0);
        back_button_layout->setContentsMargins(Utils::scale_value(14), Utils::scale_value(14), 0, 0);
        back_button_layout->setAlignment(Qt::AlignLeft);
        {
            prev_page_link_ = new BackButton(back_button_widget);
            prev_page_link_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            prev_page_link_->setFlat(true);
            prev_page_link_->setFocusPolicy(Qt::NoFocus);
            prev_page_link_->setCursor(Qt::PointingHandCursor);
            {
                const QString s = "QPushButton { width: 20dip; height: 20dip; border: none; background-color: transparent; border-image: url(:/resources/contr_back_100.png); margin: 10dip; } QPushButton:hover { border-image: url(:/resources/contr_back_100_hover.png); } QPushButton#back_button:pressed { border-image: url(:/resources/contr_back_100_active.png); }";
                Utils::ApplyStyle(prev_page_link_, s);
            }
            back_button_layout->addWidget(prev_page_link_);
        }
        verticalLayout->addWidget(back_button_widget);

        if (is_login_)
        {
            QSpacerItem* verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            verticalLayout->addItem(verticalSpacer);
        }

        QWidget* main_widget = new QWidget(this);
        main_widget->setObjectName(QStringLiteral("main_widget"));
        main_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        main_widget->setProperty("CenterControlWidgetBack", QVariant(true));
        QHBoxLayout* main_layout = new QHBoxLayout(main_widget);
        main_layout->setSpacing(0);
        main_layout->setObjectName(QStringLiteral("main_layout"));
        main_layout->setContentsMargins(0, 0, 0, 0);

        if (is_login_)
        {
            QSpacerItem* horizontalSpacer_6 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            main_layout->addItem(horizontalSpacer_6);
        }

        QWidget* controls_widget = new QWidget(main_widget);
        controls_widget->setObjectName(QStringLiteral("controls_widget"));
        controls_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        controls_widget->setProperty("CenterContolWidget", QVariant(true));
        QVBoxLayout* controls_layout = new QVBoxLayout(controls_widget);
        controls_layout->setSpacing(0);
        controls_layout->setObjectName(QStringLiteral("controls_layout"));
        controls_layout->setContentsMargins(0, 0, 0, 0);

        PictureWidget* logo_widget = new PictureWidget(controls_widget, ":/resources/main_window/content_logo_100.png");
        logo_widget->setFixedHeight(Utils::scale_value(80));
        logo_widget->setFixedWidth(Utils::scale_value(80));
        controls_layout->addWidget(logo_widget);
        controls_layout->setAlignment(logo_widget, Qt::AlignHCenter);
        logo_widget->setVisible(is_login_);

        QLabel* welcome_label = new QLabel(controls_widget);
        welcome_label->setObjectName(QStringLiteral("welcome_label"));
        welcome_label->setAlignment(Qt::AlignCenter);
        welcome_label->setProperty("WelcomeTitle", QVariant(true));
        welcome_label->setVisible(is_login_);

        controls_layout->addWidget(welcome_label);

        hint_label_ = new QLabel(controls_widget);
        hint_label_->setObjectName(QStringLiteral("hint_label"));
    
        if (is_login_)
            hint_label_->setAlignment(Qt::AlignCenter);
        hint_label_->setProperty("ActionHintLabel", QVariant(true));

        controls_layout->addWidget(hint_label_);

        QWidget * center_widget = new QWidget(controls_widget);
        center_widget->setObjectName(QStringLiteral("center_widget"));
        center_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout * horizontalLayout = new QHBoxLayout(center_widget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);

        if (is_login_)
        {
            QSpacerItem* horizontalSpacer_9 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            horizontalLayout->addItem(horizontalSpacer_9);
        }

        login_staked_widget_ = new QStackedWidget(center_widget);
        login_staked_widget_->setObjectName(QStringLiteral("login_staked_widget"));
        login_staked_widget_->setProperty("LoginStackedWidget", QVariant(true));
        login_staked_widget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QWidget* phone_login_widget = new QWidget();
        phone_login_widget->setObjectName(QStringLiteral("phone_login_widget"));
        phone_login_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phone_login_layout = new QVBoxLayout(phone_login_widget);
        phone_login_layout->setSpacing(0);
        phone_login_layout->setObjectName(QStringLiteral("phone_login_layout"));
        phone_login_layout->setContentsMargins(0, 0, 0, 0);

        country_search_widget_ = new QWidget(phone_login_widget);
        country_search_widget_->setObjectName(QStringLiteral("country_search_widget"));
        country_search_widget_->setProperty("CountrySearchWidget", QVariant(true));
        QVBoxLayout* country_search_layout = new QVBoxLayout(country_search_widget_);
        country_search_layout->setSpacing(0);
        country_search_layout->setObjectName(QStringLiteral("country_search_layout"));
        country_search_layout->setContentsMargins(0, 0, 0, 0);

        phone_login_layout->addWidget(country_search_widget_);

        phone_widget_ = new QFrame(phone_login_widget);
        phone_widget_->setObjectName(QStringLiteral("phone_widget"));
        phone_widget_->setFocusPolicy(Qt::ClickFocus);
        phone_widget_->setFrameShape(QFrame::NoFrame);
        phone_widget_->setFrameShadow(QFrame::Plain);
        phone_widget_->setLineWidth(0);
        phone_widget_->setProperty("EnterPhoneWidget", QVariant(true));
        QHBoxLayout* phone_widget_layout = new QHBoxLayout(phone_widget_);
        phone_widget_layout->setSpacing(0);
        phone_widget_layout->setObjectName(QStringLiteral("phone_widget_layout"));
        phone_widget_layout->setContentsMargins(0, 0, 0, 0);

        phone_login_layout->addWidget(phone_widget_);

        QSpacerItem* verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phone_login_layout->addItem(verticalSpacer_3);

        login_staked_widget_->addWidget(phone_login_widget);
        QWidget* phone_confirm_widget = new QWidget();
        phone_confirm_widget->setObjectName(QStringLiteral("phone_confirm_widget"));
        phone_confirm_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phone_confirm_layout = new QVBoxLayout(phone_confirm_widget);
        phone_confirm_layout->setSpacing(0);
        phone_confirm_layout->setObjectName(QStringLiteral("phone_confirm_layout"));
        phone_confirm_layout->setContentsMargins(0, 0, 0, 0);

        QWidget* entered_phone_widget = new QWidget(phone_confirm_widget);
        entered_phone_widget->setObjectName(QStringLiteral("entered_phone_widget"));
        entered_phone_widget->setProperty("EnteredPhoneWidget", QVariant(true));
        QHBoxLayout* horizontalLayout_6 = new QHBoxLayout(entered_phone_widget);
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        QSpacerItem* horizontalSpacer_4 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_4);

        entered_phone_ = new QLabel(entered_phone_widget);
        entered_phone_->setObjectName(QStringLiteral("entered_phone"));
        entered_phone_->setProperty("EnteredPhoneNumber", QVariant(true));

        horizontalLayout_6->addWidget(entered_phone_);

        edit_phone_button_ = new QPushButton(entered_phone_widget);
        edit_phone_button_->setObjectName(QStringLiteral("edit_phone_button"));
        edit_phone_button_->setCursor(QCursor(Qt::PointingHandCursor));
        edit_phone_button_->setProperty("EditPhoneButton", QVariant(true));

        horizontalLayout_6->addWidget(edit_phone_button_);

        QSpacerItem* horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_5);

        phone_confirm_layout->addWidget(entered_phone_widget);

        resend_button_ = new QPushButton(phone_confirm_widget);
        resend_button_->setObjectName(QStringLiteral("resendButton"));
        resend_button_->setCursor(QCursor(Qt::PointingHandCursor));
        resend_button_->setFocusPolicy(Qt::StrongFocus);
        resend_button_->setProperty("ResendCodeButton", QVariant(true));

        phone_confirm_layout->addWidget(resend_button_);

        code_edit_ = new QLineEdit(phone_confirm_widget);
        code_edit_->setObjectName(QStringLiteral("code_edit"));
        code_edit_->setProperty("EnteredCode", QVariant(true));
        code_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        code_edit_->setAlignment(Qt::AlignCenter);
        Testing::setAccessibleName(code_edit_, "StartWindowSMScodeField");

        phone_confirm_layout->addWidget(code_edit_);

        QSpacerItem* verticalSpacer_4 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phone_confirm_layout->addItem(verticalSpacer_4);

        login_staked_widget_->addWidget(phone_confirm_widget);
        QWidget* uin_login_widget = new QWidget();
        uin_login_widget->setObjectName(QStringLiteral("uin_login_widget"));
        uin_login_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout * uin_login_layout = new QVBoxLayout(uin_login_widget);
        uin_login_layout->setSpacing(0);
        uin_login_layout->setObjectName(QStringLiteral("uin_login_layout"));
        uin_login_layout->setContentsMargins(0, 0, 0, 0);

        uin_login_edit_ = new QLineEdit(uin_login_widget);
        uin_login_edit_->setObjectName(QStringLiteral("uin_login_edit"));
        uin_login_edit_->setProperty("Uin", QVariant(true));
        Testing::setAccessibleName(uin_login_edit_, "StartWindowUinField");

        uin_login_layout->addWidget(uin_login_edit_);

        uin_password_edit_ = new QLineEdit(uin_login_widget);
        uin_password_edit_->setObjectName(QStringLiteral("uin_password_edit"));
        uin_password_edit_->setEchoMode(QLineEdit::Password);
        uin_password_edit_->setProperty("Password", QVariant(true));
        Testing::setAccessibleName(uin_password_edit_, "StartWindowPasswordField");

        uin_login_layout->addWidget(uin_password_edit_);

        keep_logged_ = new QCheckBox(uin_login_widget);
        keep_logged_->setObjectName(QStringLiteral("keep_logged"));
        keep_logged_->setVisible(is_login_);

        uin_login_layout->addWidget(keep_logged_);

        login_staked_widget_->addWidget(uin_login_widget);

        horizontalLayout->addWidget(login_staked_widget_);

        QSpacerItem* horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);

        controls_layout->addWidget(center_widget);

        QWidget* next_button_widget = new QWidget(controls_widget);

        next_button_widget->setObjectName(QStringLiteral("next_button_widget"));
        next_button_widget->setProperty("NextButtonWidget", QVariant(true));
        QVBoxLayout* verticalLayout_8 = new QVBoxLayout(next_button_widget);
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(0, 0, 0, 0);
        next_page_link_ = new QPushButton(next_button_widget);
        next_page_link_->setObjectName(QStringLiteral("next_page_link"));
        next_page_link_->setCursor(QCursor(Qt::PointingHandCursor));
        next_page_link_->setAutoDefault(true);
        next_page_link_->setDefault(false);
        Utils::ApplyStyle(next_page_link_, main_button_style);
        Testing::setAccessibleName(next_page_link_, "StartWindowLoginButton");

        verticalLayout_8->addWidget(next_page_link_);

        controls_layout->addWidget(next_button_widget);
        
        if (is_login_)
        {
            controls_layout->setAlignment(next_button_widget, Qt::AlignHCenter);
        }
        else
        {
            controls_layout->setAlignment(next_button_widget, Qt::AlignLeft);
        }

        QWidget* widget = new QWidget(controls_widget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setProperty("ErrorWidget", QVariant(true));
        QVBoxLayout* verticalLayout_7 = new QVBoxLayout(widget);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        error_label_ = new QLabel(widget);
        error_label_->setObjectName(QStringLiteral("error_label"));

        if (is_login_)
        {
            error_label_->setAlignment(Qt::AlignCenter);
        }
        else
        {
            error_label_->setAlignment(Qt::AlignLeft);
        }

        error_label_->setProperty("ErrorLabel", QVariant(true));

        verticalLayout_7->addWidget(error_label_);

        controls_layout->addWidget(widget);

        main_layout->addWidget(controls_widget);

        QSpacerItem* horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        main_layout->addItem(horizontalSpacer_7);

        verticalLayout->addWidget(main_widget);

        QSpacerItem* verticalSpacer_2 = new QSpacerItem(0, 3, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        QWidget* switch_login_widget = new QWidget(this);
        switch_login_widget->setObjectName(QStringLiteral("switch_login_widget"));
        switch_login_widget->setProperty("LoginButtonWidget", QVariant(true));
        QHBoxLayout* switch_login_layout = new QHBoxLayout(switch_login_widget);
        switch_login_layout->setSpacing(0);
        switch_login_layout->setObjectName(QStringLiteral("switch_login_layout"));
        switch_login_layout->setContentsMargins(0, 0, 0, 0);
        QSpacerItem* horizontalSpacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        switch_login_layout->addItem(horizontalSpacer);

        switch_login_link_ = new QPushButton(switch_login_widget);
        switch_login_link_->setObjectName(QStringLiteral("switch_login_link"));
        switch_login_link_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        switch_login_link_->setCursor(QCursor(Qt::PointingHandCursor));
        switch_login_link_->setProperty("SwitchLoginButton", QVariant(true));
        switch_login_link_->setVisible(is_login_);
        Testing::setAccessibleName(switch_login_link_, "StartWindowChangeLoginType");

        switch_login_layout->addWidget(switch_login_link_);

        QSpacerItem* horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        switch_login_layout->addItem(horizontalSpacer_2);

        verticalLayout->addWidget(switch_login_widget);

        login_staked_widget_->setCurrentIndex(2);

        QMetaObject::connectSlotsByName(this);

        //prev_page_link_->setText(QString());
        welcome_label->setText(QT_TRANSLATE_NOOP("login_page","Welcome to ICQ"));
        edit_phone_button_->setText(QT_TRANSLATE_NOOP("login_page","Edit"));
        code_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","Your code"));
        uin_login_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","UIN or Email"));
        uin_login_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        uin_password_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","Password"));
        uin_password_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);

        keep_logged_->setText(QT_TRANSLATE_NOOP("login_page","Keep me signed in"));
        keep_logged_->setChecked(get_gui_settings()->get_value(settings_keep_logged_in, true));
        connect(keep_logged_, &QCheckBox::toggled, [](bool v)
        {
            if (get_gui_settings()->get_value(settings_keep_logged_in, true) != v)
                get_gui_settings()->set_value(settings_keep_logged_in, v);
        });

        next_page_link_->setText(QT_TRANSLATE_NOOP("login_page","Continue"));
        Q_UNUSED(this);

        login_staked_widget_->setCurrentIndex(2);
        next_page_link_->setDefault(false);

        QMetaObject::connectSlotsByName(this);
        init();
    }

    LoginPage::~LoginPage()
    {
    }

    void LoginPage::enableKeepLogedIn()
    {
        //get_gui_settings()->set_value(settings_keep_logged_in, true);
    }

    void LoginPage::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Return &&
            (login_staked_widget_->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX ||
            login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX))
        {
            next_page_link_->click();
        }
        return QWidget::keyPressEvent(event);
    }

    void LoginPage::paintEvent(QPaintEvent* event)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(event);
    }

    void LoginPage::init()
    {
        QMap<QString, QString> countryCodes = Utils::GetCountryCodes();
        combobox_->setEditBoxClass("CountrySearchEdit");
        combobox_->setComboboxViewClass("CountrySearchView");
        combobox_->setClass("CountySearchWidgetInternal");
        combobox_->setPlaceholder(QT_TRANSLATE_NOOP("login_page","Type country or code"));
        country_search_widget_->layout()->addWidget(combobox_);
        combobox_->setSources(countryCodes);

        connect(combobox_, SIGNAL(selected(QString)), this, SLOT(countrySelected(QString)), Qt::QueuedConnection);
        connect(this, SIGNAL(country(QString)), this, SLOT(redrawCountryCode()), Qt::QueuedConnection);
        connect(next_page_link_, SIGNAL(clicked()), this, SLOT(nextPage()), Qt::QueuedConnection);
        connect(prev_page_link_, SIGNAL(clicked()), this, SLOT(prevPage()), Qt::QueuedConnection);
        connect(edit_phone_button_, SIGNAL(clicked()), this, SLOT(prevPage()), Qt::QueuedConnection);
        connect(edit_phone_button_, SIGNAL(clicked()), this, SLOT(stats_edit_phone()), Qt::QueuedConnection);
        connect(switch_login_link_, SIGNAL(clicked()), this, SLOT(switchLoginType()), Qt::QueuedConnection);
        connect(resend_button_, SIGNAL(clicked()), this, SLOT(sendCode()), Qt::QueuedConnection);
        connect(resend_button_, SIGNAL(clicked()), this, SLOT(stats_resend_sms()), Qt::QueuedConnection);
        connect(timer_, SIGNAL(timeout()), this, SLOT(updateTimer()), Qt::QueuedConnection);

        country_code_->setProperty("CountryCodeEdit", true);
        phone_->setProperty("PhoneNumberEdit", true);
        phone_->setAttribute(Qt::WA_MacShowFocusRect, false);
        phone_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","your phone number"));
        phone_widget_->layout()->addWidget(country_code_);
        phone_widget_->layout()->addWidget(phone_);
        Testing::setAccessibleName(phone_, "StartWindowPhoneNumberField");

        connect(country_code_, SIGNAL(focusIn()), this, SLOT(setPhoneFocusIn()), Qt::QueuedConnection);
        connect(country_code_, SIGNAL(focusOut()), this, SLOT(setPhoneFocusOut()), Qt::QueuedConnection);
        connect(phone_, SIGNAL(focusIn()), this, SLOT(setPhoneFocusIn()), Qt::QueuedConnection);
        connect(phone_, SIGNAL(focusOut()), this, SLOT(setPhoneFocusOut()), Qt::QueuedConnection);

        connect(uin_login_edit_, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::QueuedConnection);
        connect(uin_password_edit_, SIGNAL(textEdited(QString)), this, SLOT(clearErrors()), Qt::QueuedConnection);
        connect(code_edit_, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::QueuedConnection);
        connect(code_edit_, SIGNAL(textChanged(QString)), this, SLOT(codeEditChanged(QString)), Qt::QueuedConnection);
        connect(country_code_, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::QueuedConnection);
        connect(country_code_, SIGNAL(textEdited(QString)), this, SLOT(countryCodeChanged(QString)), Qt::QueuedConnection);
        connect(phone_, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::QueuedConnection);
        connect(phone_, SIGNAL(emptyTextBackspace()), this, SLOT(emptyPhoneRemove()), Qt::QueuedConnection);

        QObject::connect(Ui::GetDispatcher(), SIGNAL(getSmsResult(int64_t, int)), this, SLOT(getSmsResult(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResult(int64_t, int)), this, SLOT(loginResult(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResultAttachUin(int64_t, int)), this, SLOT(loginResultAttachUin(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResultAttachPhone(int64_t, int)), this, SLOT(loginResultAttachPhone(int64_t, int)), Qt::DirectConnection);

        country_code_->setValidator(new QRegExpValidator(QRegExp("[\\+\\d]\\d*")));
        phone_->setValidator(new QRegExpValidator(QRegExp("\\d*")));
        code_edit_->setValidator(new QRegExpValidator(QRegExp("\\d*")));

        combobox_->selectItem(Utils::GetTranslator()->getCurrentPhoneCode());
        error_label_->hide();
        phone_->setFocus();
        country_code_->setFocusPolicy(Qt::ClickFocus);
        initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
    }

    void LoginPage::initLoginSubPage(int index)
    {
        setFocus();
        prev_page_link_->setVisible(is_login_ && index == SUBPAGE_PHONE_CONF_INDEX);
        next_page_link_->setVisible(index != SUBPAGE_PHONE_CONF_INDEX);

        QString next_page_text;
        if (is_login_)
        {
           next_page_text = index == SUBPAGE_PHONE_LOGIN_INDEX ? QT_TRANSLATE_NOOP("login_page","Continue") : QT_TRANSLATE_NOOP("login_page","Start messaging");
        }
        else
        {
           next_page_text = index == SUBPAGE_PHONE_LOGIN_INDEX ? QT_TRANSLATE_NOOP("login_page","Continue") : QT_TRANSLATE_NOOP("attach_page","Continue");
        }

        next_page_link_->setText(next_page_text);
        switch_login_link_->setText(index == SUBPAGE_UIN_LOGIN_INDEX ? QT_TRANSLATE_NOOP("login_page","Login via phone") : QT_TRANSLATE_NOOP("login_page","Login with UIN/Email"));
        switch (index)
        {
        case SUBPAGE_PHONE_LOGIN_INDEX:
            hint_label_->setText(QT_TRANSLATE_NOOP("login_page","Please confirm your country code and enter\nyour phone number"));
            phone_->setFocus();
            break;

        case SUBPAGE_PHONE_CONF_INDEX:
            hint_label_->setText(QT_TRANSLATE_NOOP("login_page","Please enter the code you've just received in your phone"));
            code_edit_->setFocus();
            break;

        case SUBPAGE_UIN_LOGIN_INDEX:
            if (is_login_)
            {
                hint_label_->setText(QT_TRANSLATE_NOOP("login_page","Enter your UIN or Email to get started"));
            }
            else
            {
                hint_label_->setVisible(false);
            }
            uin_login_edit_->setFocus();
            break; 
        }
        login_staked_widget_->setCurrentIndex(index);
    }

    void LoginPage::setPhoneFocusIn()
    {
        phone_widget_->setProperty("EnterPhoneWidgetFocused", true);
        phone_widget_->setProperty("EnterPhoneWidgetError", false);
        phone_widget_->setProperty("EnterPhoneWidget", false);
        phone_widget_->setStyle(QApplication::style());
        emit country(country_code_->text());
    }

    void LoginPage::setPhoneFocusOut()
    {
        phone_widget_->setProperty("EnterPhoneWidgetFocused", false);
        phone_widget_->setProperty("EnterPhoneWidgetError", false);
        phone_widget_->setProperty("EnterPhoneWidget", true);
        phone_widget_->setStyle(QApplication::style());
        emit country(country_code_->text());
    }

    void LoginPage::redrawCountryCode()
    {
        QFontMetrics fm = country_code_->fontMetrics();
        int w = fm.boundingRect(country_code_->text()).width() + 5;

        QRect content = phone_widget_->contentsRect();
        country_code_->resize(w, country_code_->height());
        phone_->resize(content.width() - w, phone_->height());
        country_code_->move(content.x(), content.y());
        phone_->move(content.x() + w, content.y());
    }

    void LoginPage::countrySelected(QString code)
    {
        if (prev_country_code_ == code)
            return;

        if (!prev_country_code_.isEmpty())
        {
            core::stats::event_props_type props;
            props.push_back(std::make_pair("prev_code", prev_country_code_.toStdString()));
            props.push_back(std::make_pair("next_code", code.toStdString()));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_country, props);
        }

        prev_country_code_ = code;
        country_code_->setText(code);
        redrawCountryCode();
        phone_->setFocus();
    }

    void LoginPage::nextPage()
    {
        setFocus();
        clearErrors();
        if (login_staked_widget_->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX)
        {
            entered_phone_->setText(country_code_->text() + phone_->text());
            entered_phone_->adjustSize();
            country_code_->setEnabled(false);
            phone_->setEnabled(false);
            sendCode();
        }
        else if (login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            uin_login_edit_->setEnabled(false);
            uin_password_edit_->setEnabled(false);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("login", uin_login_edit_->text());
            collection.set_value_as_qstring("password", uin_password_edit_->text());
            collection.set_value_as_bool("save_auth_data", keep_logged_->isChecked());
            collection.set_value_as_bool("is_login", is_login_);
            send_seq_ = GetDispatcher()->post_message_to_core("login_by_password", collection.get());
        }
    }

    void LoginPage::prevPage()
    {
        clearErrors();
        initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
    }

    void LoginPage::switchLoginType()
    {
        setFocus();
        clearErrors();
        initLoginSubPage(login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ? SUBPAGE_PHONE_LOGIN_INDEX : SUBPAGE_UIN_LOGIN_INDEX);
        GetDispatcher()->post_stats_to_core(login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX 
            ? core::stats::stats_event_names::reg_page_uin : core::stats::stats_event_names::reg_page_phone);
    }

    void LoginPage::stats_edit_phone()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_phone);
    }

    void LoginPage::stats_resend_sms()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_sms_resend);
    }

    void LoginPage::updateTimer()
    {
        resend_button_->setEnabled(false);
        QString text = remaining_seconds_ ? QT_TRANSLATE_NOOP("login_page","Resend code in ") : QT_TRANSLATE_NOOP("login_page","Resend code");
        if (remaining_seconds_ == 60)
        {
            text += "1:00";
        }
        else if (remaining_seconds_ > 0)
        {
            text += QString(remaining_seconds_ >= 10 ? "0:%1" : "0:0%1").arg(remaining_seconds_);
        }
        else
        {
            resend_button_->setEnabled(true);
        }
        resend_button_->setText(text);

        if (remaining_seconds_)
        {
            --remaining_seconds_;
            timer_->start(1000);
        }
        else
        {
            timer_->stop();
        }
    }

    void LoginPage::sendCode()
    {
        timer_->stop();
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("country", country_code_->text());
        collection.set_value_as_qstring("phone", phone_->text());
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getCurrentLang());
        collection.set_value_as_bool("is_login", is_login_);

        send_seq_ = GetDispatcher()->post_message_to_core("login_get_sms_code", collection.get());
        remaining_seconds_ = 60;
        updateTimer();
    }

    void LoginPage::getSmsResult(int64_t _seq, int result)
    {
        if (_seq != send_seq_)
            return;

        country_code_->setEnabled(true);
        phone_->setEnabled(true);
        setErrorText(result);
        error_label_->setVisible(result);
        if (result == core::le_success)
        {
            clearErrors();
            return initLoginSubPage(SUBPAGE_PHONE_CONF_INDEX);
        }

        phone_widget_->setProperty("EnterPhoneWidgetError", true);
        phone_widget_->setProperty("EnterPhoneWidgetFocused", false);
        phone_widget_->setProperty("EnterPhoneWidget", false);
        phone_widget_->setStyle(QApplication::style());
        phone_->setProperty("PhoneNumberError", true);
        phone_->setStyle(QApplication::style());
        emit country(country_code_->text());
    }

    void LoginPage::updateErrors(int result)
    {
        code_edit_->setEnabled(true);
        uin_login_edit_->setEnabled(true);
        uin_password_edit_->setEnabled(true);
        setErrorText(result);
        error_label_->setVisible(result);

        if (login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            uin_login_edit_->setProperty("UinError", true);
            uin_login_edit_->setProperty("Uin", false);
            uin_login_edit_->setStyle(QApplication::style());
            uin_password_edit_->clear();
            uin_login_edit_->setFocus();
        }
        else
        {
            code_edit_->setProperty("EnteredCodeError", true);
            code_edit_->setProperty("EnteredCode", false);
            code_edit_->setStyle(QApplication::style());
            code_edit_->setFocus();
        }
    }

    void LoginPage::loginResult(int64_t _seq, int result)
    {
        if (_seq != send_seq_)
            return;

        updateErrors(result);
        if (result == 0)
        {
            if (login_staked_widget_->currentIndex() == SUBPAGE_PHONE_CONF_INDEX)
            {
                code_edit_->setText("");
                initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
            }

            GetDispatcher()->post_stats_to_core(login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX 
                ? core::stats::stats_event_names::reg_login_uin
                : core::stats::stats_event_names::reg_login_phone);

            clearErrors();
            emit loggedIn();
        }
    }

    void LoginPage::loginResultAttachUin(int64_t _seq, int result)
    {
        if (_seq != send_seq_)
            return;
        updateErrors(result);
        if (result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::loginResultAttachPhone(int64_t _seq, int result)
    {
        if (_seq != send_seq_)
            return;
        updateErrors(result);
        if (result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::clearErrors()
    {
        error_label_->hide();

        uin_login_edit_->setProperty("UinError", false);
        uin_login_edit_->setProperty("Uin", true);
        uin_login_edit_->setStyle(QApplication::style());

        code_edit_->setProperty("EnteredCodeError", false);
        code_edit_->setProperty("EnteredCode", true);
        code_edit_->setStyle(QApplication::style());

        phone_->setProperty("PhoneNumberError", false);
        phone_->setProperty("PhoneNumberEdit", true);
        phone_->setStyle(QApplication::style());

        emit country(country_code_->text());
    }

    void LoginPage::setErrorText(int result)
    {
        setFocus();
        switch (result)
        {
        case core::le_wrong_login:
            error_label_->setText(login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ? 
                QT_TRANSLATE_NOOP("login_page","Wrong UIN/Email or password. Please try again.")
                : QT_TRANSLATE_NOOP("login_page","You have entered an invalid code. Please try again."));
            GetDispatcher()->post_stats_to_core(login_staked_widget_->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_error_uin 
                : core::stats::stats_event_names::reg_error_code);
            break;
        case core::le_error_validate_phone:
            error_label_->setText(QT_TRANSLATE_NOOP("login_page","Invalid phone number. Please try again."));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_phone);
            break;
        case core::le_success:
            error_label_->setText("");
            break;
        case core::le_attach_error_busy_phone:
            error_label_->setText(QT_TRANSLATE_NOOP("login_page","This phone number is already attached to another account. Please edit phone number and try again."));
            break;
        default:
            error_label_->setText(QT_TRANSLATE_NOOP("login_page","Error occured, try again later"));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_other);
            break;
        }
    }

    void LoginPage::codeEditChanged(QString code)
    {
        if (code.length() == 4)
        {
            setFocus();
            code_edit_->setEnabled(false);
            get_gui_settings()->set_value(settings_keep_logged_in, true);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("phone", phone_->text());
            collection.set_value_as_qstring("sms_code", code);
            collection.set_value_as_bool("save_auth_data", true);
            collection.set_value_as_bool("is_login", is_login_);
            send_seq_ = GetDispatcher()->post_message_to_core("login_by_phone", collection.get());
        }
    }

    void LoginPage::countryCodeChanged(QString text)
    {
        if (!text.isEmpty() && text != "+")
        {
            combobox_->selectItem(text);
            if (!combobox_->containsCode(text))
            {
                country_code_->setText(prev_country_code_);
                if (phone_->text().isEmpty())
                    phone_->setText(text.mid(prev_country_code_.length(), text.length() - prev_country_code_.length()));
                phone_->setFocus();
            }
        }

        prev_country_code_ = text;
    }

    void LoginPage::emptyPhoneRemove()
    {
        country_code_->setFocus();
        QString code = country_code_->text();
        country_code_->setText(code.isEmpty() ? code : code.left(code.length() - 1));
    }
}
