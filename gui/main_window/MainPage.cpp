#include "stdafx.h"
#include "MainPage.h"

#include "ContactDialog.h"
#include "GroupChatOperations.h"
#include "IntroduceYourself.h"
#include "livechats/LiveChatsHome.h"
#include "livechats/LiveChatProfile.h"
#include "contact_list/ChatMembersModel.h"
#include "contact_list/Common.h"
#include "contact_list/ContactList.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/SearchModelDLG.h"
#include "contact_list/UnknownsModel.h"
#include "history_control/MessagesModel.h"
#include "search_contacts/SearchContactsWidget.h"
#include "settings/GeneralSettingsWidget.h"
#include "sidebar/Sidebar.h"
#include "settings/themes/ThemesSettingsWidget.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../my_info.h"
#include "../controls/BackButton.h"
#include "../controls/CommonStyle.h"
#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/WidgetsNavigator.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../utils/log/log.h"
#include "../voip/IncomingCallWindow.h"
#include "../voip/VideoWindow.h"
#include "MainWindow.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const int balloon_size = 20;
    const int unreads_padding = 6;
    const int unreads_minimum_extent = balloon_size;
    const auto unreadsFont = ContactList::dif(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Semibold, 13);
}

namespace Ui
{
    CounterButton::CounterButton(QWidget *parent): QPushButton(parent), painter_(new QPainter(this))
    {
        painter_->setRenderHint(QPainter::Antialiasing);
        painter_->setRenderHint(QPainter::SmoothPixmapTransform);
        painter_->setRenderHint(QPainter::TextAntialiasing);
    }

    CounterButton::~CounterButton()
    {
        //
    }

    void CounterButton::paintEvent(QPaintEvent* _event)
    {
        QPushButton::paintEvent(_event);

        const auto unreads = (Logic::getUnknownsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads());
        if (unreads > 0)
        {
            const auto text = ((unreads > 99) ? QString("99+") : QVariant(unreads).toString());

            static QFontMetrics m(unreadsFont.font());

            const auto unreadsRect = m.tightBoundingRect(text);
            const auto firstChar = text[0];
            const auto lastChar = text[text.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

            auto balloonWidth = unreadsWidth;
            const auto isLongText = (text.length() > 1);
            if (isLongText)
            {
                balloonWidth += Utils::scale_value(unreads_padding * 2);
            }
            else
            {
                balloonWidth = Utils::scale_value(unreads_minimum_extent);
            }

            if (painter_->begin(this))
            {
                painter_->setPen(QColor("#579e1c"));
                painter_->setRenderHint(QPainter::Antialiasing);
                const auto radius = Utils::scale_value(balloon_size / 2);
                painter_->setBrush(QColor("#579e1c"));
                const int x = (width() - balloonWidth - Utils::scale_value(14));
                const int y = ((height() - Utils::scale_value(balloon_size)) / 2);
                painter_->drawRoundedRect(x, y, balloonWidth, Utils::scale_value(balloon_size), radius, radius);

                painter_->setFont(unreadsFont.font());
                painter_->setPen(Qt::white);
                const auto textX = (x + ((balloonWidth - unreadsWidth) / 2));
                painter_->drawText(textX, (height() + unreadsRect.height()) / 2, text);
                painter_->end();
            }
        }
    }

    UnknownsHeader::UnknownsHeader(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    UnknownsHeader::~UnknownsHeader()
    {
    }

    void UnknownsHeader::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent(_event);

        QPainter painter(this);
        painter.setPen(QPen(QColor("#dadada"), Utils::fscale_value(1.5)));
        painter.fillRect(rect(), CommonStyle::getFrameTransparency());
        QLineF line(geometry().width()-.5, geometry().y(), geometry().width()-.5, geometry().height());
        painter.drawLine(line);
    }

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

    MainPage::MainPage(QWidget* _parent)
        : QWidget(_parent)
        , unknownsHeader_(nullptr)
        , searchWidget_(new SearchWidget(false, this))
        , contactDialog_(new ContactDialog(this))
        , videoWindow_(nullptr)
        , pages_(new WidgetsNavigator(this))
        , searchContacts_(nullptr)
        , generalSettings_(new GeneralSettingsWidget(this))
        , liveChatsPage_(new LiveChatHome(this))
        , themesSettings_(new ThemesSettingsWidget(this))
        , noContactsYetSuggestions_(nullptr)
        , contactListWidget_(new ContactList(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
        , addContactMenu_(nullptr)
        , settingsTimer_(new QTimer(this))
        , introduceYourselfSuggestions_(nullptr)
        , needShowIntroduceYourself_(false)
        , liveChats_(new LiveChats(this))
        , recvMyInfo_(false)
        , animCLWidth_(new QPropertyAnimation(this, "clWidth"))
        , clSpacer_(new QWidget())
        , clHostLayout_(new QHBoxLayout())
        , leftPanelState_(LeftPanelState::spreaded)
        , NeedShowUnknownsHeader_(false)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPlaceholder, this, &MainPage::showPlaceholder);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::liveChatSelected, this, &MainPage::liveChatSelected, Qt::QueuedConnection);

        setStyleSheet(Utils::LoadStyle(":/main_window/main_window.qss"));
        this->setProperty("Invisible", QVariant(true));
        horizontalLayout_ = new QHBoxLayout(this);
        horizontalLayout_->setSpacing(0);
        horizontalLayout_->setContentsMargins(0, 0, 0, 0);
        QMetaObject::connectSlotsByName(this);

        originalLayout = qobject_cast<QHBoxLayout*>(layout());

        contactsWidget_ = new QWidget();
        contactsLayout = new QVBoxLayout(contactsWidget_);
        contactsLayout->setContentsMargins(0, 0, 0, 0);
        contactsLayout->setSpacing(0);

        {
            backButtonHost_ = new QWidget();
            backButtonHost_->setStyleSheet("background-color: white; border-style: solid; border-right-width: 1px; border-color: #dadada;");
            auto backButtonLayout = new QHBoxLayout(backButtonHost_);
            backButtonLayout->setContentsMargins(0, Utils::scale_value(21), 0, 0);
            backButtonLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(16), 0, QSizePolicy::Fixed));
            backButton_ = new BackButton(this);
            backButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            backButton_->setFlat(true);
            backButton_->setFocusPolicy(Qt::NoFocus);
            backButton_->setCursor(Qt::PointingHandCursor);
            connect(backButton_, &QPushButton::clicked, this, &MainPage::hideRecentsPopup);
            backButtonLayout->addWidget(backButton_);
            backButtonLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(16 - 6 /* backbutton spacer */), 0, QSizePolicy::Fixed));

            auto searchLabel = new TextEmojiWidget(backButtonHost_, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(24), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(24));
            searchLabel->setContentsMargins(0, 0, 0, 0);
            searchLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            searchLabel->setAutoFillBackground(false);
            searchLabel->setText(QT_TRANSLATE_NOOP("contact_list", "Search"));
            searchLabel->setStyleSheet("border-style: none;");

            backButtonLayout->addWidget(searchLabel);
            backButtonLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(16), 0, QSizePolicy::Maximum));

            contactsLayout->addWidget(backButtonHost_);
        }

        contactsLayout->addWidget(searchWidget_);
        {
            BackButton *back = nullptr;

            unknownsHeader_ = new UnknownsHeader(this);
            unknownsHeader_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
            unknownsHeader_->setVisible(false);
            {
                auto layout = new QHBoxLayout(unknownsHeader_);
                layout->setContentsMargins(::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).avatarX().px(), Utils::scale_value(20), 0, Utils::scale_value(8));
                layout->setSpacing(0);
                layout->setAlignment(Qt::AlignCenter);
                back = new BackButton(this);
                back->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                back->setFlat(true);
                back->setFocusPolicy(Qt::NoFocus);
                back->setCursor(Qt::PointingHandCursor);
                connect(backButton_, &QPushButton::clicked, this, &MainPage::hideRecentsPopup);
                layout->addWidget(back);

                layout->addSpacerItem(new QSpacerItem(Utils::scale_value(16 - 6 /* backbutton spacer */), 0, QSizePolicy::Fixed));

                unknownBackButtonLabel_ = new TextEmojiWidget(backButtonHost_, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(18), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(18));
                unknownBackButtonLabel_->setContentsMargins(0, 0, 0, 0);
                unknownBackButtonLabel_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                unknownBackButtonLabel_->setAutoFillBackground(false);
                unknownBackButtonLabel_->setText(QT_TRANSLATE_NOOP("contact_list", "Unknown contacts"));
                unknownBackButtonLabel_->setStyleSheet("border-style: none; background-color: transparent; ");

                layout->addWidget(unknownBackButtonLabel_);

                QSpacerItem* contactsLayoutSpacer = new QSpacerItem(1, 0, QSizePolicy::Expanding);
                layout->addSpacerItem(contactsLayoutSpacer);
            }
            Utils::ApplyStyle(unknownsHeader_, "background-color: rgba(255, 255, 255, 95%);");
            contactsLayout->addWidget(unknownsHeader_);
            connect(back, &QPushButton::clicked, this, [=]()
            {
                emit Utils::InterConnector::instance().unknownsGoBack();
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &MainPage::changeCLHeadToUnknownSlot);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, &MainPage::changeCLHeadToSearchSlot);
        }
        contactsLayout->addWidget(contactListWidget_);
        QSpacerItem* contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        contactsLayout->addSpacerItem(contactsLayoutSpacer);

        pagesLayout_ = new QVBoxLayout();
        pagesLayout_->setContentsMargins(0, 0, 0, 0);
        pagesLayout_->setSpacing(0);
        pagesLayout_->addWidget(pages_);

        {
            auto pc = pages_->count();
            pages_->addWidget(contactDialog_);
            pages_->addWidget(generalSettings_);
            pages_->addWidget(liveChatsPage_);
            pages_->addWidget(themesSettings_);
            if (!pc)
                pages_->push(contactDialog_);
        }
        originalLayout->addWidget(clSpacer_);

        clHostLayout_->addWidget(contactsWidget_);
        originalLayout->addLayout(clHostLayout_);
        originalLayout->addLayout(pagesLayout_);

        QSpacerItem* originalLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        originalLayout->addSpacerItem(originalLayoutSpacer);
        originalLayout->setAlignment(Qt::AlignLeft);
        setFocus();

        connect(contactListWidget_, &ContactList::itemSelected, contactDialog_, &ContactDialog::onContactSelected, Qt::QueuedConnection);
        connect(contactDialog_, SIGNAL(sendMessage(QString)), contactListWidget_, SLOT(onSendMessage(QString)), Qt::QueuedConnection);

        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::onContactSelected, Qt::QueuedConnection);
        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::hideRecentsPopup, Qt::QueuedConnection);
        connect(contactListWidget_, &ContactList::addContactClicked, this, &MainPage::onAddContactClicked, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsShow(QString)), this, SLOT(onProfileSettingsShow(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsShow(bool,QString)), this, SLOT(onThemesSettingsShow(bool,QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsShow(int)), this, SLOT(onGeneralSettingsShow(int)), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachPhoneBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachUinBack()), pages_, SLOT(pop()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(makeSearchWidgetVisible(bool)), searchWidget_, SLOT(setVisible(bool)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(popPagesToRoot()), this, SLOT(popPagesToRoot()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsDoMessage(QString)), contactListWidget_, SLOT(select(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, this, &MainPage::startSearhInDialog, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setSearchFocus, this, &MainPage::setSearchFocus, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchEnd, this, &MainPage::searchEnd, Qt::QueuedConnection);

        connect(searchWidget_, SIGNAL(searchBegin()), this, SLOT(searchBegin()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchEnd()), this, SLOT(searchEnd()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(inputEmpty()), this, SLOT(searchInputClear()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::getSearchModelDLG(), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(enterPressed()), contactListWidget_, SLOT(searchResult()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(upPressed()), contactListWidget_, SLOT(searchUpPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(downPressed()), contactListWidget_, SLOT(searchDownPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchIconClicked()), this, SLOT(spreadCL()), Qt::QueuedConnection);
        connect(contactListWidget_, SIGNAL(searchEnd()), searchWidget_, SLOT(searchCompleted()), Qt::QueuedConnection);

        connect(
            &Utils::InterConnector::instance(),
            &Utils::InterConnector::historyControlReady,
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::contactChanged,
            Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipShowVideoWindow(bool)), this, SLOT(onVoipShowVideoWindow(bool)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncoming(const std::string&, const std::string&)), this, SLOT(onVoipCallIncoming(const std::string&, const std::string&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);

        searchWidget_->setVisible(!contactListWidget_->shouldHideSearch());

        QTimer::singleShot(Ui::period_for_start_stats_settings_ms, this, SLOT(post_stats_with_settings()));
        QObject::connect(settingsTimer_, SIGNAL(timeout()), this, SLOT(post_stats_with_settings()));
        settingsTimer_->start(Ui::period_for_stats_settings_ms);

        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &MainPage::myInfo, Qt::UniqueConnection);

        addContactMenu_ = new FlatMenu(searchWidget_->searchEditIcon());
        addContactMenu_->addAction(QIcon(Utils::parse_image_name(":/resources/dialog_newchat_100.png")), QT_TRANSLATE_NOOP("contact_list", "New chat"), contactListWidget_, SLOT(allClicked()));
        addContactMenu_->addAction(QIcon(Utils::parse_image_name(":/resources/dialog_newgroup_100.png")), QT_TRANSLATE_NOOP("contact_list", "Create groupchat"), this, SLOT(createGroupChat()));
        addContactMenu_->setExpandDirection(Qt::AlignLeft);
        addContactMenu_->stickToIcon();
        Utils::ApplyStyle(searchWidget_->searchEditIcon(), "QPushButton::menu-indicator { image:none; } QPushButton:pressed { background-color:transparent; }");
        searchWidget_->searchEditIcon()->setMenu(addContactMenu_);

        connect(animCLWidth_, &QAbstractAnimation::finished, this, &MainPage::animCLWidthFinished);
        connect(contactDialog_, &ContactDialog::clicked, this, &MainPage::hideRecentsPopup);
        connect(searchWidget_, &SearchWidget::activeChanged, this, &MainPage::searchActivityChanged);
    }

    MainPage::~MainPage()
    {
    }

    void MainPage::setCLWidth(int _val)
    {
        contactsWidget_->setFixedWidth(_val);
        contactListWidget_->setItemWidth(_val);
        contactListWidget_->setFixedWidth(_val);
        searchWidget_->setFixedWidth(_val);
        unknownsHeader_->setFixedWidth(_val);
    }

    int MainPage::getCLWidth() const
    {
        return contactListWidget_->width();
    }

    void MainPage::animateVisibilityCL(int _newWidth, bool _withAnimation)
    {
        int startValue = getCLWidth();
        int endValue = _newWidth;

        int duration = _withAnimation ? 0 : 0;

        animCLWidth_->stop();
        animCLWidth_->setDuration(duration);
        animCLWidth_->setStartValue(startValue);
        animCLWidth_->setEndValue(endValue);
        animCLWidth_->setEasingCurve(QEasingCurve::OutExpo);
        animCLWidth_->start();
    }

    void MainPage::resizeEvent(QResizeEvent*)
    {
        static auto is_inited = false;

        auto cl_width = ::ContactList::ItemWidth(false, false, false, leftPanelState_ == LeftPanelState::picture_only).px();
        if (leftPanelState_ == LeftPanelState::spreaded
            || (::ContactList::IsPictureOnlyView() != (leftPanelState_ == LeftPanelState::picture_only)))
        {
            setLeftPanelState(::ContactList::IsPictureOnlyView() ? LeftPanelState::picture_only : LeftPanelState::normal, is_inited);
            cl_width = ::ContactList::ItemWidth(false, false, false, leftPanelState_ == LeftPanelState::picture_only).px();
        }
        else
        {
            animateVisibilityCL(cl_width, is_inited);
        }

        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            sidebar->setFixedWidth(width() - cl_width);
        
        is_inited = true;
    }

    int MainPage::getContactDialogWidth(int _mainPageWidth)
    {
        return _mainPageWidth - ::ContactList::ItemWidth(false, false, false).px();
    }

    void MainPage::setSearchFocus()
    {
        if (pages_->currentWidget() != contactDialog_)
        {
            pages_->push(contactDialog_);
            contactListWidget_->changeTab(RECENTS);
        }

        searchWidget_->setFocus();
        spreadCL();
    }

    void MainPage::onProfileSettingsShow(QString _uin)
    {
        if (!_uin.isEmpty())
        {
            showSidebar(_uin, profile_page);
            return;
        }

        contactDialog_->takeSidebar();
        Sidebar* sidebar = contactDialog_->getSidebar();
        sidebar->preparePage(_uin, profile_page);
        sidebar->setSidebarWidth(Utils::scale_value(428));
        pages_->addWidget(sidebar);
        pages_->push(sidebar);
        sidebar->setFixedWidth(width() - ::ContactList::ItemWidth(false, false, false, leftPanelState_ == LeftPanelState::picture_only).px());
    }

    void MainPage::raiseVideoWindow()
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
        }

        if (!!videoWindow_ && !videoWindow_->isHidden())
        {
            videoWindow_->showNormal();
            videoWindow_->activateWindow();
        }
    }

    void MainPage::nextChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
            Logic::getContactListModel()->select(Logic::getRecentsModel()->nextAimId(Logic::getContactListModel()->selectedContact()), -1);
        else if (contactListWidget_->currentTab() == ALL)
            Logic::getContactListModel()->next();
    }

    void MainPage::prevChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
            Logic::getContactListModel()->select(Logic::getRecentsModel()->prevAimId(Logic::getContactListModel()->selectedContact()), -1);
        else if (contactListWidget_->currentTab() == ALL)
            Logic::getContactListModel()->prev();
    }

    void MainPage::leftTab()
    {
        int tab = contactListWidget_->currentTab();
        if (tab != RECENTS)
        {
            if (--tab == LIVE_CHATS)
                contactListWidget_->liveChatsClicked();
            else
                contactListWidget_->changeTab((CurrentTab)(tab));
        }
    }

    void MainPage::rightTab()
    {
        int tab = contactListWidget_->currentTab();
        if (tab != SETTINGS && tab != SEARCH)
        {
            if (++tab == LIVE_CHATS)
                contactListWidget_->liveChatsClicked();
            else
                contactListWidget_->changeTab((CurrentTab)(tab));
        }
    }

    void MainPage::onGeneralSettingsShow(int _type)
    {
        pages_->push(generalSettings_);
        generalSettings_->setType(_type);
    }

    void MainPage::onThemesSettingsShow(bool _showBackButton, QString _aimId)
    {
        themesSettings_->setBackButton(_showBackButton);
        themesSettings_->setTargetContact(_aimId);
        pages_->push(themesSettings_);
    }

    void MainPage::clearSearchMembers()
    {
        contactListWidget_->update();
    }

    void MainPage::cancelSelection()
    {
        assert(contactDialog_);
        contactDialog_->cancelSelection();
    }

    void MainPage::createGroupChat()
    {
        QStringList empty_list;
        auto oldModel = Logic::getChatMembersModel();
        Logic::setChatMembersModel(NULL);
        Ui::createGroupChat(empty_list);
        Logic::setChatMembersModel(oldModel);
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        assert(contactDialog_);
        return contactDialog_;
    }

    HistoryControlPage* MainPage::getHistoryPage(const QString& _aimId) const
    {
        return contactDialog_->getHistoryPage(_aimId);
    }

    void MainPage::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        contactDialog_->insertTopWidget(_aimId, _widget);
    }

    void MainPage::removeTopWidget(const QString& _aimId)
    {
        contactDialog_->removeTopWidget(_aimId);
    }

    void MainPage::showSidebar(const QString& _aimId, int _page)
    {
        if (searchContacts_ && pages_->currentWidget() == searchContacts_)
        {
            contactDialog_->takeSidebar();
            Sidebar* sidebar = contactDialog_->getSidebar();
            sidebar->preparePage(_aimId, (SidebarPages)_page);
            sidebar->setSidebarWidth(Utils::scale_value(428));
            sidebar->setFixedWidth(pages_->width());
            pages_->addWidget(sidebar);
            pages_->push(sidebar);
        }
        else
        {
            contactDialog_->showSidebar(_aimId, _page);
        }
    }

    bool MainPage::isSidebarVisible() const
    {
        return contactDialog_->isSidebarVisible();
    }

    void MainPage::setSidebarVisible(bool _show)
    {
        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
        {
            pages_->pop();
            pages_->removeWidget(sidebar);
            if (!_show)
                contactDialog_->setSidebarVisible(_show);
        }
        else
        {
            contactDialog_->setSidebarVisible(_show);
        }
    }

    void MainPage::restoreSidebar()
    {
        auto cont = Logic::getContactListModel()->selectedContact();
        if (isSidebarVisible() && !cont.isEmpty())
            showSidebar(cont, menu_page);
        else
            setSidebarVisible(false);
    }

    bool MainPage::isContactDialog() const
    {
        if (pages_ == 0)
            return false;

        return pages_->currentWidget() == contactDialog_;
    }

    void MainPage::onContactSelected(QString _contact)
    {
        pages_->poproot();

        if (searchContacts_)
        {
            pages_->removeWidget(searchContacts_);
            delete searchContacts_;
            searchContacts_ = nullptr;
        }

        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
    }

    void MainPage::onAddContactClicked()
    {
        if (!searchContacts_)
        {
            searchContacts_ = new SearchContactsWidget(this);
            pages_->addWidget(searchContacts_);
        }
        pages_->push(searchContacts_);
        searchContacts_->onFocus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::search_open_page);
    }

    void MainPage::searchBegin()
    {
        contactListWidget_->setSearchMode(true);
    }

    void MainPage::searchEnd()
    {
        if (NeedShowUnknownsHeader_)
            changeCLHead(true);

        searchWidget_->clearInput();

        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();
        emit Utils::InterConnector::instance().disableSearchInDialog();

        contactListWidget_->setSearchMode(false);
        contactListWidget_->setSearchInDialog(false);
    }

    void MainPage::searchInputClear()
    {
        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();

        if (!contactListWidget_->getSearchInDialog())
            contactListWidget_->setSearchMode(false);
    }

    void MainPage::onVoipCallIncoming(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            std::string callId = _account + "#" + _contact;

            auto it = incomingCallWindows_.find(callId);
            if (incomingCallWindows_.end() == it || !it->second)
            {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(_account, _contact));
                assert(!!window);

                if (!!window)
                {
                    window->showFrame();
                    incomingCallWindows_[callId] = window;
                }
            }
            else
            {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }
        }
    }

    void MainPage::destroyIncomingCallWindow(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            std::string call_id = _account + "#" + _contact;
            auto it = incomingCallWindows_.find(call_id);
            if (incomingCallWindows_.end() != it)
            {
                auto window = it->second;
                assert(!!window);

                if (!!window)
                {
                    window->hideFrame();
                }
            }
        }
    }

    void MainPage::onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);

        Utils::InterConnector::instance().getMainWindow()->closeGallery();
        Utils::InterConnector::instance().getMainWindow()->closeVideo();
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);
    }

    void MainPage::showVideoWindow()
    {
        if (videoWindow_)
        {
            if (videoWindow_->isMinimized())
            {
                videoWindow_->showNormal();
            }

            videoWindow_->activateWindow();
#ifndef _WIN32
            videoWindow_->raise();
#endif
        }
    }

    void MainPage::notifyApplicationWindowActive(const bool isActive)
    {
        if (contactDialog_)
        {
            contactDialog_->notifyApplicationWindowActive(isActive);
        }
    }

    void MainPage::recentsTabActivate(bool _selectUnread)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->changeTab(RECENTS);

            if (_selectUnread)
            {
                QString aimId = Logic::getRecentsModel
                ()->nextUnreadAimId();
                if (aimId.isEmpty())
                    aimId = Logic::getUnknownsModel()->nextUnreadAimId();
                if (!aimId.isEmpty())
                    contactListWidget_->select(aimId);

            }
        }
    }

    void MainPage::selectRecentChat(QString _aimId)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            if (_aimId.length() > 0)
            {
                contactListWidget_->select(_aimId);
            }
        }
    }

    void MainPage::contactListActivate(bool _addContact)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->allClicked();

            if (_addContact)
            {
                contactListWidget_->addContactClicked();
            }
        }
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType _item)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->settingsClicked();

            switch (_item)
            {
            case Utils::CommonSettingsType::CommonSettingsType_General:
            case Utils::CommonSettingsType::CommonSettingsType_VoiceVideo:
            case Utils::CommonSettingsType::CommonSettingsType_Notifications:
            case Utils::CommonSettingsType::CommonSettingsType_Themes:
            case Utils::CommonSettingsType::CommonSettingsType_About:
            case Utils::CommonSettingsType::CommonSettingsType_ContactUs:
            case Utils::CommonSettingsType::CommonSettingsType_AttachPhone:
            case Utils::CommonSettingsType::CommonSettingsType_AttachUin:
                Utils::InterConnector::instance().generalSettingsShow((int)_item);
                break;
            case Utils::CommonSettingsType::CommonSettingsType_Profile:
                Utils::InterConnector::instance().profileSettingsShow("");
                break;
            default:
                break;
            }

            if (_item == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo && contactListWidget_)
                contactListWidget_->selectSettingsVoipTab();
        }
    }

    void MainPage::onVoipShowVideoWindow(bool _enabled)
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

        if (!!videoWindow_)
        {
            if (_enabled)
            {
                videoWindow_->showFrame();
            }
            else
            {
                videoWindow_->hideFrame();

                bool wndMinimized = false;
                bool wndHiden = false;
                if (QWidget* parentWnd = window())
                {
                    wndHiden = !parentWnd->isVisible();
                    wndMinimized = parentWnd->isMinimized();
                }

                if (!Utils::foregroundWndIsFullscreened() && !wndMinimized && !wndHiden)
                {
                    raise();
                }
            }
        }
    }

    void MainPage::hideInput()
    {
        contactDialog_->hideInput();
    }

    QWidget* MainPage::showNoContactsYetSuggestions(QWidget* _parent, std::function<void()> _addNewContactsRoutine)
    {
        if (!noContactsYetSuggestions_)
        {
            noContactsYetSuggestions_ = new QWidget(_parent);
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
                        auto logoWidget = new QWidget(p);
                        logoWidget->setObjectName("logoWidget");
                        logoWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        logoWidget->setFixedSize(Utils::scale_value(80), Utils::scale_value(80));
                        pl->addWidget(logoWidget);
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
                        auto w = new Ui::TextEmojiWidget(p, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(24), CommonStyle::getTextCommonColor(), Utils::scale_value(44));
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
                        auto w = new Ui::TextEmojiWidget(p, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(24), CommonStyle::getTextCommonColor(), Utils::scale_value(30));
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
                        auto appStoreButton = new QPushButton(p);
                        appStoreButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        appStoreButton->setFlat(true);

                        auto appStoreImage = QString(":/resources/placeholders/content_badge_appstore_%1_100.png").
                            arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());

                        auto appStoreImageStyle = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); }")
                            .arg(appStoreImage)
                            .arg(appStoreImage);

                        Utils::ApplyStyle(appStoreButton, appStoreImageStyle);

                        appStoreButton->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        appStoreButton->setCursor(Qt::PointingHandCursor);
                        _parent->connect(appStoreButton, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(QUrl("https://app.appsflyer.com/id302707408?pid=icq_win"));
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_ios);
                        });
                        pl->addWidget(appStoreButton);

                        auto googlePlayWidget = new QPushButton(p);
                        googlePlayWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        googlePlayWidget->setFlat(true);

                        auto googlePlayImage = QString(":/resources/placeholders/content_badge_gplay_%1_100.png")
                            .arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());
                        auto googlePlayStyle = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); }")
                            .arg(googlePlayImage)
                            .arg(googlePlayImage);

                        Utils::ApplyStyle(googlePlayWidget, googlePlayStyle);

                        googlePlayWidget->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        googlePlayWidget->setCursor(Qt::PointingHandCursor);
                        _parent->connect(googlePlayWidget, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(QUrl("https://app.appsflyer.com/com.icq.mobile.client?pid=icq_win"));
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_android);
                        });
                        pl->addWidget(googlePlayWidget);
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
                        auto w1 = new Ui::TextEmojiWidget(p, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(18), CommonStyle::getTextCommonColor(), Utils::scale_value(46));
                        w1->setSizePolicy(QSizePolicy::Policy::Preferred, w1->sizePolicy().verticalPolicy());
                        w1->setText(QT_TRANSLATE_NOOP("placeholders", "or "));
                        pl->addWidget(w1);

                        auto w2 = new Ui::TextEmojiWidget(p, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(18), CommonStyle::getLinkColor(), Utils::scale_value(46));
                        w2->setSizePolicy(QSizePolicy::Policy::Preferred, w2->sizePolicy().verticalPolicy());
                        w2->setText(QT_TRANSLATE_NOOP("placeholders", "find friends"));
                        w2->setCursor(Qt::PointingHandCursor);
                        _parent->connect(w2, &Ui::TextEmojiWidget::clicked, _addNewContactsRoutine);
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
                        auto w = new Ui::TextEmojiWidget(p, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(15), QColor("#696969"), Utils::scale_value(20));
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

    void MainPage::showPlaceholder(Utils::PlaceholdersType _placeholdersType)
    {
        switch(_placeholdersType)
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

            pages_->insertWidget(0, showIntroduceYourselfSuggestions(pages_));
            pages_->poproot();
            break;

        default:
            break;
        }
    }

    QWidget* MainPage::showIntroduceYourselfSuggestions(QWidget* _parent)
    {
        if (!introduceYourselfSuggestions_)
        {
            introduceYourselfSuggestions_ = new IntroduceYourself(MyInfo()->aimId(), MyInfo()->friendlyName(), _parent);
            introduceYourselfSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            introduceYourselfSuggestions_->setStyleSheet("border: 20px solid red;");
        }
        return introduceYourselfSuggestions_;
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::getStatsSender()->trySendStats();
    }

    void MainPage::myInfo()
    {
        if (MyInfo()->friendlyName().isEmpty() && !get_gui_settings()->get_value(get_account_setting_name(settings_skip_intro_yourself).c_str(), false))
        {
            if (!recvMyInfo_)
            {
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself);
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself);
            }
        }
        else
        {
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        recvMyInfo_ = true;
    }

    void MainPage::openCreatedGroupChat()
    {
//        auto connect_id = connect(GetDispatcher(), SIGNAL(openChat(QString)), contactListWidget_, SLOT(select(QString)));
        auto connect_id = connect(GetDispatcher(), &core_dispatcher::openChat, this, [=](QString _aimId) { QTimer::singleShot(500, [=](){ contactListWidget_->select(_aimId); } ); });
        QTimer::singleShot(3000, [=] { disconnect(connect_id); } );
    }

    void MainPage::popPagesToRoot()
    {
        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            setSidebarVisible(false);

        pages_->poproot();
    }

    void MainPage::liveChatSelected()
    {
        pages_->push(liveChatsPage_);
    }

    void MainPage::spreadCL()
    {
        if (::ContactList::IsPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true);
        }
    }

    void MainPage::hideRecentsPopup()
    {
        if (leftPanelState_ == LeftPanelState::spreaded)
            setLeftPanelState(LeftPanelState::picture_only, true);
    }

    void MainPage::setLeftPanelState(LeftPanelState _newState, bool _withAnimation)
    {
        assert(_newState > LeftPanelState::min && _newState < LeftPanelState::max);

        if (leftPanelState_ == _newState)
            return;

        int new_cl_width = 0;
        leftPanelState_ = _newState;
        if (leftPanelState_ == LeftPanelState::normal)
        {
            clSpacer_->setFixedWidth(0);
            if (clHostLayout_->count() == 0)
                clHostLayout_->addWidget(contactsWidget_);
            backButtonHost_->hide();

            new_cl_width = ::ContactList::ItemWidth(false, false, false, false).px();
            contactListWidget_->setPictureOnlyView(false);
            searchWidget_->setShortView(false);
            searchWidget_->setSearchEditIconVisible(true);
            animateVisibilityCL(new_cl_width, _withAnimation);
            unknownBackButtonLabel_->show();
        }
        else if (leftPanelState_ == LeftPanelState::picture_only)
        {
            backButtonHost_->hide();

            new_cl_width = ::ContactList::ItemWidth(false, false, false, true).px();
            contactListWidget_->setButtonsVisibility(false);
            contactListWidget_->setPictureOnlyView(true);
            searchWidget_->clearInput();
            searchWidget_->setShortView(true);
            searchWidget_->setSearchEditIconVisible(false);
            animateVisibilityCL(new_cl_width, _withAnimation);
            unknownBackButtonLabel_->hide();
        }
        else if (leftPanelState_ == LeftPanelState::spreaded)
        {
            clSpacer_->setFixedWidth(::ContactList::ItemWidth(false, false, false, true).px());
            clHostLayout_->removeWidget(contactsWidget_);
            backButtonHost_->show();

            contactsLayout->setParent(contactsWidget_);
            new_cl_width = Utils::scale_value(360);

            contactListWidget_->setButtonsVisibility(false);
            contactListWidget_->setPictureOnlyView(false);
            contactListWidget_->setItemWidth(new_cl_width);
            searchWidget_->setShortView(false);
            searchWidget_->setSearchEditIconVisible(false);
            searchWidget_->setFocus();
            unknownBackButtonLabel_->show();

            animateVisibilityCL(new_cl_width, _withAnimation);
        }
        else
        {
            assert(false && "Left Panel state does not exist.");
        }
    }

    void MainPage::animCLWidthFinished()
    {
        if (leftPanelState_ == LeftPanelState::normal)
            contactListWidget_->setButtonsVisibility(true);
    }

    void MainPage::searchActivityChanged(bool _isActive)
    {
        if (!_isActive && leftPanelState_ == LeftPanelState::spreaded)
            hideRecentsPopup();
    }

    bool MainPage::isVideoWindowActive()
    {
        return (videoWindow_ && videoWindow_->isActiveWindow());
    }

    void MainPage::setFocusOnInput()
    {
        if (contactDialog_)
            contactDialog_->setFocusOnInputWidget();
    }

    void MainPage::clearSearchFocus()
    {
        searchWidget_->clearFocus();
    }

    void MainPage::onSendMessage(const QString& contact)
    {
        if (contactDialog_)
            contactDialog_->onSendMessage(contact);
    }

    void MainPage::startSearhInDialog(QString _aimid)
    {
        changeCLHead(false);

        setSearchFocus();

        Logic::getSearchModelDLG()->setSearchInDialog(_aimid);

        searchWidget_->clearInput();
        contactListWidget_->setSearchMode(true);
        contactListWidget_->setSearchInDialog(true);
    }

    void MainPage::changeCLHead(bool _showUnknownHeader)
    {
        if (unknownsHeader_->isVisible() != _showUnknownHeader)
        {
            searchWidget_->setVisible(!_showUnknownHeader);
            unknownsHeader_->setVisible(_showUnknownHeader);
        }
    }

    void MainPage::changeCLHeadToSearchSlot()
    {
        if (unknownsHeader_->isVisible())
        {
            NeedShowUnknownsHeader_ = false;
        }

        changeCLHead(false);
    }

    void MainPage::changeCLHeadToUnknownSlot()
    {
        if (!unknownsHeader_->isVisible())
        {
            NeedShowUnknownsHeader_ = true;
        }

        changeCLHead(true);
    }
}
