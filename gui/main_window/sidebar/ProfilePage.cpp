#include "stdafx.h"
#include "ProfilePage.h"
#include "SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/SearchMembersModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../livechats/LiveChatMembersControl.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../my_info.h"
#include "../MainPage.h"

namespace
{
    const int top_margin = 16;
    const int avatar_size = 180;
    const int left_margin = 24;
    const int button_size = 40;
    const int big_button_size = 48;
    const int back_button_spacing = 10;
    const int text_top_margin = 21;
    const int more_left_margin = 4;
    const int right_margin = 48;
    const int more_right_margin = 24;
    const int desc_length = 100;
    const int members_count = 5;
    const int ignore_margins = 16;
    const int ignore_spacing = 7;
    const int buttons_spacing = 16;
    const int buttons_top_margin = 24;
    const int buttons_margin = 16;
    const int button_bottom_space = 16;
    const int button_height = 44;
    const int button_offset = 28;
    const int reverse_margin = -20;
    const int line_vertical_margin = 6;
    const int avatar_botton_spacing = 10;
    const int members_space = 10;
    const int name_margin = 18;
    const int edit_top_margin = 14;
    const int checkbox_width = 44;
    const int checkbox_height = 24;

    const static QString transparent_background = "background: transparent;";
}

namespace Ui
{
    InfoPlate::InfoPlate(QWidget* parent, int leftMargin)
        : QWidget(parent)
        , attachPhone_(false)
    {
        auto rootLayout = emptyHLayout(this);
        rootLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
        {
            auto vLayout = emptyVLayout();
            header_ = new QLabel(this);
            QPalette p;
            p.setColor(QPalette::Foreground, QColor("#696969"));
            header_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
            header_->setPalette(p);
            vLayout->addWidget(header_);
            info_ = new TextEmojiWidget(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(18), QColor("#000000"));
            connect(info_, SIGNAL(rightClicked()), this, SLOT(menuRequested()), Qt::QueuedConnection);
            info_->set_selectable(true);
            vLayout->addWidget(info_);
            phoneInfo_ = new QLabel(this);
            phoneInfo_->setWordWrap(true);
            phoneInfo_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
            phoneInfo_->setPalette(p);
            vLayout->addWidget(phoneInfo_);
            phoneInfo_->hide();
            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(button_bottom_space), QSizePolicy::Preferred, QSizePolicy::Fixed));
            rootLayout->addLayout(vLayout);
        }
    }

    void InfoPlate::menuRequested()
    {
        ContextMenu* menu = new ContextMenu(this);
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_copy_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy"), QVariant());
        connect(menu, &ContextMenu::triggered, this, &InfoPlate::menu, Qt::QueuedConnection);
        menu->popup(QCursor::pos());
    }

    void InfoPlate::menu(QAction*)
    {
        QApplication::clipboard()->setText(info_->text());
    }

    void InfoPlate::setAttachPhone(bool value)
    {
        attachPhone_ = value;
        info_->set_selectable(!value);
        if (value)
        {
            info_->setText(QT_TRANSLATE_NOOP("sidebar", "Attach phone"), QColor("#579e1c"));
            info_->adjustSize();
            connect(info_, &TextEmojiWidget::clicked, []()
            {
                emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachPhone);
            });
            phoneInfo_->setText(QT_TRANSLATE_NOOP("sidebar", "for safety and spam protection"));
            phoneInfo_->show();
        }
        else
        {
            setInfo(infoStr_);
            disconnect(info_, SIGNAL(clicked()));
            phoneInfo_->setText(QT_TRANSLATE_NOOP("sidebar", "Only visible for those, who has it in the phone book"));
            phoneInfo_->show();
        }
        info_->setCursor(value ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void InfoPlate::setHeader(const QString& header)
    {
        header_->setText(header);
        header_->adjustSize();
    }

    void InfoPlate::setInfo(const QString& info)
    {
        infoStr_ = info;
        info_->setText(infoStr_, QColor("#000000"));
        info_->adjustSize();
        phoneInfo_->hide();
    }

    void InfoPlate::elideText(int width)
    {
        if (attachPhone_)
            return;

        QFontMetrics m(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
        QString result = m.elidedText(infoStr_, Qt::ElideRight, width);
        info_->setText(result);
    }

    ProfilePage::ProfilePage(QWidget* parent)
        : SidebarPage(parent)
        , info_(std::make_shared<Data::ChatInfo>())
        , connectOldVisible_(false)
    {
        init();
    }

    void ProfilePage::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(_e);
        if (MyInfo()->aimId() == currentAimId_)
            p.fillRect(rect(), Qt::white);
        else
            p.fillRect(rect(), QColor(255, 255, 255, 0.95 * 255));
        p.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        p.drawLine(contentsRect().topLeft(), contentsRect().bottomLeft());
    }

    void ProfilePage::updateWidth()
    {
        firstLine_->setLineWidth(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin) - rightWidget_->width());
        secondLine_->setLineWidth(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin) - rightWidget_->width());
        if (MyInfo()->aimId() == currentAimId_ && width() >= Utils::scale_value(596))
        {
            name_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin + left_margin) - rightWidget_->width() - editLabel_->width());
            name_->setFixedWidth(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin + left_margin) - rightWidget_->width() - editLabel_->width());
        }
        else
        {
            name_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
            name_->setFixedWidth(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
        }

        if (width() >= Utils::scale_value(596))
            publicButton_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + checkbox_width + back_button_spacing - button_offset));
        else
            publicButton_->setFixedWidth(width_ - Utils::scale_value(right_margin + checkbox_width + back_button_spacing - button_offset));

        ignoreWidget_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + back_button_spacing) - rightWidget_->width());
        description_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
        uin_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        phone_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        firstName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        lastName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        nickName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));

        int diff = rightWidget_->width() == 0? Utils::scale_value(left_margin) : 0;
        ignoreButton_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        spamButton_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        deleteButton_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        quiAndDelete->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        renameButton_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        changeDescription->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        renameContact_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        ignoreListButton->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        attachOldAcc->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        avatar_->update();
    }

    void ProfilePage::initFor(const QString& aimId)
    {
        currentAimId_ = aimId.isEmpty() ? MyInfo()->aimId() : aimId;
        bool myInfo = (MyInfo()->aimId() == currentAimId_);
        
        backButton_->setVisible(!myInfo);

        avatar_->UpdateParams(currentAimId_, Logic::GetContactListModel()->getDisplayName(currentAimId_));
        avatar_->update();

        statusButton_->setProperty("self", myInfo);
        statusButton_->setStyle(QApplication::style());
        statusLabel_->setProperty("self", myInfo);
        statusLabel_->setStyle(QApplication::style());
        statusLabel_->setCursor(myInfo ? Qt::PointingHandCursor : Qt::ArrowCursor);
        statusButton_->setVisible(myInfo);

        publicButton_->hide();
        publicCheckBox_->hide();

        core::coll_helper helper(GetDispatcher()->create_collection(), true);
        GetDispatcher()->post_message_to_core("load_flags", helper.get());

        attachOldAcc->setVisible(myInfo && connectOldVisible_);

        changeDescription->hide();

        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(Logic::GetContactListModel()->getDisplayName(currentAimId_), cursorName, Logic::Text2DocHtmlMode::Pass, false);

        description_->hide();
        moreLabel_->hide();
        editLabel_->setVisible(myInfo);

        bool isChat = Logic::GetContactListModel()->isChat(currentAimId_);
        avatar_->SetIsInMyProfile(myInfo || (isChat && !Logic::GetContactListModel()->isLiveChat(currentAimId_)));
        if (isChat)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_int("limit", members_count);
            Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
        }

        updateStatus();

        bool isIgnored = Logic::GetIgnoreModel()->getMemberItem(currentAimId_) != 0;
        ignoreWidget_->setVisible(isIgnored);
        buttonWidget_->setVisible(!isIgnored && !isChat && !myInfo);
        buttonsMargin_->setVisible(!isIgnored && !isChat);

        auto cont = Logic::GetContactListModel()->getContactItem(currentAimId_);

        bool isNotAuth = Logic::GetContactListModel()->isNotAuth(currentAimId_) || cont == 0;
        chatButton_->setVisible(!isNotAuth);
        addButton_->setVisible(isNotAuth);
        callButton_->setEnabled(!isNotAuth);
        videoCall_button_->setEnabled(!isNotAuth);

        deleteButton_->setVisible(!isNotAuth && !isChat && !myInfo);
        quiAndDelete->setVisible(!isNotAuth && isChat);
        ignoreListButton->setVisible(myInfo);
        ignoreButton_->setVisible(!myInfo);
        spamButton_->setVisible(!isChat && !myInfo);
        members_->setVisible(false);
        membersTopSpace_->setVisible(false);
        membersBottomSpace_->setVisible(false);
        membersLabel_->setVisible(false);
        nameMargin_->setVisible(isChat);
        renameContact_->setVisible(!isChat && !isNotAuth && !isIgnored && !myInfo);
        firstLine_->setVisible(isChat);
        secondLine_->setVisible(!Logic::GetContactListModel()->isLiveChat(currentAimId_));

        renameButton_->setVisible(isChat && !Logic::GetContactListModel()->isLiveChat(currentAimId_));

        uin_->setInfo(currentAimId_);
        uin_->setVisible(!isChat);
        phone_->hide();
        firstName_->hide();
        lastName_->hide();
        nickName_->hide();

        if (Logic::GetContactListModel()->isChat(currentAimId_))
            return;

        Logic::GetContactListModel()->get_contact_profile(currentAimId_, [this, myInfo](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            if (!_profile)
                return;

            if (myInfo)
            {
                if (!MyInfo()->phoneNumber().isEmpty())
                    phone_->setInfo(MyInfo()->phoneNumber());

                phone_->setAttachPhone(MyInfo()->phoneNumber().isEmpty());
                phone_->show();
            }
            else
            {
                Logic::phone_list list = _profile->get_phones();
                if (!list.empty())
                {
                    phone_->setInfo(list.front().get_phone());
                    phone_->show();
                    renameContact_->hide();
                }
            }

            auto fn = _profile->get_first_name();
            if (!fn.isEmpty())
            {
                firstName_->setInfo(fn);
                firstName_->show();
            }

            auto ln = _profile->get_last_name();
            if (!ln.isEmpty())
            {
                lastName_->setInfo(ln);
                lastName_->show();
            }

            auto nn = myInfo ? MyInfo()->friendlyName() : _profile->get_friendly();
            if (!nn.isEmpty())
            {
                nickName_->setInfo(nn);
                nickName_->show();

                if (Logic::GetContactListModel()->getDisplayName(currentAimId_) == currentAimId_)
                {
                    name_->setPlainText(QString());
                    QTextCursor cursorName = name_->textCursor();
                    Logic::Text2Doc(nn, cursorName, Logic::Text2DocHtmlMode::Pass, false);
                    Logic::GetAvatarStorage()->UpdateAvatar(currentAimId_);
                    avatar_->UpdateParams(currentAimId_, nn);
                    avatar_->update();
                }
            }

            updateWidth();
        });

        updateWidth();
    }

    void ProfilePage::init()
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/sidebar/Sidebar.qss", Utils::get_scale_coefficient(), true));
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);
        connect(GetDispatcher(), SIGNAL(recvFlags(int)), this, SLOT(recvFlags(int)));

        auto layout = emptyVLayout(this);
        auto area = new QScrollArea(this);
        area->horizontalScrollBar()->setDisabled(true);
        layout->addWidget(area);

        area->setContentsMargins(0, 0, 0, 0);
        auto mainWidget = new QWidget(area);
        area->setWidget(mainWidget);
        area->setWidgetResizable(true);
        area->setFrameStyle(QFrame::NoFrame);
        area->setStyleSheet(transparent_background);

        auto vLayoutMain = emptyVLayout(mainWidget);
        vLayoutMain->addSpacerItem(new QSpacerItem(0, Utils::scale_value(top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));

        auto hLayoutMain = emptyHLayout();
        hLayoutMain->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));

        subBackButtonLayout_ = emptyVLayout();
        subBackButtonLayout_->setAlignment(Qt::AlignTop);
        hLayoutMain->addLayout(subBackButtonLayout_);

        subAvatarLayout_ = emptyVLayout();
        subAvatarLayout_->setAlignment(Qt::AlignTop);
        hLayoutMain->addLayout(subAvatarLayout_);

        auto rootLayout = emptyVLayout();
        rootLayout->setAlignment(Qt::AlignTop);
        {
            auto avatarLayout = emptyHLayout();
            {
                mainBackButtonLayout_ = emptyVLayout();
                mainBackButtonLayout_->setAlignment(Qt::AlignTop);
                backButton_ = new CustomButton(mainWidget, ":/resources/contr_back_100.png");
                backButton_->setHoverImage(":/resources/contr_back_100_hover.png");
                backButton_->setActiveImage(":/resources/contr_back_100_active.png");
                backButton_->setFixedSize(Utils::scale_value(button_size), Utils::scale_value(button_size));
                backButton_->setCursor(QCursor(Qt::PointingHandCursor));
                mainBackButtonLayout_->addWidget(backButton_);
                avatarLayout->addLayout(mainBackButtonLayout_);
            }
            avatarLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            {
                mainAvatarLayout_ = emptyVLayout();
                mainAvatarLayout_->setAlignment(Qt::AlignTop);
                avatar_ = new ContactAvatarWidget(mainWidget, QString(), QString(), Utils::scale_value(avatar_size), true);
                mainAvatarLayout_->addWidget(avatar_);
                avatarBottomSpace_ = new QWidget(mainWidget);
                avatarBottomSpace_->setFixedHeight(Utils::scale_value(avatar_botton_spacing));
                mainAvatarLayout_->addWidget(avatarBottomSpace_);
                avatarLayout->addLayout(mainAvatarLayout_);
            }
            avatarLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            editLayout_ = emptyVLayout();
            editLayout_->setAlignment(Qt::AlignTop);
            editLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(edit_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
            avatarLayout->addLayout(editLayout_);
            avatarLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            rootLayout->addLayout(avatarLayout);

            {
                nameLayout_ = emptyHLayout();
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                name_ = new TextEditEx(mainWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), false, false);
                name_->setStyleSheet(transparent_background);
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                nameLayout_->addWidget(name_);
                subEditLayout_ = emptyVLayout();
                subEditLayout_->setAlignment(Qt::AlignTop);
                subEditLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(edit_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                editLabel_ = new LabelEx(mainWidget);
                editLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Edit"));
                editLabel_->setProperty("edit", true);
                QPalette p;
                p.setColor(QPalette::Foreground, "#579e1c");
                editLabel_->setPalette(p);
                editLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
                editLabel_->setCursor(Qt::PointingHandCursor);
                editLabel_->adjustSize();
                subEditLayout_->addWidget(editLabel_);
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin / 2), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                nameLayout_->addLayout(subEditLayout_);
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin / 2), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                nameLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(nameLayout_);
            }

            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                description_ = new TextEditEx(mainWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor("#696969"), false, false);
                description_->setStyleSheet(transparent_background);
                description_->setFrameStyle(QFrame::NoFrame);
                description_->setContentsMargins(0, 0, 0, 0);
                description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                horLayoutIn->addWidget(description_);
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayoutIn);
            }

            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                moreLabel_ = new LabelEx(mainWidget);
                moreLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
                QPalette p;
                p.setColor(QPalette::Foreground, QColor("#579e1c"));
                moreLabel_->setPalette(p);
                moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "More"));
                moreLabel_->setContentsMargins(Utils::scale_value(more_left_margin), 0, 0, 0);
                moreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                horLayoutIn->addWidget(moreLabel_);
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayoutIn);
            }

            nameMargin_ = new QWidget(mainWidget);
            nameMargin_->setFixedHeight(Utils::scale_value(name_margin));
            rootLayout->addWidget(nameMargin_);

            firstLine_ = new LineWidget(mainWidget, Utils::scale_value(back_button_spacing + button_size), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
            rootLayout->addWidget(firstLine_);

            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                statusWidget_ = new QWidget(mainWidget);
                Utils::grabTouchWidget(statusWidget_);
                statusWidget_->setObjectName(QStringLiteral("statusWidget_"));
                statusWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
                statusWidget_->setMinimumSize(QSize(0, Utils::scale_value(45)));
                statusWidget_->setMaximumSize(QSize(16777215, Utils::scale_value(45)));
                auto statusLayout_ = new QHBoxLayout(statusWidget_);
                statusLayout_->setSpacing(0);
                statusLayout_->setObjectName(QStringLiteral("info_state_area_ayout"));
                statusLayout_->setContentsMargins(0, 0, 0, 0);

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
                statusMenu_->addAction(QIcon(":/resources/content_status_online_200.png"), QT_TRANSLATE_NOOP("sidebar", "Online"), this, SLOT(menuStateOnline()));
                statusMenu_->addAction(QIcon(":/resources/content_status_dnd_200.png"), QT_TRANSLATE_NOOP("sidebar", "Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
                statusMenu_->addAction(QIcon(":/resources/content_status_invisible_200.png"), QT_TRANSLATE_NOOP("sidebar", "Invisible"), this, SLOT(menuStateInvisible()));

                statusLabel_ = new LabelEx(statusWidget_);
                statusLabel_->setObjectName(QStringLiteral("statusLabel_"));
                statusLayout_->addWidget(statusLabel_);
                statusLayout_->addWidget(statusButton_);
                statusButton_->setMenu(statusMenu_);

                auto horizontalSpacer = new QSpacerItem(Utils::scale_value(40), Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);
                statusLayout_->addItem(horizontalSpacer);

                horLayoutIn->addWidget(statusWidget_);
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayoutIn);
            }

            buttonsMargin_ = new QWidget(mainWidget);
            buttonsMargin_->setFixedHeight(Utils::scale_value(buttons_top_margin));
            rootLayout->addWidget(buttonsMargin_);


            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                ignoreWidget_ = new QWidget(mainWidget);
                ignoreWidget_->setStyleSheet("background: #fbdbd9;");
                {
                    auto ignoreLayout = emptyVLayout(ignoreWidget_);
                    {
                        auto vLayout = new QVBoxLayout();
                        vLayout->setSpacing(Utils::scale_value(ignore_spacing));
                        vLayout->setContentsMargins(Utils::scale_value(more_left_margin + back_button_spacing), Utils::scale_value(ignore_margins), Utils::scale_value(back_button_spacing), Utils::scale_value(ignore_margins));
                        auto label = new QLabel(ignoreWidget_);
                        label->setWordWrap(true);
                        label->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16)));
                        QPalette p;
                        p.setColor(QPalette::Foreground, QColor("#000000"));
                        label->setPalette(p);
                        label->setText(QT_TRANSLATE_NOOP("sidebar", "This contact is in the ignore list"));
                        vLayout->addWidget(label);
                        ignoreLabel_ = new LabelEx(ignoreWidget_);
                        ignoreLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16)));
                        p.setColor(QPalette::Foreground, QColor("#e30f04"));
                        ignoreLabel_->setPalette(p);
                        ignoreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Delete"));
                        ignoreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                        vLayout->addWidget(ignoreLabel_);
                        ignoreLayout->addLayout(vLayout);
                    }
                }
                horLayoutIn->addWidget(ignoreWidget_);
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed));
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                rootLayout->addLayout(horLayoutIn);
            }

            {
                buttonWidget_ = new QWidget(mainWidget);
                auto hLayout = new QHBoxLayout(buttonWidget_);
                hLayout->setSpacing(Utils::scale_value(buttons_spacing));
                hLayout->setContentsMargins(0, 0, 0, Utils::scale_value(buttons_margin));
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                addButton_ = new CustomButton(mainWidget, ":/resources/contr_addpeople_big_100.png");
                addButton_->setHoverImage(":/resources/contr_addpeople_big_100_hover.png");
                addButton_->setActiveImage(":/resources/contr_addpeople_big_100_active.png");
                addButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                addButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(addButton_);

                chatButton_ = new CustomButton(mainWidget, ":/resources/contr_writemsg_big_100.png");
                chatButton_->setHoverImage(":/resources/contr_writemsg_big_100_hover.png");
                chatButton_->setActiveImage(":/resources/contr_writemsg_big_100_active.png");
                chatButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                chatButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(chatButton_);

                callButton_ = new CustomButton(mainWidget, ":/resources/contr_call_big_100.png");
                callButton_->setHoverImage(":/resources/contr_call_big_100_hover.png");
                callButton_->setActiveImage(":/resources/contr_call_big_100_active.png");
                callButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                callButton_->setDisabledImage(":/resources/contr_call_bigdisable_100.png");
                callButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(callButton_);

                videoCall_button_ = new CustomButton(mainWidget, ":/resources/contr_videocall_big_100.png");
                videoCall_button_->setHoverImage(":/resources/contr_videocall_big_100_hover.png");
                videoCall_button_->setActiveImage(":/resources/contr_videocall_big_100_active.png");
                videoCall_button_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                videoCall_button_->setDisabledImage(":/resources/contr_videocall_bigdisable_100.png");
                videoCall_button_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(videoCall_button_);

                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            }

            rootLayout->addWidget(buttonWidget_);

            uin_ = new InfoPlate(mainWidget, Utils::scale_value(back_button_spacing + button_size));
            uin_->setHeader(QT_TRANSLATE_NOOP("sidebar", "UIN"));
            rootLayout->addWidget(uin_);

            phone_ = new InfoPlate(mainWidget, Utils::scale_value(back_button_spacing + button_size));
            phone_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Phone number"));
            rootLayout->addWidget(phone_);

            firstName_ = new InfoPlate(mainWidget, Utils::scale_value(back_button_spacing + button_size));
            firstName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "First name"));
            rootLayout->addWidget(firstName_);

            lastName_ = new InfoPlate(mainWidget, Utils::scale_value(back_button_spacing + button_size));
            lastName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Last name"));
            rootLayout->addWidget(lastName_);

            nickName_ = new InfoPlate(mainWidget, Utils::scale_value(back_button_spacing + button_size));
            nickName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Nickname"));
            rootLayout->addWidget(nickName_);

            membersTopSpace_ = new QWidget(mainWidget);
            membersTopSpace_->setFixedHeight(Utils::scale_value(members_space));
            rootLayout->addWidget(membersTopSpace_);

            {
                auto hLayout = emptyHLayout();
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                members_ = new LiveChatMembersControl(mainWidget, info_, 4);
                hLayout->addWidget(members_);
                membersLabel_ = new QLabel(mainWidget);
                membersLabel_->setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(14)));
                QPalette p;
                p.setColor(QPalette::Foreground, QColor("#696969"));
                membersLabel_->setPalette(p);
                hLayout->addWidget(membersLabel_);
                hLayout->setAlignment(Qt::AlignLeft);
                rootLayout->addLayout(hLayout);
            }

            membersBottomSpace_ = new QWidget(mainWidget);
            membersBottomSpace_->setFixedHeight(Utils::scale_value(members_space));
            rootLayout->addWidget(membersBottomSpace_);

            {
                auto horLayout = emptyHLayout();
                horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(reverse_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                publicButton_ = new CustomButton(mainWidget, ":/resources/tabs_contacts_100.png");
                publicButton_->setOffsets(Utils::scale_value(button_offset), 0);
                publicButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Public chat"));
                publicButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
                publicButton_->setAlign(Qt::AlignLeft);
                publicButton_->setFocusPolicy(Qt::NoFocus);
                publicButton_->setFixedHeight(Utils::scale_value(button_height));
                publicButton_->adjustSize();
                horLayout->addWidget(publicButton_);
                publicCheckBox_ = new QCheckBox(mainWidget);
                publicCheckBox_->adjustSize();
                publicCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                publicCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(publicCheckBox_);
                horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayout);
            }

            renameButton_ = new ActionButton(mainWidget, ":/resources/sidebar_rename_100.png", QT_TRANSLATE_NOOP("sidebar", "Change chat name"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            renameButton_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(renameButton_);

            changeDescription = new ActionButton(mainWidget, ":/resources/sidebar_description_100.png", QT_TRANSLATE_NOOP("sidebar", "Change chat description"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            changeDescription->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(changeDescription);

            secondLine_ = new LineWidget(mainWidget, Utils::scale_value(back_button_spacing + button_size), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
            rootLayout->addWidget(secondLine_);

            renameContact_ = new ActionButton(mainWidget, ":/resources/sidebar_rename_100.png", QT_TRANSLATE_NOOP("sidebar", "Rename"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            renameContact_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(renameContact_);

            ignoreButton_ = new ActionButton(mainWidget, ":/resources/sidebar_ignore_100.png", QT_TRANSLATE_NOOP("sidebar", "Ignore"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            ignoreButton_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(ignoreButton_);

            spamButton_ = new ActionButton(mainWidget, ":/resources/sidebar_spam_100.png", QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            spamButton_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(spamButton_);

            deleteButton_ = new ActionButton(mainWidget, ":/resources/sidebar_delete_100.png", QT_TRANSLATE_NOOP("sidebar", "Delete"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            deleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(deleteButton_);

            quiAndDelete = new ActionButton(mainWidget, ":/resources/contr_signout_100.png", QT_TRANSLATE_NOOP("sidebar", "Leave and delete"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            quiAndDelete->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(quiAndDelete);

            ignoreListButton = new ActionButton(mainWidget, ":/resources/content_ignorelist_100.png", QT_TRANSLATE_NOOP("sidebar", "Ignored contacts"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            ignoreListButton->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(ignoreListButton);

            attachOldAcc = new ActionButton(mainWidget, ":/resources/content_oldaccount_100.png", QT_TRANSLATE_NOOP("sidebar", "Connect to ICQ account"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            attachOldAcc->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout->addWidget(attachOldAcc);

            rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
            hLayoutMain->addLayout(rootLayout);
            rightWidget_ = new QWidget(mainWidget);
            rightWidget_->setFixedWidth(0);
            hLayoutMain->addWidget(rightWidget_);
            vLayoutMain->addLayout(hLayoutMain);
        }

        connect(moreLabel_, SIGNAL(clicked()), this, SLOT(moreClicked()), Qt::QueuedConnection);
        connect(ignoreLabel_, SIGNAL(clicked()), this, SLOT(removeFromIgnore()), Qt::QueuedConnection);
        connect(Logic::GetContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Logic::GetContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);
        connect(Logic::GetIgnoreModel(), SIGNAL(results()), this, SLOT(changed()), Qt::QueuedConnection);

        connect(addButton_, SIGNAL(clicked()), this, SLOT(addClicked()), Qt::QueuedConnection);
        connect(chatButton_, SIGNAL(clicked()), this, SLOT(chatClicked()), Qt::QueuedConnection);
        connect(callButton_, SIGNAL(clicked()), this, SLOT(callClicked()), Qt::QueuedConnection);
        connect(videoCall_button_, SIGNAL(clicked()), this, SLOT(videoClicked()), Qt::QueuedConnection);
        connect(backButton_, SIGNAL(clicked()), this, SLOT(back()), Qt::QueuedConnection);

        connect(ignoreButton_, SIGNAL(clicked()), this, SLOT(ignore()), Qt::QueuedConnection);
        connect(spamButton_, SIGNAL(clicked()), this, SLOT(spam()), Qt::QueuedConnection);
        connect(deleteButton_, SIGNAL(clicked()), this, SLOT(remove()), Qt::QueuedConnection);
        connect(quiAndDelete, SIGNAL(clicked()), this, SLOT(quit()), Qt::QueuedConnection);
        connect(renameButton_, SIGNAL(clicked()), this, SLOT(rename()), Qt::QueuedConnection);
        connect(changeDescription, SIGNAL(clicked()), this, SLOT(changeDesc()), Qt::QueuedConnection);
        connect(renameContact_, SIGNAL(clicked()), this, SLOT(rename()), Qt::QueuedConnection);
        connect(ignoreListButton, SIGNAL(clicked()), this, SLOT(ignoreList()), Qt::QueuedConnection);
        connect(attachOldAcc, SIGNAL(clicked()), this, SLOT(attachOld()), Qt::QueuedConnection);
        connect(editLabel_, SIGNAL(clicked()), this, SLOT(editClicked()), Qt::QueuedConnection);
        connect(statusLabel_, SIGNAL(clicked()), this, SLOT(statusClicked()), Qt::QueuedConnection);
        connect(publicCheckBox_, SIGNAL(stateChanged(int)), this, SLOT(publicChanged(int)), Qt::QueuedConnection);

        connect(MyInfo(), SIGNAL(received()), this, SLOT(changed()), Qt::QueuedConnection);
        connect(avatar_, &Ui::ContactAvatarWidget::afterAvatarChanged, this, &Ui::ProfilePage::avatarChanged, Qt::QueuedConnection);
        
        if (Ui::GetDispatcher()->is_im_created())
            Logic::GetContactListModel()->get_ignore_list();
        else
            connect(Ui::GetDispatcher(), &core_dispatcher::im_created, []() { Logic::GetContactListModel()->get_ignore_list(); });
   }

    void ProfilePage::initDescription(const QString& description, bool full)
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
        updateWidth();
    }

    void ProfilePage::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> info)
    {
        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            initDescription(info_->About_);
            members_->updateInfo(info_);
            members_->adjustWidth();
            QString members = "+";
            members += QVariant(info_->MembersCount_ - members_->getRealCount()).toString();
            membersLabel_->setText(members);
            if (info_->YourRole_ == "admin")
            {
                avatar_->SetIsInMyProfile(true);
                renameButton_->show();
                changeDescription->show();
                publicButton_->show();
                publicCheckBox_->show();
                publicCheckBox_->blockSignals(true);
                publicCheckBox_->setChecked(info_->Public_);
                publicCheckBox_->blockSignals(false);
            }
            updateWidth();
        }
    }

    void ProfilePage::moreClicked()
    {
        initDescription(info_->About_, true);
    }

    void ProfilePage::removeFromIgnore()
    {
        deleteMemberDialog(Logic::GetIgnoreModel(), currentAimId_, Logic::IGNORE_LIST, this);
    }

    void ProfilePage::contactChanged(QString aimId)
    {
        if (aimId == currentAimId_)
            initFor(aimId);
    }

    void ProfilePage::contactRemoved(QString aimId)
    {
        if (aimId == currentAimId_)
            Utils::InterConnector::instance().setSidebarVisible(false);
    }

    void ProfilePage::changed()
    {
        initFor(currentAimId_);
    }

    void ProfilePage::updateStatus()
    {
        statusWidget_->hide();
        if (Logic::GetContactListModel()->isChat(currentAimId_))
            return;

        QString state;
        QDateTime lastSeen;
        Logic::ContactItem* cont = Logic::GetContactListModel()->getContactItem(currentAimId_);
        if (!cont)
        {
            if (currentAimId_ == MyInfo()->aimId())
            {
                state = MyInfo()->state();
            }
        }
        else
        {
            state = cont->Get()->State_;
            lastSeen = cont->Get()->GetLastSeen();
            if (lastSeen.isValid())
            {
                state = QT_TRANSLATE_NOOP("sidebar", "Seen ");
                if (lastSeen.isValid())
                {
                    const auto current = QDateTime::currentDateTime();
                    const auto days = lastSeen.daysTo(current);
                    if (days == 0)
                        state += QT_TRANSLATE_NOOP("sidebar", "today");
                    else if (days == 1)
                        state += QT_TRANSLATE_NOOP("sidebar", "yesterday");
                    else
                        state += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
                    if (lastSeen.date().year() == current.date().year())
                    {
                        state += QT_TRANSLATE_NOOP("sidebar", " at ");
                        state += lastSeen.time().toString(Qt::SystemLocaleShortDate);
                    }
                }
            }
            else if (state.isEmpty())
            {
                state = QT_TRANSLATE_NOOP("sidebar", "Online");
            }
        }

        QPalette p;
        p.setColor(QPalette::Foreground, QColor(lastSeen.isValid() ? "#282828" : "#579e1c"));
        statusLabel_->setPalette(p);
        statusLabel_->setText(state);
        statusLabel_->adjustSize();

        if (!state.isEmpty())
        {
            statusWidget_->show();
        }
    }

    void ProfilePage::addClicked()
    {
        Logic::GetContactListModel()->add_contact_to_contact_list(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_profile_page);
    }

    void ProfilePage::chatClicked()
    {
        Logic::GetContactListModel()->setCurrent(currentAimId_, true, true);
    }

    void ProfilePage::callClicked()
    {
        Ui::GetDispatcher()->getVoipController().setStartA(currentAimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance()) {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_call);
    }

    void ProfilePage::videoClicked()
    {
        Ui::GetDispatcher()->getVoipController().setStartV(currentAimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance()) {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_video_call);
    }

    void ProfilePage::back()
    {
        if (prevAimId_.isEmpty())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else
            Utils::InterConnector::instance().showSidebar(prevAimId_, menu_page);
    }

    void ProfilePage::ignore()
    {
        if (Logic::GetContactListModel()->ignore_and_remove_from_cl_contact(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_profile_page);
            back();
        }
    }

    void ProfilePage::spam()
    {
        if (Logic::GetContactListModel()->block_spam_contact(currentAimId_))
        {
            Logic::GetContactListModel()->remove_contact_from_contact_list(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_profile_page);
        }
    }

    void ProfilePage::remove()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete contact?"),
            Logic::GetContactListModel()->getDisplayName(currentAimId_),
            NULL);
        if (confirmed)
        {
            Logic::GetContactListModel()->remove_contact_from_contact_list(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_profile_page);
        }
    }

    void ProfilePage::quit()
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
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_profile_page);
        }
    }

    void ProfilePage::rename()
    {
        QString result_chat_name;

        bool isChat = Logic::GetContactListModel()->isChat(currentAimId_);
        bool isLiveChat = Logic::GetContactListModel()->isLiveChat(currentAimId_);

        auto result = Utils::NameEditor(
            this,
            Logic::GetContactListModel()->getDisplayName(currentAimId_),
            QT_TRANSLATE_NOOP("sidebar","Save"),
            isChat ? QT_TRANSLATE_NOOP("sidebar", "Chat name") : QT_TRANSLATE_NOOP("sidebar", "Contact name"),
            result_chat_name);

        if (result)
        {
            if (isChat)
            {
                if (isLiveChat)
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("aimid", currentAimId_);
                    collection.set_value_as_qstring("name", result_chat_name);
                    Ui::GetDispatcher()->post_message_to_core("chats/mod/name", collection.get());
                }
                else
                {
                    Logic::GetContactListModel()->rename_chat(currentAimId_, result_chat_name);
                }
            }
            else
            {
                Logic::GetContactListModel()->rename_contact(currentAimId_, result_chat_name);
            }
        }
    }

    void ProfilePage::resizeEvent(QResizeEvent* e)
    {
        if (e->size().width() >= Utils::scale_value(596))
        {
            mainBackButtonLayout_->takeAt(0);
            if (mainAvatarLayout_->itemAt(0) && mainAvatarLayout_->itemAt(0)->widget() == avatar_)
                mainAvatarLayout_->takeAt(0);
            avatarBottomSpace_->setVisible(false);
            subBackButtonLayout_->insertWidget(0, backButton_);
            subAvatarLayout_->insertWidget(0, avatar_);
            rightWidget_->setFixedWidth(Utils::scale_value(more_right_margin));
            if (editLayout_->itemAt(1) && editLayout_->itemAt(1)->widget() && editLayout_->itemAt(1)->widget()->property("edit").toBool())
                editLayout_->takeAt(1);
            subEditLayout_->insertWidget(1, editLabel_);
        }
        else
        {
            subBackButtonLayout_->takeAt(0);
            subAvatarLayout_->takeAt(0);
            avatarBottomSpace_->setVisible(true);
            mainBackButtonLayout_->insertWidget(0, backButton_);
            mainAvatarLayout_->insertWidget(0, avatar_);
            rightWidget_->setFixedWidth(Utils::scale_value(0));
            if (subEditLayout_->itemAt(1) && subEditLayout_->itemAt(1)->widget() && subEditLayout_->itemAt(1)->widget()->property("edit").toBool())
                subEditLayout_->takeAt(1);
            editLayout_->insertWidget(1, editLabel_);
        }
        QWidget::resizeEvent(e);
        updateWidth();
    }

    void ProfilePage::menuStateOnline()
    {
        setState(core::profile_state::online);
    }
    void ProfilePage::menuStateDoNotDisturb()
    {
        setState(core::profile_state::dnd);
    }

    void ProfilePage::menuStateInvisible()
    {
        setState(core::profile_state::invisible);
    }

    void ProfilePage::ignoreList()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_open);
        QVector<QString> temp;
        Logic::UpdateIgnoredModel(temp);
        Logic::GetSearchMemberModel()->SetChatMembersModel(Logic::GetIgnoreModel());

        SelectContactsWidget select_members_dialog_(Logic::GetIgnoreModel(), Logic::MembersWidgetRegim::IGNORE_LIST,
            "", QT_TRANSLATE_NOOP("groupchat_pages", "Done"), this);
        auto connectId = connect(GetDispatcher(), SIGNAL(recv_permit_deny(bool)), &select_members_dialog_, SLOT(UpdateViewForIgnoreList(bool)), Qt::QueuedConnection);

        Logic::ContactListModel::get_ignore_list();
        select_members_dialog_.setView(false);
        select_members_dialog_.show(-1, -1);
    }

    void ProfilePage::editClicked()
    {
        auto url = QString("https://icq.com/people/%1/edit").arg(currentAimId_);
        core::coll_helper helper(GetDispatcher()->create_collection(), true);
        helper.set_value_as_string("url", url.toStdString());
        GetDispatcher()->post_message_to_core("sign_url", helper.get());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_edit);

        GetDispatcher()->disconnect(SIGNAL(signedUrl(QString)));
        connect(GetDispatcher(), &core_dispatcher::signedUrl, [](QString url)
        {
            Utils::InterConnector::instance().unsetUrlHandler();
            QDesktopServices::openUrl(url);
            Utils::InterConnector::instance().setUrlHandler();
        });
    }

    void ProfilePage::statusClicked()
    {
        if (MyInfo()->aimId() == currentAimId_)
            statusMenu_->popup(QCursor::pos());
    }

    void ProfilePage::changeDesc()
    {
        QString result_chat_desc;
        auto result = Utils::NameEditor(
            this,
            info_->About_,
            QT_TRANSLATE_NOOP("sidebar","Save"),
            QT_TRANSLATE_NOOP("sidebar", "Chat description"),
            result_chat_desc, false);


        if (result)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("about", result_chat_desc);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/about", collection.get());
        }
    }

    void ProfilePage::publicChanged(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("public", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core("chats/mod/public", collection.get());
    }

    void ProfilePage::attachOld()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachUin);
    }

    void ProfilePage::recvFlags(int flags)
    {
        connectOldVisible_ = flags & 0x1000;
        attachOldAcc->setVisible(currentAimId_ == MyInfo()->aimId() && connectOldVisible_);
    }

    void ProfilePage::setState(const core::profile_state _state)
    {
        assert(_state > core::profile_state::min);
        assert(_state < core::profile_state::max);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        std::stringstream stream;
        stream << _state;

        collection.set_value_as_string("state", stream.str());
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
        GetDispatcher()->post_stats_to_core(stateToStatisticsEvent(_state));
    }

    void ProfilePage::avatarChanged()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_avatar_changed);
    }
}