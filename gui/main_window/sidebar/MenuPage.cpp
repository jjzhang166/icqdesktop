#include "stdafx.h"
#include "MenuPage.h"
#include "SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../history_control/MessagesModel.h"
#include "../contact_list/AbstractSearchModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactListItemDelegate.h"
#include "../contact_list/ContactListItemRenderer.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/SearchWidget.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LabelEx.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace
{
    const int top_margin = 8;
    const int avatar_size = 80;
    const int left_margin = 24;
    const int right_margin = 28;
    const int text_margin = 12;
    const int text_top_margin = -4;
    const int text_top_spacing = 25;
    const int button_offset = 28;
    const int list_buttons_offset = 32;
    const int button_height = 44;
    const int preload_members = 20;
    const int item_height = 44;
    const int members_left_margin = 8;
    const int min_visible_memebres = 5;
    const int labels_left_margin = 68;
    const int labels_bottom_margin = 6;
    const int add_contact_margin = 16;
    const int back_button_size = 40;
    const int back_button_spacing = 10;
    const int left_margin_list = 16;
    const int desc_length = 100;
    const int more_left_margin = 4;
    const int checkbox_width = 44;
    const int checkbox_height = 24;
    const int list_left_margin = 10;
    const int search_margin_right = 26;
    const int approve_all_margin = 24;
    const int approve_all_bottom = 24;
    const int line_vertical_margin = 6;
    const int privacy_top_offset = 14;
    const int option_about_right_margin = 24;
    const int option_about_top_margin = 9;
    const int public_bottom_margin = 11;
    const int approved_top_space = 5;
    const int icon_offset = 42;
    const int reverse_margin = -5;

    const static QString transparent_background = "background: transparent;";

    enum widgets
    {
        main = 0,
        list = 1,
    };

    enum currentListTab
    {
        all = 0,
        block = 1,
        admins = 2,
        pending = 3,
        privacy = 4,
    };

    QMap<QString, QVariant> makeData(const QString& command, const QString& aimId)
    {
        QMap<QString, QVariant> result;
        result["command"] = command;
        result["aimid"] = aimId;
        return result;
    }
}

namespace Ui
{
    AddToChat::AddToChat(QWidget* _parent)
        : QWidget(_parent)
        , painter_(0)
        , Hovered_(false)
    {
    }

    void AddToChat::paintEvent(QPaintEvent*)
    {
        if (!painter_)
        {
            painter_ = new QPainter(this);
            painter_->setRenderHint(QPainter::Antialiasing);
        }

        painter_->begin(this);
        ::ContactList::ViewParams viewParams;
        ::ContactList::RenderServiceContact(*painter_, false, false
            , QT_TRANSLATE_NOOP("sidebar","Add to chat"), Data::ContactType::ADD_CONTACT, Utils::scale_value(members_left_margin), viewParams);
        if (Hovered_)
        {            
            QPainter p(this);
            painter_->fillRect(rect(), Ui::CommonStyle::getContactListHoveredColor());
        }
        painter_->end();
    }

    void AddToChat::mouseReleaseEvent(QMouseEvent* _e)
    {
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(_e);
    }

    void AddToChat::enterEvent(QEvent* e)
    {
        Hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void AddToChat::leaveEvent(QEvent* e)
    {
        Hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    MenuPage::MenuPage(QWidget* parent)
        : SidebarPage(parent)
        , currentTab_(all)
    {
        init();
    }

    void MenuPage::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(_e);
        p.fillRect(rect(), CommonStyle::getFrameTransparency());
        p.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        p.drawLine(contentsRect().topLeft(), contentsRect().bottomLeft());
    }

    void MenuPage::resizeEvent(QResizeEvent *e)
    {
        mainWidget_->setFixedWidth(e->size().width() - Utils::scale_value(8));
        return SidebarPage::resizeEvent(e);
    }

    void MenuPage::updateWidth()
    {
        name_->adjustHeight(width_ - Utils::scale_value(avatar_size + left_margin + text_margin + right_margin));
        firstLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        secondLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        thirdLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        approveAllLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin - (labels_left_margin - left_margin)));
        searchWidget_->setFixedWidth(width_ + Utils::scale_value(search_margin_right));
        approveAllWidget_->setFixedWidth(width_ + Utils::scale_value(search_margin_right));
        notificationsButton_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + checkbox_width - button_offset));
        publicButton_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + checkbox_width - list_buttons_offset));
        approvedButton_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + checkbox_width - list_buttons_offset));
        delegate_->setItemWidth(width_);
        delegate_->setLeftMargin(Utils::scale_value(members_left_margin));
        delegate_->setRightMargin(Utils::scale_value(right_margin - more_left_margin));
        addContact_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin));
        description_->adjustHeight(width_ - Utils::scale_value(avatar_size + left_margin + text_margin + right_margin));
        allMembersLabel_->setFixedWidth(width_ - Utils::scale_value(labels_left_margin + right_margin - more_left_margin) - allMembersCount_->width());
        blockLabel_->setFixedWidth(width_ - Utils::scale_value(labels_left_margin + right_margin - more_left_margin) - blockCount_->width());
        pendingLabel_->setFixedWidth(width_ - Utils::scale_value(labels_left_margin + right_margin - more_left_margin) - pendingCount_->width());

        avatarName_->setFixedWidth(width_);
        addToChat_->setFixedWidth(width_);
        favoriteButton_->setFixedWidth(width_);
        copyLink_->setFixedWidth(width_);
        themesButton_->setFixedWidth(width_);
        privacyButton_->setFixedWidth(width_);
        eraseHistoryButton_->setFixedWidth(width_);
        ignoreButton_->setFixedWidth(width_);
        quitAndDeleteButton_->setFixedWidth(width_);
        spamButtonAuth_->setFixedWidth(width_);
        deleteButton_->setFixedWidth(width_);
        spamButton_->setFixedWidth(width_);
        admins_->setFixedWidth(width_);
        allMembers_->setFixedWidth(width_);
        blockList_->setFixedWidth(width_);
        pendingList_->setFixedWidth(width_);
    }

    void MenuPage::initFor(const QString& aimId)
    {
        const auto newContact = (currentAimId_ != aimId);
        currentAimId_ = aimId;
        currentTab_ = all;

        avatar_->UpdateParams(currentAimId_, Logic::getContactListModel()->getDisplayName(currentAimId_));
        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(Logic::getContactListModel()->getDisplayName(currentAimId_), cursorName, Logic::Text2DocHtmlMode::Pass, false);

        if (newContact)
        {
            moreLabel_->hide();
            publicButton_->hide();
            publicCheckBox_->hide();
            publicAbout_->hide();
            copyLink_->hide();
            pendingList_->hide();
            description_->hide();
            blockList_->hide();
            privacyButton_->hide();
            nameLayout_->setAlignment(Qt::AlignVCenter);
            nameLayout_->invalidate();

            allMembersCount_->setText(QString());
            blockCount_->setText(QString());
            pendingCount_->setText(QString());
            chatMembersModel_->clear();
        }

        bool isFavorite = Logic::getRecentsModel()->isFavorite(currentAimId_);
        favoriteButton_->setImage(isFavorite ? ":/resources/sidebar_unfavorite_100.png" : ":/resources/sidebar_favorite_100.png");
        favoriteButton_->setText(isFavorite ? QT_TRANSLATE_NOOP("sidebar", "Remove from favorites") : QT_TRANSLATE_NOOP("sidebar", "Add to favorites"));

        bool isMuted = Logic::getContactListModel()->isMuted(currentAimId_);
        notificationsCheckbox_->blockSignals(true);
        notificationsCheckbox_->setChecked(!isMuted);
        notificationsCheckbox_->blockSignals(false);
        notificationsCheckbox_->adjustSize();

        bool isLiveChat = Logic::getContactListModel()->isLiveChat(currentAimId_);
        delegate_->setRenderRole(isLiveChat);

        info_.reset();
        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);
        avatarName_->setEnabled(!isChat);
        avatarName_->setCursor(isChat ? QCursor(Qt::ArrowCursor) : (Qt::PointingHandCursor));
        if (isChat)
        {
            chatMembersModel_->isShortView_ = true;
            chatMembersModel_->loadAllMembers(currentAimId_, preload_members);
            delegate_->setRegim((isLiveChat && (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())) ? Logic::ADMIN_MEMBERS : Logic::DELETE_MEMBERS);
        }
        else
        {
            chatMembersModel_->initForSingle(currentAimId_);
            delegate_->setRegim(Logic::CONTACT_LIST);
        }

        quitAndDeleteButton_->setVisible(isChat);
        allMembers_->setVisible(isChat);
        admins_->setVisible(isLiveChat);

        bool isNotAuth = Logic::getContactListModel()->isNotAuth(currentAimId_);
        bool isIgnored = Logic::getIgnoreModel()->getMemberItem(currentAimId_) != 0;

        deleteButton_->setVisible(!isNotAuth && !isChat);
        spamButtonAuth_->setVisible(!isNotAuth && !isChat);
        addContact_->setVisible(isNotAuth);
        addContactSpacer_->setVisible(isNotAuth);
        addContactSpacerTop_->setVisible(isNotAuth);
        spamButton_->setVisible(isNotAuth);
        addToChat_->setVisible(!isIgnored);
        thirdLine_->setVisible(!isIgnored);
        favoriteButton_->setVisible(!isNotAuth);
        notificationsCheckbox_->setVisible(!isNotAuth);
        notificationsButton_->setVisible(!isNotAuth);
        themesButton_->setVisible(!isNotAuth);
        secondLine_->setVisible(!isNotAuth);

        stackedWidget_->setCurrentIndex(main);
        updateWidth();
    }

    void MenuPage::init()
    {
        stackedWidget_ = new QStackedWidget(this);
        stackedWidget_->setContentsMargins(0, 0, 0, 0);
        auto layout = emptyVLayout(this);
        layout->addWidget(stackedWidget_);
        area_ = CreateScrollAreaAndSetTrScrollBar(stackedWidget_);
        stackedWidget_->insertWidget(main, area_);

        mainWidget_ = new QWidget(area_);
        mainWidget_->setStyleSheet(transparent_background);

        area_->setContentsMargins(0, 0, 0, 0);
        area_->setWidget(mainWidget_);
        area_->setWidgetResizable(true);
        area_->setFrameStyle(QFrame::NoFrame);
        area_->setStyleSheet(transparent_background);
        area_->horizontalScrollBar()->setEnabled(false);

        setStyleSheet(Utils::LoadStyle(":/main_window/sidebar/Sidebar.qss"));
        rootLayot_ = emptyVLayout(mainWidget_);
        rootLayot_->setAlignment(Qt::AlignTop);

        initAvatarAndName();
        initAddContactAndSpam();
        initFavoriteNotificationsTheme();
        initChatMembers();
        initEraseIgnoreDelete();
        initListWidget();
        connectSignals();

        stackedWidget_->setCurrentIndex(main);
        Utils::grabTouchWidget(area_->viewport(), true);
        Utils::grabTouchWidget(mainWidget_);
        connect(QScroller::scroller(area_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);
    }

    void MenuPage::initAvatarAndName()
    {
        rootLayot_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            avatarName_ = new ClickedWidget(mainWidget_);
            Utils::grabTouchWidget(avatarName_);
            avatarName_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            auto horLayout = new QHBoxLayout(avatarName_);
            horLayout->setSpacing(0);
            horLayout->setContentsMargins(0, Utils::scale_value(top_margin), 0, Utils::scale_value(top_margin));
            horLayout->setAlignment(Qt::AlignLeft);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            {
                auto vLayout = emptyVLayout();
                vLayout->setAlignment(Qt::AlignTop);
                avatar_ = new ContactAvatarWidget(avatarName_, QString(), QString(), Utils::scale_value(avatar_size), true);
                avatar_->setAttribute(Qt::WA_TransparentForMouseEvents);
                vLayout->addWidget(avatar_);
                horLayout->addLayout(vLayout);
            }
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(text_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            {
                nameLayout_ = emptyVLayout();
                nameLayout_->setAlignment(Qt::AlignVCenter);
                nameLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(text_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                name_ = new TextEditEx(avatarName_, Fonts::defaultAppFontFamily(), Utils::scale_value(20), Ui::CommonStyle::getTextCommonColor(), false, false);
                name_->setStyleSheet(transparent_background);
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setAttribute(Qt::WA_TransparentForMouseEvents);
                name_->setContextMenuPolicy(Qt::NoContextMenu);
                nameLayout_->addWidget(name_);

                description_ = new TextEditEx(avatarName_, Fonts::defaultAppFontFamily(), Utils::scale_value(15), QColor("#696969"), false, false);
                description_->setStyleSheet(transparent_background);
                description_->setFrameStyle(QFrame::NoFrame);
                description_->setContentsMargins(0, 0, 0, 0);
                description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setAttribute(Qt::WA_TransparentForMouseEvents);

                nameLayout_->addWidget(description_);

                {
                    auto hLayout = emptyHLayout();
                    hLayout->setAlignment(Qt::AlignLeft);
                    moreLabel_ = new LabelEx(mainWidget_);
                    moreLabel_->setFont(Fonts::appFontScaled(14));
                    QPalette p;
                    p.setColor(QPalette::Foreground, CommonStyle::getLinkColor());
                    moreLabel_->setPalette(p);
                    moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "More"));
                    moreLabel_->setContentsMargins(Utils::scale_value(more_left_margin), 0, 0, 0);
                    moreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                    hLayout->addWidget(moreLabel_);
                    nameLayout_->addLayout(hLayout);
                }

                horLayout->addLayout(nameLayout_);
            }
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            avatarName_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayot_->addWidget(avatarName_);
        }
    }

    void MenuPage::initAddContactAndSpam()
    {

        addContactSpacerTop_ = new QWidget(mainWidget_);
        addContactSpacerTop_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        Utils::grabTouchWidget(addContactSpacerTop_);
        rootLayot_->addWidget(addContactSpacerTop_);
        {
            auto horLayout = emptyHLayout();
            horLayout->setAlignment(Qt::AlignLeft);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            addContact_ = new CustomButton(mainWidget_, QString());
            addContact_->setText(QT_TRANSLATE_NOOP("sidebar", "Add contact"));
            addContact_->setAlign(Qt::AlignHCenter);
            addContact_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::ApplyStyle(addContact_, Ui::CommonStyle::getGreenButtonStyle());
            horLayout->addWidget(addContact_);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            rootLayot_->addLayout(horLayout);
        }

        addContactSpacer_ = new QWidget(mainWidget_);
        addContactSpacer_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        Utils::grabTouchWidget(addContactSpacer_);
        rootLayot_->addWidget(addContactSpacer_);

        spamButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_spam_100.png", QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        spamButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(spamButton_);
        rootLayot_->addWidget(spamButton_);

        firstLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
        Utils::grabTouchWidget(firstLine_);
        rootLayot_->addWidget(firstLine_);
    }

    void MenuPage::initFavoriteNotificationsTheme()
    {
        favoriteButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_favorite_100.png", QString(), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        favoriteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(favoriteButton_);
        rootLayot_->addWidget(favoriteButton_);

        copyLink_ = new ActionButton(mainWidget_, ":/resources/dialog_link_100.png", QString(), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        copyLink_->setCursor(QCursor(Qt::PointingHandCursor));
        copyLink_->setText(QT_TRANSLATE_NOOP("sidebar", "Copy link to chat"));
        Utils::grabTouchWidget(copyLink_);
        rootLayot_->addWidget(copyLink_);

        {
            auto horLayout = emptyHLayout();
            notificationsButton_ = new CustomButton(mainWidget_, ":/resources/contr_notifysettings_100.png");
            notificationsButton_->setOffsets(Utils::scale_value(button_offset), 0);
            notificationsButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Notifications"));
            notificationsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            notificationsButton_->setAlign(Qt::AlignLeft);
            notificationsButton_->setFocusPolicy(Qt::NoFocus);
            notificationsButton_->setFixedHeight(Utils::scale_value(button_height));
            notificationsButton_->adjustSize();
            Utils::grabTouchWidget(notificationsButton_);
            horLayout->addWidget(notificationsButton_);
            notificationsCheckbox_ = new QCheckBox(mainWidget_);
            notificationsCheckbox_->setObjectName("greenSwitcher");
            notificationsCheckbox_->adjustSize();
            notificationsCheckbox_->setCursor(QCursor(Qt::PointingHandCursor));
            notificationsCheckbox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
            Utils::grabTouchWidget(notificationsCheckbox_);
            horLayout->addWidget(notificationsCheckbox_);
            horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            rootLayot_->addLayout(horLayout);
        }

        themesButton_ = new ActionButton(mainWidget_, ":/resources/contr_themes_100.png", QT_TRANSLATE_NOOP("sidebar", "Wallpaper"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        themesButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(themesButton_);
        rootLayot_->addWidget(themesButton_);

        privacyButton_ = new ActionButton(mainWidget_, ":/resources/contr_privacy_100.png", QT_TRANSLATE_NOOP("sidebar", "Chat settings"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        privacyButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(privacyButton_);
        rootLayot_->addWidget(privacyButton_);

        secondLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));        
        Utils::grabTouchWidget(secondLine_);
        rootLayot_->addWidget(secondLine_);
    }

    void MenuPage::initChatMembers()
    {
        chatMembersModel_ = new Logic::ChatMembersModel(mainWidget_);
        chatMembersModel_->setSelectEnabled(false);
        chatMembersModel_->setFlag(Logic::HasMouseOver);
        getCurrentSearchModel(Logic::DELETE_MEMBERS)->setSelectEnabled(false);
        delegate_ = new Logic::ContactListItemDelegate(mainWidget_, Logic::DELETE_MEMBERS);
        {
            auto horLayout = emptyHLayout();
            {
                auto verLayout = emptyVLayout();
                addToChat_ = new AddToChat(mainWidget_);
                addToChat_->setFixedHeight(Utils::scale_value(item_height));
                addToChat_->setCursor(QCursor(Qt::PointingHandCursor));
                Utils::grabTouchWidget(addToChat_);
                verLayout->addWidget(addToChat_);
                verLayout->setAlignment(Qt::AlignLeft);
                {
                    QFont f = Fonts::appFontScaled(16);
                    QPalette p;
                    p.setColor(QPalette::Foreground, Ui::CommonStyle::getTextCommonColor());

                    admins_ = new ClickedWidget(mainWidget_);
                    admins_->setFixedHeight(Utils::scale_value(button_height));
                    admins_->setCursor(QCursor(Qt::PointingHandCursor));
                    auto horLayout2 = emptyHLayout(admins_);
                    horLayout2->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    auto adminsLabel = new QLabel(admins_);
                    adminsLabel->setFont(f);
                    adminsLabel->setPalette(p);
                    adminsLabel->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
                    adminsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout2->addWidget(adminsLabel);
                    horLayout2->addSpacerItem(new QSpacerItem(QWIDGETSIZE_MAX, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    verLayout->addWidget(admins_);
                    Utils::grabTouchWidget(admins_);

                    allMembers_ = new ClickedWidget(mainWidget_);
                    allMembers_->setCursor(QCursor(Qt::PointingHandCursor));
                    allMembers_->setFixedHeight(Utils::scale_value(button_height));
                    auto horLayout3 = emptyHLayout(allMembers_);
                    horLayout3->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    allMembersLabel_ = new QLabel(allMembers_);
                    allMembersLabel_->setFont(f);
                    allMembersLabel_->setPalette(p);
                    allMembersLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Members"));
                    allMembersLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout3->addWidget(allMembersLabel_);
                    allMembersCount_ = new QLabel(allMembers_);
                    allMembersCount_->setPalette(p);
                    allMembersCount_->setFont(f);
                    allMembersCount_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout3->addWidget(allMembersCount_);
                    horLayout3->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    horLayout3->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    Utils::grabTouchWidget(allMembers_);
                    verLayout->addWidget(allMembers_);

                    pendingList_ = new ClickedWidget(mainWidget_);
                    pendingList_->setCursor(QCursor(Qt::PointingHandCursor));
                    pendingList_->setFixedHeight(Utils::scale_value(button_height));
                    auto horLayout5 = emptyHLayout(pendingList_);
                    horLayout5->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    pendingLabel_ = new QLabel(pendingList_);
                    pendingLabel_->setFont(f);
                    pendingLabel_->setPalette(p);
                    pendingLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"));
                    pendingLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout5->addWidget(pendingLabel_);
                    pendingCount_ = new QLabel(pendingList_);
                    pendingCount_->setPalette(p);
                    pendingCount_->setFont(f);
                    pendingCount_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout5->addWidget(pendingCount_);
                    horLayout5->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    horLayout5->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    Utils::grabTouchWidget(pendingList_);
                    verLayout->addWidget(pendingList_);

                    blockList_ = new ClickedWidget(mainWidget_);
                    blockList_->setCursor(QCursor(Qt::PointingHandCursor));
                    blockList_->setFixedHeight(Utils::scale_value(button_height));
                    auto horLayout4 = emptyHLayout(blockList_);
                    horLayout4->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    blockLabel_ = new QLabel(blockList_);
                    blockLabel_->setFont(f);
                    blockLabel_->setPalette(p);
                    blockLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
                    blockLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout4->addWidget(blockLabel_);
                    blockCount_ = new QLabel(blockList_);
                    blockCount_->setPalette(p);
                    blockCount_->setFont(f);
                    blockCount_->setAttribute(Qt::WA_TransparentForMouseEvents);
                    horLayout4->addWidget(blockCount_);
                    horLayout4->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    horLayout4->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    Utils::grabTouchWidget(blockList_);
                    verLayout->addWidget(blockList_);
                }
                horLayout->addLayout(verLayout);
            }
            rootLayot_->addLayout(horLayout);
        }

        thirdLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
        Utils::grabTouchWidget(thirdLine_);
        rootLayot_->addWidget(thirdLine_);
    }

    void MenuPage::initEraseIgnoreDelete()
    {
        eraseHistoryButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_closechat_100.png", QT_TRANSLATE_NOOP("sidebar", "Clear history"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        eraseHistoryButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(eraseHistoryButton_);
        rootLayot_->addWidget(eraseHistoryButton_);

        ignoreButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_ignore_100.png", QT_TRANSLATE_NOOP("sidebar", "Ignore"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        ignoreButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(ignoreButton_);
        rootLayot_->addWidget(ignoreButton_);

        quitAndDeleteButton_ = new ActionButton(mainWidget_, ":/resources/contr_signout_100.png", QT_TRANSLATE_NOOP("sidebar", "Leave and delete"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        quitAndDeleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(quitAndDeleteButton_);
        rootLayot_->addWidget(quitAndDeleteButton_);

        spamButtonAuth_ = new ActionButton(mainWidget_, ":/resources/sidebar_spam_100.png", QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        spamButtonAuth_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(spamButtonAuth_);
        rootLayot_->addWidget(spamButtonAuth_);

        deleteButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_delete_100.png", QT_TRANSLATE_NOOP("sidebar", "Delete"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        deleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(deleteButton_);
        rootLayot_->addWidget(deleteButton_);

        bottomSpacer_ = new QSpacerItem(0, QWIDGETSIZE_MAX, QSizePolicy::Preferred, QSizePolicy::Expanding);
        rootLayot_->addSpacerItem(bottomSpacer_);
        rootLayot_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(labels_bottom_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));

    }

    void MenuPage::initListWidget()
    {
        listWidget_ = new QWidget(stackedWidget_);
        stackedWidget_->insertWidget(list, listWidget_);
        auto vLayout = emptyVLayout(listWidget_);
        vLayout->setAlignment(Qt::AlignTop);
        vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(add_contact_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            auto horLayout = emptyHLayout();
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
            backButton_ = new CustomButton(listWidget_, ":/resources/contr_back_100.png");
            backButton_->setHoverImage(":/resources/contr_back_100_hover.png");
            backButton_->setActiveImage(":/resources/contr_back_100_active.png");
            backButton_->setFixedSize(Utils::scale_value(back_button_size), Utils::scale_value(back_button_size));
            backButton_->setCursor(QCursor(Qt::PointingHandCursor));
            horLayout->addWidget(backButton_);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
            listLabel_ = new QLabel(listWidget_);
            listLabel_->setFont(Fonts::appFontScaled(20));
            QPalette p;
            p.setColor(QPalette::Foreground, Ui::CommonStyle::getTextCommonColor());
            listLabel_->setPalette(p);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Members"));
            horLayout->addWidget(listLabel_);
            horLayout->addSpacerItem(new QSpacerItem(1, 0, QSizePolicy::Expanding));
            vLayout->addLayout(horLayout);

            privacyWidget_ = new QWidget(listWidget_);
            auto privacyLayout = emptyVLayout(privacyWidget_);
            privacyLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(privacy_top_offset), QSizePolicy::Preferred, QSizePolicy::Fixed));
            {
                auto vlayout = emptyVLayout();
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = emptyHLayout();
                horLayout->setAlignment(Qt::AlignTop);
                publicButton_ = new CustomButton(listWidget_, ":/resources/sidebar_public_100.png");
                publicButton_->setOffsets(Utils::scale_value(list_buttons_offset), 0);
                publicButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Public chat"));
                publicButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
                publicButton_->setAlign(Qt::AlignLeft);
                publicButton_->setFocusPolicy(Qt::NoFocus);
                publicButton_->setFixedHeight(Utils::scale_value(button_height));
                publicButton_->adjustSize();
                Utils::ApplyStyle(publicButton_, QString("padding-left: %1dip").arg(list_buttons_offset + icon_offset));
                horLayout->addWidget(publicButton_);

                publicCheckBox_ = new QCheckBox(listWidget_);
                publicCheckBox_->setObjectName("greenSwitcher");
                publicCheckBox_->adjustSize();
                publicCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                publicCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(publicCheckBox_);
                horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                
                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(list_buttons_offset + icon_offset), 0, QSizePolicy::Fixed));
                publicAbout_ = new LabelEx(listWidget_);
                publicAbout_->setFont(Fonts::appFontScaled(13));
                QPalette p;
                p.setBrush(QPalette::Foreground, QColor("#696969"));
                publicAbout_->setPalette(p);
                publicAbout_->setText(QT_TRANSLATE_NOOP("sidebar", "Chat will be visible to everyone"));
                publicAbout_->setWordWrap(true);
                hLayout->addWidget(publicAbout_);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(option_about_right_margin), 0, QSizePolicy::Fixed));
                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                vlayout->addLayout(horLayout);
                vlayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(reverse_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                vlayout->addLayout(hLayout);
                privacyLayout->addLayout(vlayout);
            }

            publicBottomSpace_ = new QWidget(listWidget_);
            publicBottomSpace_->setFixedHeight(Utils::scale_value(public_bottom_margin));
            privacyLayout->addWidget(publicBottomSpace_);

            {
                auto vlayout = emptyVLayout();
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = emptyHLayout();
                approvedButton_ = new CustomButton(listWidget_, ":/resources/sidebar_approve_100.png");
                approvedButton_->setOffsets(Utils::scale_value(list_buttons_offset), 0);
                approvedButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Join with Approval"));
                approvedButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
                approvedButton_->setAlign(Qt::AlignLeft);
                approvedButton_->setFocusPolicy(Qt::NoFocus);
                approvedButton_->setFixedHeight(Utils::scale_value(button_height));
                approvedButton_->adjustSize();
                Utils::ApplyStyle(approvedButton_, QString("padding-left: %1dip").arg(list_buttons_offset + icon_offset));
                horLayout->addWidget(approvedButton_);
                approvedCheckBox_ = new QCheckBox(listWidget_);
                approvedCheckBox_->setObjectName("greenSwitcher");
                approvedCheckBox_->adjustSize();
                approvedCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                approvedCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(approvedCheckBox_);
                horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));

                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(list_buttons_offset + icon_offset), 0, QSizePolicy::Fixed));
                approvalAbout_ = new LabelEx(listWidget_);
                approvalAbout_->setFont(Fonts::appFontScaled(13));
                QPalette p;
                p.setBrush(QPalette::Foreground, QColor("#696969"));
                approvalAbout_->setPalette(p);
                approvalAbout_->setText(QT_TRANSLATE_NOOP("sidebar", "Admin approval required to join"));
                approvalAbout_->setWordWrap(true);
                hLayout->addWidget(approvalAbout_);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(option_about_right_margin), 0, QSizePolicy::Fixed));
                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                vlayout->addLayout(horLayout);
                vlayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(reverse_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                vlayout->addLayout(hLayout);
                vlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
                privacyLayout->addLayout(vlayout);
            }

            vLayout->addWidget(privacyWidget_);

            contactListWidget_ = new QWidget(listWidget_);
            auto contactListLayout = emptyVLayout(contactListWidget_);
            {
                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(list_left_margin), 0, QSizePolicy::Fixed));
                searchWidget_ = new SearchWidget(false, listWidget_);
                searchWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                searchWidget_->setShowButton(false);
                searchWidget_->setTransparent(true);
                hLayout->addWidget(searchWidget_);
                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                contactListLayout->addLayout(hLayout);
            }
            cl_ = new Ui::ContactList(listWidget_, Logic::DELETE_MEMBERS, chatMembersModel_);
            cl_->changeTab(SEARCH);
            cl_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            cl_->setContentsMargins(0, 0, 0, 0);
            cl_->setClDelegate(delegate_);
            cl_->setTransparent(true);
            contactListLayout->addWidget(cl_);
            vLayout->addWidget(contactListWidget_);
            {
                approveAllWidget_ = new QWidget(listWidget_);
                auto verLayout = emptyVLayout(approveAllWidget_);
                approveAllLine_ = new LineWidget(listWidget_, Utils::scale_value(left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
                verLayout->addWidget(approveAllLine_);
                auto hlayout = emptyHLayout();
                hlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                approveAll_ = new LabelEx(listWidget_);
                approveAll_->setFont(Fonts::appFontScaled(16));
                QPalette p;
                p.setColor(QPalette::Foreground, CommonStyle::getLinkColor());
                approveAll_->setPalette(p);
                approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All"));
                approveAll_->setCursor(QCursor(Qt::PointingHandCursor));
                hlayout->addWidget(approveAll_);
                hlayout->addSpacerItem(new QSpacerItem(Utils::scale_value(search_margin_right + approve_all_margin), 0, QSizePolicy::Fixed));
                verLayout->addLayout(hlayout);
                verLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(approve_all_bottom), QSizePolicy::Preferred, QSizePolicy::Fixed));
            }
            vLayout->addWidget(approveAllWidget_);
        }
    }

    void MenuPage::connectSignals()
    {
        connect(favoriteButton_, SIGNAL(clicked()), this, SLOT(favoritesClicked()), Qt::QueuedConnection);
        connect(copyLink_, SIGNAL(clicked()), this, SLOT(copyLinkClicked()), Qt::QueuedConnection);
        connect(themesButton_, SIGNAL(clicked()), this, SLOT(themesClicked()), Qt::QueuedConnection);
        connect(privacyButton_, SIGNAL(clicked()), this, SLOT(privacyClicked()), Qt::QueuedConnection);
        connect(eraseHistoryButton_, SIGNAL(clicked()), this, SLOT(eraseHistoryClicked()), Qt::QueuedConnection);
        connect(ignoreButton_, SIGNAL(clicked()), this, SLOT(ignoreClicked()), Qt::QueuedConnection);
        connect(notificationsCheckbox_, SIGNAL(stateChanged(int)), this, SLOT(notificationsChecked(int)), Qt::QueuedConnection);
        connect(publicCheckBox_, SIGNAL(stateChanged(int)), this, SLOT(publicChanged(int)), Qt::QueuedConnection);
        connect(approvedCheckBox_, SIGNAL(stateChanged(int)), this, SLOT(approvedChanged(int)), Qt::QueuedConnection);
        connect(quitAndDeleteButton_, SIGNAL(clicked()), this, SLOT(quitClicked()), Qt::QueuedConnection);
        connect(addToChat_, SIGNAL(clicked()), this, SLOT(addToChatClicked()), Qt::QueuedConnection);
        connect(addContact_, SIGNAL(clicked()), this, SLOT(addContactClicked()), Qt::QueuedConnection);
        connect(spamButton_, SIGNAL(clicked()), this, SLOT(spamClicked()), Qt::QueuedConnection);
        connect(spamButtonAuth_, SIGNAL(clicked()), this, SLOT(spam()), Qt::QueuedConnection);
        connect(deleteButton_, SIGNAL(clicked()), this, SLOT(remove()), Qt::QueuedConnection);

        connect(avatarName_, SIGNAL(clicked()), this, SLOT(avatarClicked()), Qt::QueuedConnection);

        connect(allMembers_, SIGNAL(clicked()), this, SLOT(allMemebersClicked()), Qt::QueuedConnection);
        connect(backButton_, SIGNAL(clicked()), this, SLOT(backButtonClicked()), Qt::QueuedConnection);

        connect(Logic::getRecentsModel(), SIGNAL(favoriteChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatBlocked(QList<Data::ChatMemberInfo>)), this, SLOT(chatBlocked(QList<Data::ChatMemberInfo>)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatPending(QList<Data::ChatMemberInfo>)), this, SLOT(chatPending(QList<Data::ChatMemberInfo>)), Qt::QueuedConnection);
        connect(cl_, SIGNAL(itemClicked(QString)), this, SLOT(contactClicked(QString)), Qt::QueuedConnection);

        connect(cl_, SIGNAL(searchEnd()), searchWidget_, SLOT(searchCompleted()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchBegin()), this,  SLOT(searchBegin()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchEnd()), this,  SLOT(searchEnd()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(enterPressed()), cl_, SLOT(searchResult()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(upPressed()), cl_, SLOT(searchUpPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(downPressed()), cl_, SLOT(searchDownPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(moreLabel_, SIGNAL(clicked()), this, SLOT(moreClicked()), Qt::QueuedConnection);
        connect(admins_, SIGNAL(clicked()), this, SLOT(adminsClicked()), Qt::QueuedConnection);
        connect(blockList_, SIGNAL(clicked()), this, SLOT(blockedClicked()), Qt::QueuedConnection);
        connect(pendingList_, SIGNAL(clicked()), this, SLOT(pendingClicked()), Qt::QueuedConnection);
        connect(Logic::GetMessagesModel(), SIGNAL(chatEvent(QString)), this, SLOT(chatEvent(QString)), Qt::QueuedConnection);

        connect(approveAll_, SIGNAL(clicked()), this, SLOT(approveAllClicked()), Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), SIGNAL(setChatRoleResult(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(blockMemberResult(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(pendingListResult(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
    }

    void MenuPage::initDescription(const QString& description, bool full)
    {
        QString normalizedDesc = description == " " ? QString() : description;
        description_->setPlainText(QString());
        QTextCursor cursorDesc = description_->textCursor();
        Logic::Text2Doc(normalizedDesc, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);
        moreLabel_->setVisible(normalizedDesc.length() > desc_length);
        if (!full)
        {
            if (normalizedDesc.length() > desc_length)
            {
                QString newDescription = normalizedDesc.left(desc_length);
                newDescription += "...";

                description_->setPlainText(QString());
                cursorDesc = description_->textCursor();
                Logic::Text2Doc(newDescription, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);

                moreLabel_->show();
            }
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "more"));
        }
        else
        {
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "hide"));
        }


        if (info_ && (info_->YourRole_ == "admin" || (info_->Live_ == false && info_->ApprovedJoin_ == false) || info_->Creator_ == MyInfo()->aimId()))
        {
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "edit"));
            moreLabel_->setVisible(true);
        }

        description_->setVisible(!normalizedDesc.isEmpty());

        nameLayout_->setAlignment(normalizedDesc.isEmpty() ? Qt::AlignVCenter : Qt::AlignTop);
        nameLayout_->invalidate();

        updateWidth();
    }

    void MenuPage::blockUser(const QString& aimId, bool blockUser)
    {
        auto cont = chatMembersModel_->getMemberItem(aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            blockUser ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to block user in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_bool("block", blockUser);
            Ui::GetDispatcher()->post_message_to_core("chats/block", collection.get());

            if (!blockUser)
            {
                if (info_->BlockedCount_ == 1)
                    backButtonClicked();
                chatMembersModel_->loadBlocked();
            }
        }
    }

    void MenuPage::actionResult(int)
    {
        if (currentTab_ == block)
            chatMembersModel_->loadBlocked();
        else if (currentTab_ == pending)
            chatMembersModel_->loadPending();
        else
            chatMembersModel_->loadAllMembers();
    }

    void MenuPage::approveAllClicked()
    {
        if (currentTab_ != pending)
            return;

        auto members = chatMembersModel_->getMembers();
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);

        core::ifptr<core::iarray> contacts_array(collection->create_array());
        contacts_array->reserve(members.size());

        for (auto iter : members)
        {
            core::ifptr<core::ivalue> val(collection->create_value());
            val->set_as_string(iter.AimId_.toStdString().c_str(), (int)iter.AimId_.length());
            contacts_array->push_back(val.get());
        }

        collection.set_value_as_array("contacts", contacts_array.get());
        collection.set_value_as_bool("approve", true);
        Ui::GetDispatcher()->post_message_to_core("chats/pending/resolve", collection.get());

        backButtonClicked();
    }

    void MenuPage::changeRole(const QString& aimId, bool moder)
    {
        auto cont = chatMembersModel_->getMemberItem(aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            moder ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_qstring("role", moder ? "moder" : "member");
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }
    }

    void MenuPage::approve(const QString& aimId, bool approve)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        core::ifptr<core::iarray> contacts_array(collection->create_array());
        contacts_array->reserve(1);
        core::ifptr<core::ivalue> val(collection->create_value());
        val->set_as_string(aimId.toStdString().c_str(), (int)aimId.length());
        contacts_array->push_back(val.get());
        collection.set_value_as_array("contacts", contacts_array.get());
        collection.set_value_as_bool("approve", approve);
        Ui::GetDispatcher()->post_message_to_core("chats/pending/resolve", collection.get());

        if (info_->PendingCount_ == 1)
            backButtonClicked();
    }

    void MenuPage::changeTab(int tab)
    {
        Logic::setChatMembersModel(chatMembersModel_);
        cl_->changeTab(SEARCH);
        stackedWidget_->setCurrentIndex(list);

        switch (tab)
        {
        case all:
            chatMembersModel_->loadAllMembers();
            Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
            if (Logic::getContactListModel()->isLiveChat(currentAimId_))
                delegate_->setRegim(
                (chatMembersModel_->isAdmin() || chatMembersModel_->isModer()) ?
                    Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST
                );
            else if (Logic::getContactListModel()->isChat(currentAimId_))
                delegate_->setRegim(Logic::DELETE_MEMBERS);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);

            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Members"));
            currentTab_ = all;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            break;

        case block:
            chatMembersModel_->loadBlocked();
            Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
            if (
                Logic::getContactListModel()->isLiveChat(currentAimId_) &&
                (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
                )
                delegate_->setRegim(Logic::DELETE_MEMBERS);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
            currentTab_ = block;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_blocked);
            break;

        case admins:
            chatMembersModel_->adminsOnly();
            Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
            if (Logic::getContactListModel()->isLiveChat(currentAimId_) && (chatMembersModel_->isAdmin()))
                delegate_->setRegim(Logic::DELETE_MEMBERS);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
            currentTab_ = admins;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_admins);
            break;

        case pending:
            chatMembersModel_->loadPending();
            Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
            delegate_->setRegim(Logic::PENDING_MEMBERS);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"));
            currentTab_ = pending;
            approveAllWidget_->show();
            privacyWidget_->hide();
            contactListWidget_->show();
            break;

        case privacy:
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Chat settings"));
            currentTab_ = privacy;
            approveAllWidget_->hide();
            privacyWidget_->show();
            contactListWidget_->hide();
            break;

        default:
            assert(!"wrong index of sidebar page");
            break;
        }
    }

    void MenuPage::contactChanged(QString aimid)
    {
        if (aimid != currentAimId_)
            return;

        int index = stackedWidget_->currentIndex();
        int curTab = currentTab_;
        initFor(currentAimId_);
        stackedWidget_->setCurrentIndex(index);
        currentTab_ = curTab;
    }

    void MenuPage::contactClicked(QString aimId)
    {
        auto global_cursor_pos = mapFromGlobal(QCursor::pos());
        if (!rect().contains(global_cursor_pos))
            return;

        if (global_cursor_pos.x() > width_)
            return;

        auto minXofDeleteImage = ::ContactList::GetXOfRemoveImg(false, true, width_);
        auto minXOfApprove = minXofDeleteImage - Utils::scale_value(48);
        bool advMenu = (
            Logic::getContactListModel()->isLiveChat(currentAimId_) &&
            (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
            );
        if (
            Logic::getContactListModel()->isChat(currentAimId_) &&
            !Logic::getContactListModel()->isLiveChat(currentAimId_)
            )
            advMenu = true;

        if (advMenu && global_cursor_pos.x() > minXofDeleteImage &&
            global_cursor_pos.x() <= (minXofDeleteImage + Utils::scale_value(20)) &&
            chatMembersModel_)
        {
            if (currentTab_ == all &&
                Logic::getContactListModel()->isLiveChat(currentAimId_) &&
                (chatMembersModel_->isModer() || chatMembersModel_->isAdmin()))
            {
                auto menu = new ContextMenu(mainWidget_);
                auto cont = chatMembersModel_->getMemberItem(aimId);
                bool myInfo = cont->AimId_ == MyInfo()->aimId();
                if (cont->Role_ != "admin" && chatMembersModel_->isAdmin())
                {
                    if (cont->Role_ == "moder")
                        menu->addActionWithIcon(
                            QIcon(Utils::parse_image_name(":/resources/dialog_removeking_100.png")),
                            QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"),
                            makeData("revoke_admin", aimId));
                    else
                        menu->addActionWithIcon(
                            QIcon(Utils::parse_image_name(":/resources/dialog_king_100.png")),
                            QT_TRANSLATE_NOOP("sidebar", "Make admin"),
                            makeData("make_admin", aimId));
                }
                if (!myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(":/resources/dialog_profile_100.png")),
                        QT_TRANSLATE_NOOP("sidebar", "Profile"),
                        makeData("profile", aimId));

                if ((cont->Role_ != "admin" && cont->Role_ != "moder") || myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")),
                        QT_TRANSLATE_NOOP("sidebar", "Delete from chat"),
                        makeData("remove", aimId));

                if (cont->Role_ != "admin" && cont->Role_ != "moder")
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(":/resources/dialog_block_100.png")),
                        QT_TRANSLATE_NOOP("sidebar", "Block"),
                        makeData("block", aimId));

                if (!myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(":/resources/dialog_spam_100.png")),
                        QT_TRANSLATE_NOOP("sidebar", "Report spam"),
                        makeData("spam", aimId));

                menu->invertRight(true);
                connect(menu, &ContextMenu::triggered, this, &MenuPage::menu, Qt::QueuedConnection);
                menu->popup(QCursor::pos());
            }
            else if (currentTab_ == block)
            {
                blockUser(aimId, false);
            }
            else if (currentTab_ == admins)
            {
                changeRole(aimId, false);
            }
            else if (currentTab_ == pending)
            {
                approve(aimId, false);
            }
            else
            {
                deleteMemberDialog(chatMembersModel_, aimId, Logic::DELETE_MEMBERS, this);
            }
        }
        else if (currentTab_ == pending &&
            global_cursor_pos.x() > minXOfApprove &&
            global_cursor_pos.x() <= (minXOfApprove + Utils::scale_value(32)) &&
            chatMembersModel_)
        {
            approve(aimId, true);
        }
        else
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_members_list);
    }

    void MenuPage::moreClicked()
    {
        if (moreLabel_->text() == QT_TRANSLATE_NOOP("sidebar", "more"))
            initDescription(info_->About_, true);
        else if (moreLabel_->text() == QT_TRANSLATE_NOOP("sidebar", "hide"))
            initDescription(info_->About_, false);
        else
            Utils::InterConnector::instance().showSidebar(currentAimId_, profile_page);
    }

    void MenuPage::adminsClicked()
    {
        changeTab(admins);
    }

    void MenuPage::blockedClicked()
    {
        changeTab(block);
    }

    void MenuPage::pendingClicked()
    {
        changeTab(pending);
    }

    void MenuPage::avatarClicked()
    {
        Utils::InterConnector::instance().showSidebar(currentAimId_, profile_page);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_sidebar);
    }

    void MenuPage::chatEvent(QString aimId)
    {
        if (aimId == currentAimId_)
        {
            if (currentTab_ == pending)
                chatMembersModel_->loadPending();
            else
                chatMembersModel_->loadAllMembers();
        }
    }

    void MenuPage::menu(QAction* action)
    {
        const auto params = action->data().toMap();
        const auto command = params["command"].toString();
        const auto aimId = params["aimid"].toString();

        if (command == "remove")
        {
            deleteMemberDialog(chatMembersModel_, aimId, Logic::DELETE_MEMBERS, this);
        }

        if (command == "profile")
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }

        if (command == "spam")
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_sidebar);
            }
        }

        if (command == "block")
        {
            blockUser(aimId, true);
        }

        if (command == "make_admin")
        {
            changeRole(aimId, true);
        }

        if (command == "revoke_admin")
        {
            changeRole(aimId, false);
        }
    }

    void MenuPage::searchBegin()
    {
        cl_->setSearchMode(true);
    }

    void MenuPage::searchEnd()
    {
        Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
    }

    void MenuPage::backButtonClicked()
    {
        chatMembersModel_->loadAllMembers();
        Logic::setChatMembersModel(NULL);
        stackedWidget_->setCurrentIndex(main);
        currentTab_ = all;
    }

    void MenuPage::allMemebersClicked()
    {
        changeTab(all);
    }

    void MenuPage::favoritesClicked()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", currentAimId_);
        Ui::GetDispatcher()->post_message_to_core(Logic::getRecentsModel()->isFavorite(currentAimId_) ? "unfavorite" : "favorite", collection.get());
    }

    void MenuPage::copyLinkClicked()
    {
        if (info_ == 0)
            return;
        
        QApplication::clipboard()->setText("https://icq.com/chat/" + info_->Stamp_);
        copyLink_->setLink(QT_TRANSLATE_NOOP("sidebar", "link copied"), CommonStyle::getLinkColor());
    }

    void MenuPage::themesClicked()
    {
        emit Utils::InterConnector::instance().themesSettingsShow(true, currentAimId_);
    }

    void MenuPage::privacyClicked()
    {
        changeTab(privacy);
    }

    void MenuPage::eraseHistoryClicked()
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            nullptr);

        if (!confirmed)
        {
            return;
        }

        Logic::GetMessagesModel()->eraseHistory(currentAimId_);
    }

    void MenuPage::ignoreClicked()
    {
        if (Logic::getContactListModel()->ignoreContactWithConfirm(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_sidebar);
            Utils::InterConnector::instance().setSidebarVisible(false);
        }
    }

    void MenuPage::quitClicked()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to leave chat?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            NULL);
        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_sidebar);
        }
    }

    void MenuPage::spam()
    {
        if (Logic::getContactListModel()->blockAndSpamContact(currentAimId_))
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_profile_page);
        }
    }

    void MenuPage::remove()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete contact?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            NULL);
        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_profile_page);
        }
    }

    void MenuPage::touchScrollStateChanged(QScroller::State st)
    {
        moreLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        addContact_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        addToChat_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        favoriteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        copyLink_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        themesButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        privacyButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        eraseHistoryButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        ignoreButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        quitAndDeleteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        spamButtonAuth_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        deleteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        spamButton_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        admins_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        allMembers_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        blockList_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        pendingList_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
    }

    void MenuPage::addToChatClicked()
    {
        if (!Logic::getContactListModel()->isChat(currentAimId_))
        {
            QStringList list;
            list.append(currentAimId_);
            createGroupChat(list);
            return;
        }

        Logic::setChatMembersModel(chatMembersModel_);

        if (!chatMembersModel_->isFullListLoaded_)
        {
            chatMembersModel_->loadAllMembers();
        }

        SelectContactsWidget select_members_dialog(NULL, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("sidebar", "Add to chat"), QT_TRANSLATE_NOOP("groupchat_pages", "Done"), QString(), this);
        connect(this, SIGNAL(updateMembers()), &select_members_dialog, SLOT(updateMembers()), Qt::QueuedConnection);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_add_member_sidebar);
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }
        Logic::setChatMembersModel(NULL);
    }

    void MenuPage::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> info)
    {
        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            allMembersCount_->setText(QVariant(info_->MembersCount_).toString());
            allMembers_->setVisible(info_->MembersCount_ > 0);

            initDescription(info_->About_);

            int blockedCount = info_->BlockedCount_;
            bool isLiveChat = Logic::getContactListModel()->isLiveChat(currentAimId_);
            copyLink_->setVisible(isLiveChat);
            blockCount_->setText(QVariant(blockedCount).toString());
            blockList_->setVisible(blockedCount != 0 && isLiveChat);
            int pendingCount = info_->PendingCount_;
            pendingCount_->setText(QVariant(pendingCount).toString());
            approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All (") + QVariant(pendingCount).toString() + ")");
            pendingList_->setVisible(info->ApprovedJoin_ && pendingCount != 0 && (info->YourRole_ == "admin" || info_->YourRole_ == "moder" || info_->Creator_ == MyInfo()->aimId()));
            if (currentTab_ == all)
            {
                chatMembersModel_->updateInfo(info_, true);
                if (chatMembersModel_->isShortView_)
                {
                    chatMembersModel_->isShortView_ = false;
                    chatMembersModel_->loadAllMembers(currentAimId_, info_->MembersCount_);
                }

                if (Logic::getContactListModel()->isLiveChat(currentAimId_))
                    delegate_->setRegim((chatMembersModel_->isAdmin() || chatMembersModel_->isModer()) ? Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST);
                else if (Logic::getContactListModel()->isChat(currentAimId_))
                    delegate_->setRegim(Logic::DELETE_MEMBERS);
                else
                    delegate_->setRegim(Logic::CONTACT_LIST);
            }

            if (info_->YourRole_ == "admin" || info_->Creator_ == MyInfo()->aimId())
            {
                privacyButton_->show();
                if (info_->Live_)
                {
                    publicButton_->show();
                    publicCheckBox_->show();
                    publicAbout_->show();
                    publicCheckBox_->blockSignals(true);
                    publicCheckBox_->setChecked(info_->Public_);
                    publicCheckBox_->blockSignals(false);
                    publicBottomSpace_->setFixedHeight(Utils::scale_value(public_bottom_margin));
                }
                else
                {
                    publicBottomSpace_->setFixedHeight(Utils::scale_value(approved_top_space));
                }

                approvedCheckBox_->blockSignals(true);
                approvedCheckBox_->setChecked(info_->ApprovedJoin_);
                approvedCheckBox_->blockSignals(false);
            }

            if (isLiveChat)
            {
                QString link = "https://icq.com/chat/" + info_->Stamp_;
                if (QApplication::clipboard()->text() == link)
                    copyLink_->setLink(QT_TRANSLATE_NOOP("sidebar", "link copied"), CommonStyle::getLinkColor());
                else
                    copyLink_->setLink(link, QColor(0x97, 0x97, 0x97));
            }

            if (currentTab_ == admins)
                chatMembersModel_->adminsOnly();

            Logic::setChatMembersModel(chatMembersModel_);

            emit updateMembers();
            updateWidth();
            Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        }
    }

    void MenuPage::chatBlocked(QList<Data::ChatMemberInfo>)
    {
        Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
    }

    void MenuPage::chatPending(QList<Data::ChatMemberInfo> info)
    {
        Logic::getCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All (") + QVariant(info.size()).toString() + ")");
    }

    void MenuPage::notificationsChecked(int state)
    {
        Logic::getRecentsModel()->muteChat(currentAimId_, state == Qt::Unchecked);
        GetDispatcher()->post_stats_to_core(state == Qt::Unchecked ? core::stats::stats_event_names::mute_sidebar : core::stats::stats_event_names::unmute);
    }


    void MenuPage::publicChanged(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("public", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core("chats/mod/public", collection.get());
    }

    void MenuPage::approvedChanged(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("approved", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core("chats/mod/join", collection.get());
    }

    void MenuPage::addContactClicked()
    {
        Logic::getContactListModel()->addContactToCL(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_sidebar);
    }

    void MenuPage::spamClicked()
    {
        if (Logic::getContactListModel()->blockAndSpamContact(currentAimId_))
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_sidebar);
        }
    }
}