#include "stdafx.h"
#include "MainPage.h"

#include "contact_list/ContactList.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/SearchModel.h"
#include "contact_list/Common.h"

#include "../gui_settings.h"
#include "history_control/MessagesModel.h"
#include "../voip/VideoWindow.h"
#include "../voip/CallPanelMain.h"
#include "../voip/VideoSettings.h"
#include "../voip/IncomingCallWindow.h"
#include "ContactDialog.h"
#include "contact_list/ChatMembersModel.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../../corelib/core_face.h"
#include "../controls/TextEditEx.h"
#include "search_contacts/SearchContactsWidget.h"
#include "contact_list/SelectionContactsForGroupChat.h"
#include "../controls/GeneralDialog.h"
#include "settings/ProfileSettingsWidget.h"
#include "settings/GeneralSettingsWidget.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../my_info.h"
#include "../controls/SemitransparentWindow.h"
#include "../controls/WidgetsNavigator.h"
#include "../controls/TextEmojiWidget.h"
#include "settings/themes/ThemesSettingsWidget.h"
#include "../controls/ContextMenu.h"
#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "contact_list/RecentsModel.h"
#include "../utils/log/log.h"
#include "IntroduceYourself.h"
#include "livechats/LiveChatsHome.h"
#include "GroupChatOperations.h"
#include "livechats/LiveChatProfile.h"
#include "sidebar/Sidebar.h"
#include "contact_list/Common.h"

namespace Ui
{
    MainPage* MainPage::_instance = NULL;

    MainPage* MainPage::instance(QWidget* _parent)
    {
        assert(_instance || _parent);

        if (!_instance)
            _instance = new MainPage(_parent);

        return _instance;
    }

    void MainPage::reset()
    {
        if (_instance)
        {
            delete _instance;
            _instance = 0;
        }
    }

    MainPage::MainPage(QWidget* parent)
        : QWidget(parent)
        , search_widget_(new SearchWidget(false, this))
        , contact_dialog_(new ContactDialog(this))
#ifndef STRIP_VOIP
        , video_window_(nullptr)
#else
        , video_window_(nullptr)
#endif //STRIP_VOIP
        , pages_(new WidgetsNavigator(this))
        , search_contacts_(nullptr)
        , profile_settings_(new ProfileSettingsWidget(this))
        , general_settings_(new GeneralSettingsWidget(this))
        , live_chats_page_(new LiveChatHome(this))
        , themes_settings_(new ThemesSettingsWidget(this))
        , noContactsYetSuggestions_(nullptr)
        , contact_list_widget_(new ContactList(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
        , add_contact_menu_(nullptr)
        , settings_timer_(new QTimer(this))
        , introduceYourselfSuggestions_(nullptr)
        , needShowIntroduceYourself_(false)
        , liveChats_(new LiveChats(this))
        , login_new_user_(false)
        , recv_my_info_(false)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPlaceholder, this, &MainPage::showPlaceholder);

        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("main_page"));
        setStyleSheet(Utils::LoadStyle(":/main_window/main_window.qss", Utils::get_scale_coefficient(), true));
        this->resize(400, 300);
        this->setProperty("Invisible", QVariant(true));
        horizontal_layout_ = new QHBoxLayout(this);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        QMetaObject::connectSlotsByName(this);

        QHBoxLayout* originalLayout = qobject_cast<QHBoxLayout*>(layout());
        QVBoxLayout* contactsLayout = new QVBoxLayout();
        contactsLayout->setContentsMargins(0, 0, 0, 0);
        contactsLayout->setSpacing(0);

        contactsLayout->addWidget(search_widget_);
        contactsLayout->addWidget(contact_list_widget_);
        QSpacerItem* contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        contactsLayout->addSpacerItem(contactsLayoutSpacer);

        pages_layout_ = new QVBoxLayout();
        pages_layout_->setContentsMargins(0, 0, 0, 0);
        pages_layout_->setSpacing(0);
        pages_layout_->addWidget(pages_);
        {
            auto pc = pages_->count();
            pages_->addWidget(contact_dialog_);
            pages_->addWidget(profile_settings_);
            pages_->addWidget(general_settings_);
            pages_->addWidget(live_chats_page_);
            pages_->addWidget(themes_settings_);
            if (!pc)
                pages_->push(contact_dialog_);
        }
        originalLayout->addLayout(contactsLayout);
        originalLayout->addLayout(pages_layout_);
        QSpacerItem* originalLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        originalLayout->addSpacerItem(originalLayoutSpacer);
        originalLayout->setAlignment(Qt::AlignLeft);
        setFocus();

        connect(contact_list_widget_, SIGNAL(itemSelected(QString)), contact_dialog_, SLOT(onContactSelected(QString)), Qt::QueuedConnection);
        connect(contact_dialog_, SIGNAL(sendMessage(QString)), contact_list_widget_, SLOT(onSendMessage(QString)), Qt::QueuedConnection);

        connect(contact_list_widget_, SIGNAL(itemSelected(QString)), this, SLOT(onContactSelected(QString)), Qt::QueuedConnection);
        connect(contact_list_widget_, SIGNAL(addContactClicked()), this, SLOT(onAddContactClicked()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsShow(QString)), this, SLOT(onProfileSettingsShow(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsShow(bool,QString)), this, SLOT(onThemesSettingsShow(bool,QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsShow(int)), this, SLOT(onGeneralSettingsShow(int)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(liveChatsShow()), this, SLOT(onLiveChatsShow()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachPhoneBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachUinBack()), pages_, SLOT(pop()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(makeSearchWidgetVisible(bool)), search_widget_, SLOT(setVisible(bool)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(popPagesToRoot()), this, SLOT(popPagesToRoot()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsDoMessage(QString)), contact_list_widget_, SLOT(select(QString)), Qt::QueuedConnection);

        connect(search_widget_, SIGNAL(searchBegin()), this, SLOT(searchBegin()), Qt::QueuedConnection);
        connect(search_widget_, SIGNAL(searchEnd()), this, SLOT(searchEnd()), Qt::QueuedConnection);
        connect(search_widget_, SIGNAL(search(QString)), Logic::GetSearchModel(), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(search_widget_, SIGNAL(enterPressed()), contact_list_widget_, SLOT(searchResult()), Qt::QueuedConnection);
        connect(search_widget_, SIGNAL(upPressed()), contact_list_widget_, SLOT(searchUpPressed()), Qt::QueuedConnection);
        connect(search_widget_, SIGNAL(downPressed()), contact_list_widget_, SLOT(searchDownPressed()), Qt::QueuedConnection);
        connect(contact_list_widget_, SIGNAL(searchEnd()), search_widget_, SLOT(searchCompleted()), Qt::QueuedConnection);

        connect(Logic::GetContactListModel(), SIGNAL(selectedContactChanged(QString)), Logic::GetMessagesModel(), SLOT(contactChanged(QString)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipShowVideoWindow(bool)), this, SLOT(onVoipShowVideoWindow(bool)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncoming(const std::string&, const std::string&)), this, SLOT(onVoipCallIncoming(const std::string&, const std::string&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);

        search_widget_->setVisible(!contact_list_widget_->shouldHideSearch());

        post_stats_with_settings();
        QObject::connect(settings_timer_, SIGNAL(timeout()), this, SLOT(post_stats_with_settings()));

        settings_timer_->start(Ui::period_for_stats_settings_ms);
        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &MainPage::myInfo, Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::login_new_user, this, &MainPage::loginNewUser, Qt::DirectConnection);
        
        add_contact_menu_ = new FlatMenu(search_widget_->searchEditIcon());
        Utils::ApplyStyle(add_contact_menu_,
                          "QMenu { background-color: #f2f2f2; border:1px solid #cccccc; } "
                          "QMenu::item { padding-left:40dip; background-color:transparent; color:black; padding-top:6dip; padding-bottom:6dip; padding-right:12dip; } "
                          "QMenu::item:selected { background-color:#e2e2e2; } "
                          "QMenu::icon { padding-left:22dip; }"
                          );
        add_contact_menu_->addAction(QIcon(Utils::parse_image_name(":/resources/dialog_newchat_100.png")), QT_TRANSLATE_NOOP("contact_list", "New chat"), contact_list_widget_, SLOT(allClicked()));
        add_contact_menu_->addAction(QIcon(Utils::parse_image_name(":/resources/dialog_newgroup_100.png")), QT_TRANSLATE_NOOP("contact_list", "Create Groupchat"), this, SLOT(createGroupChat()));
        add_contact_menu_->setExpandDirection(Qt::AlignLeft);
        add_contact_menu_->stickToIcon();
        Utils::ApplyStyle(search_widget_->searchEditIcon(), "QPushButton::menu-indicator { image:none; } QPushButton:pressed { background-color:transparent; }");
        search_widget_->searchEditIcon()->setMenu(add_contact_menu_);
    }

    MainPage::~MainPage()
    {
    }

    void MainPage::resizeEvent(QResizeEvent*)
    {
        auto new_cl_width = ::ContactList::ItemWidth(false, false, false).px();
        contact_list_widget_->setFixedWidth(new_cl_width);
        search_widget_->setFixedWidth(new_cl_width);

        Sidebar* sidebar = contact_dialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            sidebar->setFixedWidth(getContactDialogWidth(width()));
    }

    int MainPage::getContactDialogWidth(int _main_page_width)
    {
        return _main_page_width - ::ContactList::ItemWidth(false, false, false).px();
    }

    void MainPage::setSearchFocus()
    {
        if (pages_->currentWidget() != contact_dialog_)
        {
            pages_->push(contact_dialog_);
            contact_list_widget_->changeTab(RECENTS);
        }
            
        search_widget_->setFocus();
    }

    void MainPage::onProfileSettingsShow(QString uin)
    {
        if (uin.isEmpty())
        {
            contact_dialog_->takeSidebar();
            Sidebar* sidebar = contact_dialog_->getSidebar();
            sidebar->preparePage(uin, profile_page);
            sidebar->setSidebarWidth(Utils::scale_value(428));
            pages_->addWidget(sidebar);
            pages_->push(sidebar);
            sidebar->setFixedWidth(width() - ::ContactList::ItemWidth(false, false, false).px());
        }
        else
        {
            showSidebar(uin, profile_page);
        }
    }

    void MainPage::onLiveChatsShow()
    {
        pages_->push(live_chats_page_);
    }

    void MainPage::raiseVideoWindow() {
        if (!video_window_) {
            video_window_ = new(std::nothrow) VideoWindow();
        }

        if (!!video_window_ && !video_window_->isHidden()) {
            video_window_->showNormal();
            video_window_->activateWindow();
        }
    }

    void MainPage::onGeneralSettingsShow(int type)
    {
        pages_->push(general_settings_);
        general_settings_->setType(type);
    }

    void MainPage::onThemesSettingsShow(bool _show_back_button, QString _aimId)
    {
        ThemesSettingsWidget::initWhenNeeded(themes_settings_)->setBackButton(_show_back_button);
        ThemesSettingsWidget::initWhenNeeded(themes_settings_)->setTargetContact(_aimId);
        pages_->push(themes_settings_);
    }

    void MainPage::clearSearchMembers()
    {
        contact_list_widget_->update();
    }

    void MainPage::cancelSelection()
    {
        assert(contact_dialog_);
        contact_dialog_->cancelSelection();
    }

    void MainPage::createGroupChat()
    {
        QStringList empty_list;
        auto oldModel = Logic::GetChatMembersModel();
        Logic::SetChatMembersModel(NULL);
        Ui::createGroupChat(empty_list);
        Logic::SetChatMembersModel(oldModel);
        
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_from_create_button);
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        assert(contact_dialog_);
        return contact_dialog_;
    }

    HistoryControlPage* MainPage::getHistoryPage(const QString& aimId) const
    {
        return contact_dialog_->getHistoryPage(aimId);
    }

    void MainPage::insertTopWidget(const QString& aimId, QWidget* widget)
    {
        contact_dialog_->insertTopWidget(aimId, widget);
    }

    void MainPage::removeTopWidget(const QString& aimId)
    {
        contact_dialog_->removeTopWidget(aimId);
    }

    void MainPage::showSidebar(const QString& aimId, int page)
    {
        if (search_contacts_ && pages_->currentWidget() == search_contacts_)
        {
            contact_dialog_->takeSidebar();
            Sidebar* sidebar = contact_dialog_->getSidebar();
            sidebar->preparePage(aimId, (SidebarPages)page);
            sidebar->setSidebarWidth(Utils::scale_value(428));
            sidebar->setFixedWidth(pages_->width());
            pages_->addWidget(sidebar);
            pages_->push(sidebar);
        }
        else
        {
            contact_dialog_->showSidebar(aimId, page);
        }
    }
    
    bool MainPage::isSidebarVisible() const
    {
        return contact_dialog_->isSidebarVisible();
    }

    void MainPage::setSidebarVisible(bool show)
    {
        Sidebar* sidebar = contact_dialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
        {
            pages_->pop();
            pages_->removeWidget(sidebar);
            contact_list_widget_->clearSettingsSelection();
            if (!show)
                contact_dialog_->setSidebarVisible(show);
        }
        else
        {
            contact_dialog_->setSidebarVisible(show);
        }
    }

    bool MainPage::isContactDialog() const
    {
        if (pages_ == 0)
            return false;
        
        return pages_->currentWidget() == contact_dialog_;
    }

    ProfileSettingsWidget* MainPage::getProfileSettings() const
    {
        return profile_settings_;
    }
    
    void MainPage::onContactSelected(QString _contact)
    {
        pages_->poproot();

        if (search_contacts_)
        {
            pages_->removeWidget(search_contacts_);
            delete search_contacts_;
            search_contacts_ = nullptr;
        }

        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
    }

    void MainPage::onAddContactClicked()
    {
        if (!search_contacts_)
        {
            search_contacts_ = new SearchContactsWidget(this);
            pages_->addWidget(search_contacts_);
        }
        pages_->push(search_contacts_);
        search_contacts_->on_focus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::search_open_page);
    }

    void MainPage::searchBegin()
    {
        contact_list_widget_->setSearchMode(true);
    }

    void MainPage::searchEnd()
    {
        contact_list_widget_->setSearchMode(false);
    }

    void MainPage::onVoipCallIncoming(const std::string& account, const std::string& contact) {
        assert(!account.empty());
        assert(!contact.empty());

        if (!account.empty() && !contact.empty()) {
            std::string call_id = account + "#" + contact;

            auto it = incoming_call_windows_.find(call_id);
            if (incoming_call_windows_.end() == it || !it->second) {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(account, contact));
                assert(!!window);

                if (!!window) {
                    window->showFrame();
                    incoming_call_windows_[call_id] = window;
                }
            } else {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }
        }
    }

    void MainPage::_destroy_incoming_call_window(const std::string& account, const std::string& contact) {
        assert(!account.empty());
        assert(!contact.empty());

        if (!account.empty() && !contact.empty()) {
            std::string call_id = account + "#" + contact;
            auto it = incoming_call_windows_.find(call_id);
            if (incoming_call_windows_.end() != it) {
                auto window = it->second;
                assert(!!window);

                if (!!window) {
                    window->hideFrame();
                }
            }
        }
    }

    void MainPage::onVoipCallIncomingAccepted(const voip_manager::ContactEx& contact_ex) {
        _destroy_incoming_call_window(contact_ex.contact.account, contact_ex.contact.contact);
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex) {
        _destroy_incoming_call_window(contact_ex.contact.account, contact_ex.contact.contact);
    }

    void MainPage::recentsTabActivate(bool selectUnread)
    {
        assert(!!contact_list_widget_);
        if (contact_list_widget_) {
            contact_list_widget_->recentsClicked();

            if (selectUnread)
            {
                QString aimId = Logic::GetRecentsModel()->nextUnreadAimId();
                if (aimId.length() > 0)
                {
                    contact_list_widget_->select(aimId);
                }
            }
        }
    }

    void MainPage::selectRecentChat(QString aimId)
    {
        assert(!!contact_list_widget_);
        if (contact_list_widget_) {
            if (aimId.length() > 0)
            {
                contact_list_widget_->select(aimId);
            }
        }
    }

    void MainPage::contactListActivate(bool addContact)
    {
        assert(!!contact_list_widget_);
        if (contact_list_widget_) {
            contact_list_widget_->allClicked();

            if (addContact)
            {
                contact_list_widget_->addContactClicked();
            }
        }
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType item) {
        assert(!!contact_list_widget_);
        if (contact_list_widget_) {
            contact_list_widget_->settingsClicked();

            switch (item)
            {
            case Utils::CommonSettingsType::CommonSettingsType_General:
            case Utils::CommonSettingsType::CommonSettingsType_VoiceVideo:
            case Utils::CommonSettingsType::CommonSettingsType_Notifications:
            case Utils::CommonSettingsType::CommonSettingsType_Themes:
            case Utils::CommonSettingsType::CommonSettingsType_About:
            case Utils::CommonSettingsType::CommonSettingsType_ContactUs:
            case Utils::CommonSettingsType::CommonSettingsType_AttachPhone:
            case Utils::CommonSettingsType::CommonSettingsType_AttachUin:
                Utils::InterConnector::instance().generalSettingsShow((int)item);
                break;
            case Utils::CommonSettingsType::CommonSettingsType_Profile:
                Utils::InterConnector::instance().profileSettingsShow("");
                break;
            default:
                break;
            }
        }
    }

    void MainPage::onVoipShowVideoWindow(bool enabled) {
        if (!video_window_) {
            video_window_ = new(std::nothrow) VideoWindow();
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

		if (!!video_window_) {
			if (enabled) {
				video_window_->showFrame();
			} else {
				video_window_->hideFrame();

                bool wndMinimized = false;
                bool wndHiden = false;
                if (QWidget* parentWnd = window()) {
                    wndHiden = !parentWnd->isVisible();
                    wndMinimized = parentWnd->isMinimized();
                }

                if (!Utils::foregroundWndIsFullscreened() && !wndMinimized && !wndHiden) {
                    raise();
                }
            }
        }
    }

    void MainPage::hideInput()
    {
        contact_dialog_->hideInput();
    }

    QWidget* MainPage::showNoContactsYetSuggestions(QWidget *parent, std::function<void()> addNewContactsRoutine)
    {
        if (!noContactsYetSuggestions_)
        {
            noContactsYetSuggestions_ = new QWidget(parent);
            noContactsYetSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            noContactsYetSuggestions_->setStyleSheet("background-color: white;");
            {
                auto l = new QVBoxLayout(noContactsYetSuggestions_);
                l->setContentsMargins(0, 0, 0, 0);
                l->setSpacing(0);
                l->setAlignment(Qt::AlignCenter);
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, 0, 0, 0);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new QWidget(p);
                        w->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        w->setStyleSheet("image: url(:/resources/main_window/content_logo_200.png);");
                        w->setFixedSize(Utils::scale_value(64), Utils::scale_value(64));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, 0, 0, 0);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(44));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "Install ICQ on mobile"));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, 0, 0, 0);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(30));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "to synchronize your contacts"));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, Utils::scale_value(28), 0, 0);
                    pl->setSpacing(Utils::scale_value(8));
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto as = new QPushButton(p);
                        as->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        as->setFlat(true);
                        auto asu = QString(":/resources/placeholders/content_badge_appstore_%1_200.png").arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());
                        auto asi = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); }").arg(asu).arg(asu);
                        as->setStyleSheet(asi);
                        as->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        as->setCursor(Qt::PointingHandCursor);
                        parent->connect(as, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(QUrl("https://app.appsflyer.com/id302707408?pid=icq_win"));
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_ios);
                        });
                        pl->addWidget(as);

                        auto gp = new QPushButton(p);
                        gp->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        gp->setFlat(true);
                        auto gpu = QString(":/resources/placeholders/content_badge_gplay_%1_200.png").arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());
                        auto gpi = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); }").arg(gpu).arg(gpu);
                        gp->setStyleSheet(gpi);
                        gp->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        gp->setCursor(Qt::PointingHandCursor);
                        parent->connect(gp, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(QUrl("https://app.appsflyer.com/com.icq.mobile.client?pid=icq_win"));
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_android);
                        });
                        pl->addWidget(gp);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, 0, 0, 0);
                    pl->setSpacing(0);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w1 = new Ui::TextEmojiWidget(p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#282828"), Utils::scale_value(46));
                        w1->setSizePolicy(QSizePolicy::Policy::Preferred, w1->sizePolicy().verticalPolicy());
                        w1->setText(QT_TRANSLATE_NOOP("placeholders", "or "));
                        pl->addWidget(w1);

                        auto w2 = new Ui::TextEmojiWidget(p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#579e1c"), Utils::scale_value(46));
                        w2->setSizePolicy(QSizePolicy::Policy::Preferred, w2->sizePolicy().verticalPolicy());
                        w2->setText(QT_TRANSLATE_NOOP("placeholders", "find friends"));
                        w2->setCursor(Qt::PointingHandCursor);
                        parent->connect(w2, &Ui::TextEmojiWidget::clicked, addNewContactsRoutine);
                        pl->addWidget(w2);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, 0, 0, 0);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor("#696969"), Utils::scale_value(20));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "by phone number or UIN"));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }

            }
        }
        return noContactsYetSuggestions_;
    }

    void MainPage::showPlaceholder(Utils::PlaceholdersType _PlaceholdersType)
    {
        switch(_PlaceholdersType)
        {
        case Utils::PlaceholdersType::PlaceholdersType_HideFindFriend:
            if (noContactsYetSuggestions_)
            {
                noContactsYetSuggestions_->setHidden(true);
                pages_->removeWidget(noContactsYetSuggestions_);
            }
            pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_FindFriend:
            pages_->insertWidget(1, showNoContactsYetSuggestions(pages_, [this]()
            {
                onAddContactClicked();
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_find_friends);
            }));
            pages_->push(contact_list_widget_);
            pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself:
            needShowIntroduceYourself_ = true;
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself:
            if (needShowIntroduceYourself_)
            {
                if (introduceYourselfSuggestions_)
                {
                    introduceYourselfSuggestions_->setHidden(true);
                    pages_->removeWidget(introduceYourselfSuggestions_);
                }

                pages_->poproot();

                needShowIntroduceYourself_ = false;
            }
            break;

        case Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself:
            if (!needShowIntroduceYourself_)
                break;

            pages_->insertWidget(0, showIntroduceYourselfSuggestions(pages_, [this]()
            {
                onAddContactClicked();
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_find_friends);
            }));
            pages_->push(contact_list_widget_);
            pages_->poproot();
            break;
                
        default:
            break;
        }
    }

    QWidget* MainPage::showIntroduceYourselfSuggestions(QWidget *parent, std::function<void()> addNewContactsRoutine)
    {
        if (!introduceYourselfSuggestions_)
        {
            introduceYourselfSuggestions_ = new IntroduceYourself(MyInfo()->aimId(), MyInfo()->friendlyName(), parent);
            introduceYourselfSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            introduceYourselfSuggestions_->setStyleSheet("border: 20px solid red;");
        }
        return introduceYourselfSuggestions_;
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::get_stats_sender()->trySendStats();
    }

    void MainPage::myInfo()
    {
        if (login_new_user_ && MyInfo()->friendlyName().isEmpty() && !get_gui_settings()->get_value(get_account_setting_name(settings_skip_intro_yourself).c_str(), false))
        {
            if (!recv_my_info_)
            {
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself);
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself);
            }
        }
        else
        {
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        recv_my_info_ = true;
    }

    void MainPage::openCreatedGroupChat()
    {
        auto connect_id = connect(GetDispatcher(), SIGNAL(openChat(QString)), contact_list_widget_, SLOT(select(QString)));
        QTimer::singleShot(2000, [this, connect_id] { disconnect(connect_id); } );
    }

    void MainPage::loginNewUser()
    {
        login_new_user_ = true;
    }

    void MainPage::popPagesToRoot()
    {
        Sidebar* sidebar = contact_dialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            setSidebarVisible(false);

        pages_->poproot();
    }
}
