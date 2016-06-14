#include "stdafx.h"
#include "MenuPage.h"
#include "SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../history_control/MessagesModel.h"
#include "../contact_list/AbstractSearchModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactListItemDelegate.h"
#include "../contact_list/ContactListItemRenderer.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/SearchWidget.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LabelEx.h"
#include "../../controls/ContextMenu.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../../my_info.h"

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
    const int button_height = 44;
    const int preload_members = 20;
    const int item_height = 44;
    const int members_left_margin = 8;
    const int min_visible_memebres = 5;
    const int labels_left_margin = 68;
    const int members_bottom_margin = 10;
    const int labels_spacing = 4;
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
    const int line_vertical_margin = 6;

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
        ::ContactList::RenderServiceContact(*painter_, false, false, false, QT_TRANSLATE_NOOP("sidebar","Add to chat"), Data::ContactType::ADD_CONTACT, Utils::scale_value(members_left_margin));
        if (Hovered_)
        {            QPainter p(this);
            painter_->fillRect(rect(), QColor(QColor(220, 220, 220, 0.4 * 255)));
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
        connect(this, SIGNAL(needUpdate()), this, SLOT(updateSize()), Qt::QueuedConnection);
    }

    void MenuPage::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(_e);
        p.fillRect(rect(), QColor(255, 255, 255, 0.95 * 255));
        p.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        p.drawLine(contentsRect().topLeft(), contentsRect().bottomLeft());
    }

    void MenuPage::resizeEvent(QResizeEvent* e)
    {
        QWidget::resizeEvent(e);
        updateSize();
    }

    void MenuPage::updateWidth()
    {
        name_->adjustHeight(width_ - Utils::scale_value(avatar_size + left_margin + text_margin + right_margin));
        firstLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        secondLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        thirdLine_->setLineWidth(width_ - Utils::scale_value(left_margin + avatar_size / 2 + right_margin));
        searchWidget_->setFixedWidth(width_ + Utils::scale_value(search_margin_right));
        notificationsButton_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + checkbox_width - button_offset));
        delegate_->setItemWidth(width_);
        delegate_->setLeftMargin(Utils::scale_value(members_left_margin));
        delegate_->setRightMargin(Utils::scale_value(right_margin - more_left_margin));
        addContact_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin));
        description_->adjustHeight(width_ - Utils::scale_value(avatar_size + left_margin + text_margin + right_margin));
        allMembersLabel_->setFixedWidth(width_ - Utils::scale_value(labels_left_margin + right_margin - more_left_margin) - allMembersCount_->width());
        blockLabel_->setFixedWidth(width_ - Utils::scale_value(labels_left_margin + right_margin - more_left_margin) - blockCount_->width());

        avatarName_->setFixedWidth(width_);
        addToChat_->setFixedWidth(width_);
        favoriteButton_->setFixedWidth(width_);
        themesButton_->setFixedWidth(width_);
        eraseHistoryButton_->setFixedWidth(width_);
        ignoreButton_->setFixedWidth(width_);
        quitAndDeleteButton_->setFixedWidth(width_);
        spamButton_->setFixedWidth(width_);
        admins_->setFixedWidth(width_);
        allMembers_->setFixedWidth(width_);
        blockList_->setFixedWidth(width_);
    }

    void MenuPage::updateSize()
    {
        if (!members_->isVisible())
            return;

        const int freeSpace = bottomSpacer_->geometry().height();
        const int scaledHeight = Utils::scale_value(item_height);
        int visibleMembers = members_->height() / scaledHeight;
        int modelCount = chatMembersModel_->rowCount();

        if (freeSpace > scaledHeight || visibleMembers < min_visible_memebres)
        {
            if (visibleMembers == modelCount)
            {
                allMembers_->hide();
                return;
            }

            if (modelCount > visibleMembers)
            {
                if (visibleMembers < min_visible_memebres)
                {
                    if (modelCount >= min_visible_memebres)
                        members_->setFixedHeight(min_visible_memebres * scaledHeight);
                    else
                        members_->setFixedHeight(modelCount * scaledHeight);
                    members_->update();
                    emit needUpdate();
                    return;
                }

                int addSpace = std::min(freeSpace / scaledHeight, modelCount - visibleMembers) * scaledHeight;
                members_->setFixedHeight(members_->height() + addSpace);
            }
        }
        else if (freeSpace == 0)
        {
            if (visibleMembers <= min_visible_memebres)
                return;

            members_->setFixedHeight(members_->height() - scaledHeight);
            update();
        }

        visibleMembers = members_->height() / scaledHeight;
        allMembers_->setVisible(visibleMembers < modelCount);
        members_->update();
    }

    void MenuPage::scrollChaged(int, int)
    {
        updateSize();
    }

    void MenuPage::initFor(const QString& aimId)
    {
        currentAimId_ = aimId;
        currentTab_ = all;
        avatar_->UpdateParams(currentAimId_, Logic::GetContactListModel()->getDisplayName(currentAimId_));

        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(Logic::GetContactListModel()->getDisplayName(currentAimId_), cursorName, Logic::Text2DocHtmlMode::Pass, false);

        bool haveAbout = false;
        moreLabel_->hide();

        bool isFavorite = Logic::GetRecentsModel()->isFavorite(currentAimId_);
        favoriteButton_->setImage(isFavorite ? ":/resources/sidebar_unfavorite_100.png" : ":/resources/sidebar_favorite_100.png");
        favoriteButton_->setText(isFavorite ? QT_TRANSLATE_NOOP("sidebar", "Remove from favorites") : QT_TRANSLATE_NOOP("sidebar", "Add to favorites"));

        bool isMuted = Logic::GetContactListModel()->isMuted(currentAimId_);
        notificationsCheckbox_->blockSignals(true);
        notificationsCheckbox_->setChecked(!isMuted);
        notificationsCheckbox_->blockSignals(false);
        notificationsCheckbox_->adjustSize();
        bool isLiveChat = Logic::GetContactListModel()->isLiveChat(currentAimId_);
        delegate_->setRenderRole(isLiveChat);
        info_.reset();
        bool isChat = Logic::GetContactListModel()->isChat(currentAimId_);
        if (isChat)
        {
            chatMembersModel_->is_short_view_ = true;
            chatMembersModel_->load_all_members(currentAimId_, preload_members);
            delegate_->setRegim((isLiveChat && (chatMembersModel_->is_admin() || chatMembersModel_->is_moder())) ? Logic::ADMIN_MEMBERS : Logic::DELETE_MEMBERS);
        }
        else
        {
            chatMembersModel_->init_for_single(currentAimId_);
            delegate_->setRegim(Logic::CONTACT_LIST);
        }

        quitAndDeleteButton_->setVisible(isChat);
        labelsSpacer_->setVisible(isChat);

        allMembers_->setVisible(isChat);
        admins_->setVisible(isLiveChat);
        adminsSpacer_->setVisible(isLiveChat);
        blockList_->setVisible(isLiveChat);
        blockSpacer_->setVisible(isLiveChat);
        description_->setVisible(isLiveChat && haveAbout);

        bool isNotAuth = Logic::GetContactListModel()->isNotAuth(currentAimId_);
        bool isIgnored = Logic::GetIgnoreModel()->getMemberItem(currentAimId_) != 0;
        addContact_->setVisible(isNotAuth);
        addContactSpacer_->setVisible(isNotAuth);
        addContactSpacerTop_->setVisible(isNotAuth);
        spamButton_->setVisible(isNotAuth);
        addToChat_->setVisible(!isIgnored);
        thirdLine_->setVisible(!isIgnored);
        members_->setVisible(!isIgnored);
        members_->setFixedHeight(0);
        favoriteButton_->setVisible(!isNotAuth);
        notificationsCheckbox_->setVisible(!isNotAuth);
        notificationsButton_->setVisible(!isNotAuth);
        themesButton_->setVisible(!isNotAuth);
        secondLine_->setVisible(!isNotAuth);
        textTopSpace_->setVisible(!isLiveChat);

        stackedWidget_->setCurrentIndex(main);

        updateSize();
        updateWidth();
    }

    void MenuPage::init()
    {
        stackedWidget_ = new QStackedWidget(this);
        auto layout = emptyVLayout(this);
        layout->addWidget(stackedWidget_);
        auto  area = new QScrollArea(stackedWidget_);
        stackedWidget_->insertWidget(main, area);
        area->horizontalScrollBar()->setDisabled(true);

        mainWidget_ = new QWidget(area);
        mainWidget_->setStyleSheet(transparent_background);

        area->setContentsMargins(0, 0, 0, 0);
        area->setWidget(mainWidget_);
        area->setWidgetResizable(true);
        area->setFrameStyle(QFrame::NoFrame);
        area->setStyleSheet(transparent_background);

        connect(area->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(scrollChaged(int, int)), Qt::QueuedConnection);

        setStyleSheet(Utils::LoadStyle(":/main_window/sidebar/Sidebar.qss", Utils::get_scale_coefficient(), true));
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
    }

    void MenuPage::initAvatarAndName()
    {
        rootLayot_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            avatarName_ = new ClickedWidget(mainWidget_);
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
                auto vLayout = emptyVLayout();
                vLayout->setAlignment(Qt::AlignTop);
                vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(text_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                textTopSpace_ = new QWidget(mainWidget_);
                textTopSpace_->setFixedHeight(Utils::scale_value(text_top_spacing));
                vLayout->addWidget(textTopSpace_);
                name_ = new TextEditEx(avatarName_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(20), QColor("#282828"), false, false);
                name_->setStyleSheet(transparent_background);
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setAttribute(Qt::WA_TransparentForMouseEvents);
                vLayout->addWidget(name_);

                description_ = new TextEditEx(avatarName_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor("#696969"), false, false);
                description_->setStyleSheet(transparent_background);
                description_->setFrameStyle(QFrame::NoFrame);
                description_->setContentsMargins(0, 0, 0, 0);
                description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setAttribute(Qt::WA_TransparentForMouseEvents);

                vLayout->addWidget(description_);
                horLayout->addLayout(vLayout);
            }
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            avatarName_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayot_->addWidget(avatarName_);
            {
                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin + text_margin + avatar_size), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                moreLabel_ = new LabelEx(mainWidget_);
                moreLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
                QPalette p;
                p.setColor(QPalette::Foreground, QColor("#579e1c"));
                moreLabel_->setPalette(p);
                moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "More"));
                moreLabel_->setContentsMargins(Utils::scale_value(more_left_margin), 0, 0, 0);
                moreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(moreLabel_);
                rootLayot_->addLayout(hLayout);
            }
        }
    }

    void MenuPage::initAddContactAndSpam()
    {

        addContactSpacerTop_ = new QWidget(mainWidget_);
        addContactSpacerTop_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        rootLayot_->addWidget(addContactSpacerTop_);
        {
            auto horLayout = emptyHLayout();
            horLayout->setAlignment(Qt::AlignLeft);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            addContact_ = new CustomButton(mainWidget_, QString());
            addContact_->setText(QT_TRANSLATE_NOOP("sidebar", "Add contact"));
            addContact_->setAlign(Qt::AlignHCenter);
            addContact_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::ApplyStyle(addContact_, Ui::main_button_style);
            horLayout->addWidget(addContact_);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            rootLayot_->addLayout(horLayout);
        }

        addContactSpacer_ = new QWidget(mainWidget_);
        addContactSpacer_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        rootLayot_->addWidget(addContactSpacer_);

        spamButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_spam_100.png", QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        spamButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(spamButton_);

        firstLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
        rootLayot_->addWidget(firstLine_);
    }

    void MenuPage::initFavoriteNotificationsTheme()
    {
        favoriteButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_favorite_100.png", QString(), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        favoriteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(favoriteButton_);
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
            horLayout->addWidget(notificationsButton_);
            notificationsCheckbox_ = new QCheckBox(mainWidget_);
            notificationsCheckbox_->adjustSize();
            notificationsCheckbox_->setCursor(QCursor(Qt::PointingHandCursor));
            notificationsCheckbox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
            horLayout->addWidget(notificationsCheckbox_);
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            rootLayot_->addLayout(horLayout);
        }

        themesButton_ = new ActionButton(mainWidget_, ":/resources/contr_themes_100.png", QT_TRANSLATE_NOOP("sidebar", "Wallpaper"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        themesButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(themesButton_);

        secondLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));        
        rootLayot_->addWidget(secondLine_);
    }

    void MenuPage::initChatMembers()
    {
        chatMembersModel_ = new Logic::ChatMembersModel(mainWidget_);
        chatMembersModel_->setSelectEnabled(false);
        chatMembersModel_->setFlag(Logic::HasMouseOver);
        GetCurrentSearchModel(Logic::DELETE_MEMBERS)->setSelectEnabled(false);
        delegate_ = new Logic::ContactListItemDelegate(mainWidget_, Logic::DELETE_MEMBERS);
        {
            auto horLayout = emptyHLayout();
            {
                auto verLayout = emptyVLayout();
                addToChat_ = new AddToChat(mainWidget_);
                addToChat_->setFixedHeight(Utils::scale_value(item_height));
                addToChat_->setCursor(QCursor(Qt::PointingHandCursor));
                verLayout->addWidget(addToChat_);
                members_ = new FocusableListView(mainWidget_);
                members_->setMouseTracking(true);
                members_->setModel(chatMembersModel_);
                members_->setItemDelegate(delegate_);
                members_->setFrameStyle(QFrame::NoFrame);
                members_->setFixedHeight(0);
                members_->setStyleSheet(transparent_background);
                members_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                members_->verticalScrollBar()->setDisabled(true);
                members_->setCursor(QCursor(Qt::PointingHandCursor));
                members_->setFocusPolicy(Qt::NoFocus);
                verLayout->addWidget(members_);
                verLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(members_bottom_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                {
                    QFont f = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16));
                    QPalette p;
                    p.setColor(QPalette::Foreground, QColor("#282828"));

                    admins_ = new ClickedWidget(mainWidget_);
                    admins_->setFixedHeight(Utils::scale_value(button_height));
                    admins_->setCursor(QCursor(Qt::PointingHandCursor));
                    auto horLayout2 = emptyHLayout(admins_);
                    horLayout2->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    auto adminsLabel = new QLabel(admins_);
                    adminsLabel->setFont(f);
                    adminsLabel->setPalette(p);
                    adminsLabel->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
                    horLayout2->addWidget(adminsLabel);
                    horLayout2->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    verLayout->addWidget(admins_);

                    adminsSpacer_ = new QWidget(mainWidget_);
                    adminsSpacer_->setFixedSize(1, Utils::scale_value(labels_spacing));
                    verLayout->addWidget(adminsSpacer_);

                    allMembers_ = new ClickedWidget(mainWidget_);
                    allMembers_->setCursor(QCursor(Qt::PointingHandCursor));
                    allMembers_->setFixedHeight(Utils::scale_value(button_height));
                    auto horLayout3 = emptyHLayout(allMembers_);
                    horLayout3->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    allMembersLabel_ = new QLabel(allMembers_);
                    allMembersLabel_->setFont(f);
                    allMembersLabel_->setPalette(p);
                    allMembersLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "All members"));
                    horLayout3->addWidget(allMembersLabel_);
                    allMembersCount_ = new QLabel(allMembers_);
                    allMembersCount_->setPalette(p);
                    allMembersCount_->setFont(f);
                    horLayout3->addWidget(allMembersCount_);
                    horLayout3->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    horLayout3->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    verLayout->addWidget(allMembers_);

                    blockSpacer_ = new QWidget(mainWidget_);
                    blockSpacer_->setFixedSize(1, Utils::scale_value(labels_spacing));
                    verLayout->addWidget(blockSpacer_);

                    blockList_ = new ClickedWidget(mainWidget_);
                    blockList_->setCursor(QCursor(Qt::PointingHandCursor));
                    blockList_->setFixedHeight(Utils::scale_value(button_height));
                    auto horLayout4 = emptyHLayout(blockList_);
                    horLayout4->addSpacerItem(new QSpacerItem(Utils::scale_value(labels_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    blockLabel_ = new QLabel(blockList_);
                    blockLabel_->setFont(f);
                    blockLabel_->setPalette(p);
                    blockLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
                    horLayout4->addWidget(blockLabel_);
                    blockCount_ = new QLabel(blockList_);
                    blockCount_->setPalette(p);
                    blockCount_->setFont(f);
                    horLayout4->addWidget(blockCount_);
                    horLayout4->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                    horLayout4->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                    verLayout->addWidget(blockList_);

                    labelsSpacer_ = new QWidget(mainWidget_);
                    labelsSpacer_->setFixedSize(1, Utils::scale_value(labels_bottom_margin));
                    verLayout->addWidget(labelsSpacer_);
                }
                horLayout->addLayout(verLayout);
            }
            rootLayot_->addLayout(horLayout);
        }

        thirdLine_ = new LineWidget(mainWidget_, Utils::scale_value(labels_left_margin), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
        rootLayot_->addWidget(thirdLine_);
    }

    void MenuPage::initEraseIgnoreDelete()
    {
        eraseHistoryButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_closechat_100.png", QT_TRANSLATE_NOOP("sidebar", "Clear history"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        eraseHistoryButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(eraseHistoryButton_);

        ignoreButton_ = new ActionButton(mainWidget_, ":/resources/sidebar_ignore_100.png", QT_TRANSLATE_NOOP("sidebar", "Ignore"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        ignoreButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(ignoreButton_);

        quitAndDeleteButton_ = new ActionButton(mainWidget_, ":/resources/contr_signout_100.png", QT_TRANSLATE_NOOP("sidebar", "Leave and delete"), Utils::scale_value(button_height), 0, Utils::scale_value(button_offset));
        quitAndDeleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        rootLayot_->addWidget(quitAndDeleteButton_);

        bottomSpacer_ = new QSpacerItem(0, QWIDGETSIZE_MAX, QSizePolicy::Preferred, QSizePolicy::Expanding);
        rootLayot_->addSpacerItem(bottomSpacer_);
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
            listLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(20)));
            QPalette p;
            p.setColor(QPalette::Foreground, QColor("#282828"));
            listLabel_->setPalette(p);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "All members"));
            horLayout->addWidget(listLabel_);
            horLayout->addSpacerItem(new QSpacerItem(1, 0, QSizePolicy::Expanding));
            vLayout->addLayout(horLayout);

            {
                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(list_left_margin), 0, QSizePolicy::Fixed));
                searchWidget_ = new SearchWidget(false, listWidget_);
                searchWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                hLayout->addWidget(searchWidget_);
                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                vLayout->addLayout(hLayout);
                searchWidget_->SetShowButton(false);
                searchWidget_->setTransparent(true);
            }
            cl_ = new Ui::ContactList(listWidget_, Logic::DELETE_MEMBERS, chatMembersModel_);
            cl_->changeTab(SEARCH);
            cl_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            cl_->setContentsMargins(0, 0, 0, 0);
            cl_->setClDelegate(delegate_);
            cl_->setTransparent(true);

            vLayout->addWidget(cl_);
        }
    }

    void MenuPage::connectSignals()
    {
        connect(favoriteButton_, SIGNAL(clicked()), this, SLOT(favoritesClicked()), Qt::QueuedConnection);
        connect(themesButton_, SIGNAL(clicked()), this, SLOT(themesClicked()), Qt::QueuedConnection);
        connect(eraseHistoryButton_, SIGNAL(clicked()), this, SLOT(eraseHistoryClicked()), Qt::QueuedConnection);
        connect(ignoreButton_, SIGNAL(clicked()), this, SLOT(ignoreClicked()), Qt::QueuedConnection);
        connect(notificationsCheckbox_, SIGNAL(stateChanged(int)), this, SLOT(notificationsChecked(int)), Qt::QueuedConnection);
        connect(quitAndDeleteButton_, SIGNAL(clicked()), this, SLOT(quitClicked()), Qt::QueuedConnection);
        connect(addToChat_, SIGNAL(clicked()), this, SLOT(addToChatClicked()), Qt::QueuedConnection);
        connect(addContact_, SIGNAL(clicked()), this, SLOT(addContactClicked()), Qt::QueuedConnection);
        connect(spamButton_, SIGNAL(clicked()), this, SLOT(spamClicked()), Qt::QueuedConnection);

        connect(avatarName_, SIGNAL(clicked()), this, SLOT(avatarClicked()), Qt::QueuedConnection);

        connect(allMembers_, SIGNAL(clicked()), this, SLOT(allMemebersClicked()), Qt::QueuedConnection);
        connect(backButton_, SIGNAL(clicked()), this, SLOT(backButtonClicked()), Qt::QueuedConnection);
        connect(members_, SIGNAL(pressed(const QModelIndex &)), SLOT(membersClicked(const QModelIndex &)), Qt::QueuedConnection);

        connect(Logic::GetRecentsModel(), SIGNAL(favoriteChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Logic::GetContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(chatMembersModel_, SIGNAL(results()), this, SLOT(updateSize()), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatBlocked(QList<Data::ChatMemberInfo>)), this, SLOT(chatBlocked(QList<Data::ChatMemberInfo>)), Qt::QueuedConnection);
        connect(cl_, SIGNAL(itemClicked(QString)), this, SLOT(contactClicked(QString)), Qt::QueuedConnection);

        connect(cl_, SIGNAL(searchEnd()), searchWidget_, SLOT(searchCompleted()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchBegin()), this,  SLOT(searchBegin()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchEnd()), this,  SLOT(searchEnd()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(enterPressed()), cl_, SLOT(searchResult()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(upPressed()), cl_, SLOT(searchUpPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(downPressed()), cl_, SLOT(searchDownPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(moreLabel_, SIGNAL(clicked()), this, SLOT(moreClicked()), Qt::QueuedConnection);
        connect(admins_, SIGNAL(clicked()), this, SLOT(adminsClicked()), Qt::QueuedConnection);
        connect(blockList_, SIGNAL(clicked()), this, SLOT(blockedClicked()), Qt::QueuedConnection);
        connect(Logic::GetMessagesModel(), SIGNAL(chatEvent(QString)), this, SLOT(chatEvent(QString)), Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), SIGNAL(set_chat_role_result(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(block_member_result(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
    }

    void MenuPage::initDescription(const QString& description, bool full)
    {
        description_->setPlainText(QString());
        QTextCursor cursorDesc = description_->textCursor();
        Logic::Text2Doc(description, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);
        moreLabel_->hide();
        if (!full)
        {
            if (description.length() > desc_length)
            {
                QString newDescription = description.left(desc_length);
                newDescription += "...";

                description_->setPlainText(QString());
                cursorDesc = description_->textCursor();
                Logic::Text2Doc(newDescription, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);

                moreLabel_->show();
            }
        }

        description_->setVisible(!description.isEmpty());
        textTopSpace_->setVisible(description.isEmpty());
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
                chatMembersModel_->load_blocked();
        }
    }

    void MenuPage::actionResult(int)
    {
        if (currentTab_ == block)
            chatMembersModel_->load_blocked();
        else
            chatMembersModel_->load_all_members();
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
        bool advMenu = (Logic::GetContactListModel()->isLiveChat(currentAimId_) && (chatMembersModel_->is_admin() || chatMembersModel_->is_moder()));
        if (Logic::GetContactListModel()->isChat(currentAimId_) && !Logic::GetContactListModel()->isLiveChat(currentAimId_))
            advMenu = true;
        if (advMenu && global_cursor_pos.x() > minXofDeleteImage && global_cursor_pos.x() <= (minXofDeleteImage + Utils::scale_value(20)) && chatMembersModel_)
        {
            if (currentTab_ == all && Logic::GetContactListModel()->isLiveChat(currentAimId_) && (chatMembersModel_->is_moder() || chatMembersModel_->is_admin()))
            {
                auto menu = new ContextMenu(mainWidget_);
                auto cont = chatMembersModel_->getMemberItem(aimId);
                bool myInfo = cont->AimId_ == MyInfo()->aimId();
                if (cont->Role_ != "admin" && chatMembersModel_->is_admin())
                {
                    if (cont->Role_ == "moder")
                        menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_removeking_100.png")), QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"), makeData("revoke_admin", aimId));
                    else
                        menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_king_100.png")), QT_TRANSLATE_NOOP("sidebar", "Make admin"), makeData("make_admin", aimId));
                }
                if (!myInfo)
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_profile_100.png")), QT_TRANSLATE_NOOP("sidebar", "Profile"), makeData("profile", aimId));
                if ((cont->Role_ != "admin" && cont->Role_ != "moder") || myInfo)
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("sidebar", "Delete from chat"), makeData("remove", aimId));
                if (cont->Role_ != "admin" && cont->Role_ != "moder")
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_block_100.png")), QT_TRANSLATE_NOOP("sidebar", "Block"), makeData("block", aimId));
                if (!myInfo)
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_spam_100.png")), QT_TRANSLATE_NOOP("sidebar", "Report spam"), makeData("spam", aimId));
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
            else
            {
                deleteMemberDialog(chatMembersModel_, aimId, Logic::DELETE_MEMBERS, this);
            }
        }
        else
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }
    }

    void MenuPage::membersClicked(const QModelIndex & current)
    {
        if (Data::ChatMemberInfo* info = current.data().value<Data::ChatMemberInfo*>())
            contactClicked(info->AimId_);
    }

    void MenuPage::moreClicked()
    {
        initDescription(info_->About_, true);
    }

    void MenuPage::adminsClicked()
    {
        chatMembersModel_->admins_only();
        Logic::SetChatMembersModel(chatMembersModel_);
        if (Logic::GetContactListModel()->isLiveChat(currentAimId_) && (chatMembersModel_->is_admin()))
            delegate_->setRegim(Logic::DELETE_MEMBERS);
        else
            delegate_->setRegim(Logic::CONTACT_LIST);
        Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
        cl_->changeTab(SEARCH);
        stackedWidget_->setCurrentIndex(list);
        currentTab_ = admins;
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_admins);
    }

    void MenuPage::blockedClicked()
    {
        chatMembersModel_->load_blocked();
        Logic::SetChatMembersModel(chatMembersModel_);
        if (Logic::GetContactListModel()->isLiveChat(currentAimId_) && (chatMembersModel_->is_admin() || chatMembersModel_->is_moder()))
            delegate_->setRegim(Logic::DELETE_MEMBERS);
        else
            delegate_->setRegim(Logic::CONTACT_LIST);
        Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
        cl_->changeTab(SEARCH);
        stackedWidget_->setCurrentIndex(list);
        currentTab_ = block;
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_blocked);
    }

    void MenuPage::avatarClicked()
    {
        Utils::InterConnector::instance().showSidebar(currentAimId_, profile_page);
    }

    void MenuPage::chatEvent(QString aimId)
    {
        if (aimId == currentAimId_)
            chatMembersModel_->load_all_members();
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
            if (Logic::GetContactListModel()->block_spam_contact(aimId))
            {
                Logic::GetContactListModel()->remove_contact_from_contact_list(aimId);
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
        Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
    }

    void MenuPage::backButtonClicked()
    {
        chatMembersModel_->load_all_members();
        Logic::SetChatMembersModel(NULL);
        stackedWidget_->setCurrentIndex(main);
        currentTab_ = all;
    }

    void MenuPage::allMemebersClicked()
    {
        chatMembersModel_->load_all_members();
        Logic::SetChatMembersModel(chatMembersModel_);
        if (Logic::GetContactListModel()->isLiveChat(currentAimId_))
            delegate_->setRegim((chatMembersModel_->is_admin() || chatMembersModel_->is_moder()) ? Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST);
        else if (Logic::GetContactListModel()->isChat(currentAimId_))
            delegate_->setRegim(Logic::DELETE_MEMBERS);
        else
            delegate_->setRegim(Logic::CONTACT_LIST);

        Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "All members"));
        cl_->changeTab(SEARCH);
        stackedWidget_->setCurrentIndex(list);
        currentTab_ = all;
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_viewall);
    }

    void MenuPage::favoritesClicked()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", currentAimId_);
        Ui::GetDispatcher()->post_message_to_core(Logic::GetRecentsModel()->isFavorite(currentAimId_) ? "unfavorite" : "favorite", collection.get());
    }

    void MenuPage::themesClicked()
    {
        emit Utils::InterConnector::instance().themesSettingsShow(true, currentAimId_);
    }

    void MenuPage::eraseHistoryClicked()
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
            Logic::GetContactListModel()->getDisplayName(currentAimId_),
            nullptr);

        if (!confirmed)
        {
            return;
        }

        const auto lastMessageId = Logic::GetMessagesModel()->getLastMessageId(currentAimId_);
        if (lastMessageId > 0)
        {
            GetDispatcher()->delete_messages_from(currentAimId_, lastMessageId);
            Utils::InterConnector::instance().setSidebarVisible(false);
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_delete);
    }

    void MenuPage::ignoreClicked()
    {
        if (Logic::GetContactListModel()->ignore_and_remove_from_cl_contact(currentAimId_))
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
            Logic::GetContactListModel()->getDisplayName(currentAimId_),
            NULL);
        if (confirmed)
        {
            Logic::GetContactListModel()->remove_contact_from_contact_list(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_sidebar);
        }
    }

    void MenuPage::addToChatClicked()
    {
        if (!Logic::GetContactListModel()->isChat(currentAimId_))
        {
            QStringList list;
            list.append(currentAimId_);
            createGroupChat(list);
            return;
        }

        Logic::SetChatMembersModel(chatMembersModel_);

        if (!chatMembersModel_->is_full_list_loaded_)
        {
            chatMembersModel_->load_all_members();
        }

        SelectContactsWidget select_members_dialog(NULL, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("sidebar", "Add to chat"), QT_TRANSLATE_NOOP("groupchat_pages", "Done"), this);
        connect(this, SIGNAL(updateMembers()), &select_members_dialog, SLOT(updateMembers()), Qt::QueuedConnection);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_add_member_sidebar);
        }
        else
        {
            Logic::GetContactListModel()->clearChecked();
        }
        Logic::SetChatMembersModel(NULL);
    }

    void MenuPage::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> info)
    {
        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            allMembersCount_->setText(QVariant(info_->MembersCount_).toString());
            int visibleMembers = members_->height() / Utils::scale_value(item_height);
            allMembers_->setVisible(visibleMembers < info_->MembersCount_);

            initDescription(info_->About_);

            int blockedCount = info_->BlockedCount_;
            bool isLiveChat = Logic::GetContactListModel()->isLiveChat(currentAimId_);
            blockCount_->setText(QVariant(blockedCount).toString());
            blockList_->setVisible(blockedCount != 0 && isLiveChat);
            blockSpacer_->setVisible(blockedCount != 0 && isLiveChat);
            adminsSpacer_->setVisible(admins_->isVisible() && (allMembers_->isVisible() || blockList_->isVisible()));
            chatMembersModel_->update_info(info_, true);
            if (chatMembersModel_->is_short_view_)
            {
                chatMembersModel_->is_short_view_ = false;
                chatMembersModel_->load_all_members(currentAimId_, info_->MembersCount_);
            }
            members_->setFixedHeight(0);
            members_->update();
            if (Logic::GetContactListModel()->isLiveChat(currentAimId_))
                delegate_->setRegim((chatMembersModel_->is_admin() || chatMembersModel_->is_moder()) ? Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST);
            else if (Logic::GetContactListModel()->isChat(currentAimId_))
                delegate_->setRegim(Logic::DELETE_MEMBERS);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);

            labelsSpacer_->setVisible(allMembers_->isVisible() || blockList_->isVisible() || admins_->isVisible());

            if (currentTab_ == admins)
                chatMembersModel_->admins_only();

            Logic::SetChatMembersModel(chatMembersModel_);

            emit updateMembers();
            updateSize();
            updateWidth();
            Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
        }
    }

    void MenuPage::chatBlocked(QList<Data::ChatMemberInfo>)
    {
        Logic::GetCurrentSearchModel(Logic::DELETE_MEMBERS)->searchPatternChanged("");
    }

    void MenuPage::notificationsChecked(int state)
    {
        Logic::GetRecentsModel()->muteChat(currentAimId_, state == Qt::Unchecked);
        GetDispatcher()->post_stats_to_core(state == Qt::Unchecked ? core::stats::stats_event_names::mute_sidebar : core::stats::stats_event_names::unmute);
    }

    void MenuPage::addContactClicked()
    {
        Logic::GetContactListModel()->add_contact_to_contact_list(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_sidebar);
    }

    void MenuPage::spamClicked()
    {
        if (Logic::GetContactListModel()->block_spam_contact(currentAimId_))
        {
            Logic::GetContactListModel()->remove_contact_from_contact_list(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_sidebar);
        }
    }
}