#include "stdafx.h"
#include "ProfilePage.h"
#include "SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/SearchMembersModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../livechats/LiveChatMembersControl.h"
#include "../../my_info.h"
#include "../../core_dispatcher.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../cache/countries.h"

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
    const int edit_right_margin = 12;
    const int save_button_offset = 32;
    const int name_desc_space = 40;

    const static QString transparent_background = "background: transparent;";

    QString saveButtonStyle()
    {
        return QString(
            "QPushButton {"
            "color: #ffffff;"
            "font-size: 16dip;"
            "background-color: #579e1c;"
            "border-style: none;"
            "margin: 0;"
            "padding-left: 20dip; padding-right: 20dip;"
            "min-width: 100dip;"
            "max-height: 40dip; min-height: 40dip;"
            "text-align: center; }"
            "QPushButton:hover { background-color: #57a813; }"
            "QPushButton:pressed { background-color: #50901b; } "
            );
    }
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
            header_->setFont(Fonts::appFontScaled(14));
            header_->setPalette(p);
            vLayout->addWidget(header_);
            info_ = new TextEmojiWidget(this, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), Utils::scale_value(18), QColor("#000000"));
            connect(info_, SIGNAL(rightClicked()), this, SLOT(menuRequested()), Qt::QueuedConnection);
            info_->setSelectable(true);
            vLayout->addWidget(info_);
            phoneInfo_ = new QLabel(this);
            phoneInfo_->setWordWrap(true);
            phoneInfo_->setFont(Fonts::appFontScaled(14));
            phoneInfo_->setPalette(p);
            vLayout->addWidget(phoneInfo_);
            phoneInfo_->hide();
            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(button_bottom_space), QSizePolicy::Preferred, QSizePolicy::Fixed));
            rootLayout->addLayout(vLayout);
            Utils::grabTouchWidget(header_);
            Utils::grabTouchWidget(info_);
            Utils::grabTouchWidget(phoneInfo_);
            Utils::grabTouchWidget(this);
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
        QApplication::clipboard()->setText(infoStr_);
    }

    void InfoPlate::setAttachPhone(bool value)
    {
        attachPhone_ = value;
        info_->setSelectable(!value);
        if (value)
        {
            info_->setText(QT_TRANSLATE_NOOP("sidebar", "Attach phone"), CommonStyle::getLinkColor());
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

    void InfoPlate::setInfo(const QString& info, const QString& prefix)
    {
        infoStr_ = info;
        if (!infoStr_.startsWith(prefix))
            infoStr_ = prefix + infoStr_;

        info_->setText(infoStr_, QColor("#000000"));
        info_->adjustSize();
        phoneInfo_->hide();
    }

    void InfoPlate::elideText(int width)
    {
        if (attachPhone_)
            return;

        QFontMetrics m(Fonts::appFontScaled(14));
        QString result = m.elidedText(infoStr_, Qt::ElideRight, width);
        info_->setText(result);
        info_->setSourceText(infoStr_);
    }

    ProfilePage::ProfilePage(QWidget* parent)
        : SidebarPage(parent)
        , info_(std::make_shared<Data::ChatInfo>())
        , connectOldVisible_(false)
        , myInfo_(false)
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
        if (myInfo_)
            p.fillRect(rect(), Qt::white);
        else
            p.fillRect(rect(), CommonStyle::getFrameTransparency());
        p.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        p.drawLine(contentsRect().topLeft(), contentsRect().bottomLeft());
    }

    void ProfilePage::updateWidth()
    {
        Line_->setLineWidth(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin) - rightWidget_->width());
        if (myInfo_ && width() >= Utils::scale_value(596))
        {
            name_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin + left_margin) - rightWidget_->width() - editLabel_->width());
        }
        else if (width() >= Utils::scale_value(596))
        {
            nameEdit_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin + left_margin) - rightWidget_->width());
            descriptionEdit_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin + left_margin) - rightWidget_->width());
            saveButtonSpace_->setFixedWidth(width_ - Utils::scale_value(button_size + back_button_spacing + left_margin + Utils::scale_value(8)) - rightWidget_->width() - saveButton_->width());
        }
        else
        {
            name_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
            nameEdit_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
            descriptionEdit_->adjustHeight(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width());
            saveButtonSpace_->setFixedWidth(width_ - Utils::scale_value(button_size + back_button_spacing + right_margin) - rightWidget_->width() - saveButton_->width());
        }

        ignoreWidget_->setFixedWidth(width_ - Utils::scale_value(left_margin + right_margin + back_button_spacing) - rightWidget_->width());
        uin_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        phone_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        firstName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        lastName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        nickName_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        birthday_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        city_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));
        country_->elideText(width_ - Utils::scale_value(back_button_spacing + button_size + right_margin + left_margin + button_offset));

        int diff = rightWidget_->width() == 0? Utils::scale_value(left_margin) : 0;
        renameContact_->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        ignoreListButton->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        attachOldAcc->setFixedWidth(width_ - Utils::scale_value(right_margin + left_margin) + diff);
        avatar_->update();
    }

    void ProfilePage::touchScrollStateChanged(QScroller::State st)
    {
        name_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        nameEdit_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        descriptionEdit_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        uin_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        phone_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        firstName_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        lastName_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        nickName_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        birthday_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        city_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        country_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        renameContact_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        ignoreListButton->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        attachOldAcc->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
        avatar_->setAttribute(Qt::WA_TransparentForMouseEvents, st != QScroller::Inactive);
    }

    void ProfilePage::initFor(const QString& aimId)
    {
        const auto fixedAimId = aimId.isEmpty() ? MyInfo()->aimId() : aimId;
        bool newContact = (currentAimId_ != fixedAimId);
        currentAimId_ = fixedAimId;
        myInfo_ = (MyInfo()->aimId() == currentAimId_ && aimId.isEmpty());
        
        backButton_->setVisible(!myInfo_);
        
        Logic::GetAvatarStorage()->ForceRequest(currentAimId_, Utils::scale_value(avatar_size));

        avatar_->UpdateParams(currentAimId_, Logic::getContactListModel()->getDisplayName(currentAimId_));
        avatar_->update();

        statusButton_->setStyle(QApplication::style());
        statusLabel_->setStyle(QApplication::style());
        statusLabel_->setCursor(myInfo_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
        statusButton_->setVisible(myInfo_);

        core::coll_helper helper(GetDispatcher()->create_collection(), true);
        GetDispatcher()->post_message_to_core("load_flags", helper.get());

        attachOldAcc->setVisible(myInfo_ && connectOldVisible_);

        if (newContact)
        {
            chatEditWidget_->hide();
            phone_->hide();
            firstName_->hide();
            lastName_->hide();
            nickName_->hide();
            birthday_->hide();
            city_->hide();
            country_->hide();
        }


        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(Logic::getContactListModel()->getDisplayName(currentAimId_), cursorName, Logic::Text2DocHtmlMode::Pass, false);

        editLabel_->setVisible(myInfo_);

        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);
        saveButton_->setVisible(isChat);
        saveButtonMargin_->setVisible(isChat);
        name_->setVisible(!isChat);
        avatar_->SetMode(myInfo_ ? ContactAvatarWidget::Mode::MyProfile : ContactAvatarWidget::Mode::Common);
        if (isChat)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_int("limit", members_count);
            Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
        }

        updateStatus();

        bool isIgnored = Logic::getIgnoreModel()->getMemberItem(currentAimId_) != 0;
        ignoreWidget_->setVisible(isIgnored);
        buttonWidget_->setVisible(!isIgnored && !isChat && currentAimId_ != MyInfo()->aimId());
        buttonsMargin_->setVisible(!isIgnored && !isChat);

        auto cont = Logic::getContactListModel()->getContactItem(currentAimId_);

        bool isNotAuth = Logic::getContactListModel()->isNotAuth(currentAimId_) || cont == 0;
        chatButton_->setVisible(!isNotAuth);
        addButton_->setVisible(isNotAuth);
        callButton_->setEnabled(!isNotAuth);
        videoCall_button_->setEnabled(!isNotAuth);

        callButton_->setVisible(!platform::is_linux());
        videoCall_button_->setVisible(!platform::is_linux());

        ignoreListButton->setVisible(myInfo_);
        nameMargin_->setVisible(isChat);
        renameContact_->setVisible(!isChat && !isNotAuth && !isIgnored && !myInfo_);

        Line_->setVisible((!isChat && !isNotAuth && !isIgnored && !myInfo_) || myInfo_);

        uin_->setInfo(currentAimId_);
        uin_->setVisible(!isChat);

        if (Logic::getContactListModel()->isChat(currentAimId_))
            return;

        Logic::getContactListModel()->getContactProfile(currentAimId_, [this](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            if (!_profile)
                return;

            if (myInfo_)
            {
                if (!MyInfo()->phoneNumber().isEmpty())
                    phone_->setInfo(MyInfo()->phoneNumber(), "+");

                phone_->setAttachPhone(MyInfo()->phoneNumber().isEmpty());
                phone_->show();
            }
            else
            {
                Logic::phone_list list = _profile->get_phones();
                if (!list.empty())
                {
                    phone_->setInfo(list.front().get_phone(), "+");
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

            auto nn = myInfo_ ? MyInfo()->friendlyName() : _profile->get_friendly();
            if (!nn.isEmpty())
            {
                nickName_->setInfo(nn);
                nickName_->show();

                if (Logic::getContactListModel()->getDisplayName(currentAimId_) == currentAimId_ || myInfo_)
                {
                    name_->setPlainText(QString());
                    QTextCursor cursorName = name_->textCursor();
                    Logic::Text2Doc(nn, cursorName, Logic::Text2DocHtmlMode::Pass, false);
                    Logic::GetAvatarStorage()->UpdateAvatar(currentAimId_, false);
                    avatar_->UpdateParams(currentAimId_, nn);
                    avatar_->update();
                }
            }
            else if (Logic::getContactListModel()->getDisplayName(currentAimId_) == currentAimId_)
            {
                QString name = fn;
                if (!name.isEmpty())
                    name += " ";
                name += ln;
                if (!name.isEmpty())
                {
                    name_->setPlainText(QString());
                    QTextCursor cursorName = name_->textCursor();
                    Logic::Text2Doc(name, cursorName, Logic::Text2DocHtmlMode::Pass, false);
                }
            }

            int64_t birth = _profile->get_birthdate();
            if (birth > 0)
            {
                QDateTime dt = QDateTime::fromTime_t(birth);
                if (dt.isValid())
                {
                    birthday_->setInfo(Utils::GetTranslator()->formatDate(dt.date(), false));
                    birthday_->show();
                }
            }

            auto city = _profile->get_home_address().get_city();
            if (!city.isEmpty())
            {
                city_->setInfo(city);
                city_->show();
            }

            auto countryCode = _profile->get_home_address().get_country().toLower();
            if (!countryCode.isEmpty())
            {
                auto countries = countries::get();
                for (auto country : countries)
                {
                    if (country.iso_code_ == countryCode)
                    {
                        country_->setInfo(country.name_);
                        country_->show();
                        break;
                    }
                }
            }

            updateWidth();
        });

        updateWidth();

        if (myInfo_)
            connect(MyInfo(), SIGNAL(received()), this, SLOT(changed()), Qt::UniqueConnection);
        else
            disconnect(MyInfo(), SIGNAL(received()), this, SLOT(changed()));
    }

    void ProfilePage::init()
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/sidebar/Sidebar.qss"));
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);
        connect(GetDispatcher(), SIGNAL(recvFlags(int)), this, SLOT(recvFlags(int)));

        auto layout = emptyVLayout(this);
        auto area = CreateScrollAreaAndSetTrScrollBar(this);
        area->horizontalScrollBar()->setDisabled(true);
        layout->addWidget(area);

        area->setContentsMargins(0, 0, 0, 0);
        mainWidget_ = new QWidget(area);
        area->setWidget(mainWidget_);
        area->setWidgetResizable(true);
        area->setFrameStyle(QFrame::NoFrame);
        area->setStyleSheet(transparent_background);

        Utils::grabTouchWidget(area->viewport(), true);
        Utils::grabTouchWidget(mainWidget_);

        connect(QScroller::scroller(area->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        auto vLayoutMain = emptyVLayout(mainWidget_);
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
                backButton_ = new CustomButton(mainWidget_, ":/resources/contr_back_100.png");
                backButton_->setHoverImage(":/resources/contr_back_100_hover.png");
                backButton_->setActiveImage(":/resources/contr_back_100_active.png");
                backButton_->setFixedSize(Utils::scale_value(button_size), Utils::scale_value(button_size));
                backButton_->setCursor(QCursor(Qt::PointingHandCursor));
                Utils::grabTouchWidget(backButton_);
                mainBackButtonLayout_->addWidget(backButton_);
                avatarLayout->addLayout(mainBackButtonLayout_);
            }
            avatarLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            {
                mainAvatarLayout_ = emptyVLayout();
                mainAvatarLayout_->setAlignment(Qt::AlignTop);
                avatar_ = new ContactAvatarWidget(mainWidget_, QString(), QString(), Utils::scale_value(avatar_size), true);
                auto mainSaveButtonLayout_ = emptyHLayout();
                mainSaveButtonLayout_->setAlignment(Qt::AlignRight);
                saveButtonSpace_ = new QWidget(mainWidget_);
                Utils::grabTouchWidget(saveButtonSpace_);
                mainSaveButtonLayout_->addWidget(saveButtonSpace_);
                auto subSaveButtonLayout = emptyVLayout();
                saveButton_ = new QPushButton(mainWidget_);
                Utils::ApplyStyle(saveButton_, saveButtonStyle());
                saveButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Save"));
                saveButton_->setCursor(Qt::PointingHandCursor);
                saveButton_->adjustSize();
                Utils::grabTouchWidget(saveButton_);
                subSaveButtonLayout->addWidget(saveButton_);
                saveButtonMargin_ = new QWidget(mainWidget_);
                saveButtonMargin_->setFixedHeight(Utils::scale_value(save_button_offset));
                Utils::grabTouchWidget(saveButtonMargin_);
                subSaveButtonLayout->addWidget(saveButtonMargin_);
                mainSaveButtonLayout_->addLayout(subSaveButtonLayout);
                mainAvatarLayout_->addLayout(mainSaveButtonLayout_);
                Utils::grabTouchWidget(avatar_);
                mainAvatarLayout_->addWidget(avatar_);
                avatarBottomSpace_ = new QWidget(mainWidget_);
                avatarBottomSpace_->setFixedHeight(Utils::scale_value(avatar_botton_spacing));
                Utils::grabTouchWidget(avatarBottomSpace_);
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
                name_ = new TextEditEx(mainWidget_, Fonts::defaultAppFontFamily(), Utils::scale_value(24), CommonStyle::getTextCommonColor(), false, false);
                name_->setStyleSheet(transparent_background);
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setContextMenuPolicy(Qt::NoContextMenu);
                Utils::grabTouchWidget(name_);
                nameLayout_->addWidget(name_);
                subEditLayout_ = emptyVLayout();
                subEditLayout_->setAlignment(Qt::AlignTop);
                subEditLayout_->addSpacerItem(new QSpacerItem(0, Utils::scale_value(edit_top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));
                editLabel_ = new LabelEx(mainWidget_);
                editLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Edit"));
                editLabel_->setProperty("edit", true);
                QPalette p;
                p.setColor(QPalette::Foreground, CommonStyle::getLinkColor());
                editLabel_->setPalette(p);
                editLabel_->setFont(Fonts::appFontScaled(14));
                editLabel_->setCursor(Qt::PointingHandCursor);
                editLabel_->adjustSize();
                subEditLayout_->addWidget(editLabel_);
                Utils::grabTouchWidget(editLabel_);
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin / 2), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                nameLayout_->addLayout(subEditLayout_);
                nameLayout_->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin / 2), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                nameLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(nameLayout_);
            }

            nameMargin_ = new QWidget(mainWidget_);
            nameMargin_->setFixedHeight(Utils::scale_value(name_margin));
            Utils::grabTouchWidget(nameMargin_);
            rootLayout->addWidget(nameMargin_);

            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                statusWidget_ = new QWidget(mainWidget_);
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
                statusButton_->setObjectName("statusButton");
                statusButton_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
                statusButton_->setMinimumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
                statusButton_->setMaximumSize(QSize(Utils::scale_value(36), Utils::scale_value(36)));
                statusButton_->setFlat(true);
                statusButton_->setIconSize(QSize(Utils::scale_value(24), Utils::scale_value(24)));
                statusButton_->setFocusPolicy(Qt::NoFocus);
                statusButton_->setCursor(Qt::CursorShape::PointingHandCursor);
                statusMenu_ = new FlatMenu(statusButton_);
                statusMenu_->setObjectName("statusMenu");
                statusMenu_->addAction(QIcon(":/resources/content_status_online_200.png"), QT_TRANSLATE_NOOP("sidebar", "Online"), this, SLOT(menuStateOnline()));
                statusMenu_->addAction(QIcon(":/resources/content_status_dnd_200.png"), QT_TRANSLATE_NOOP("sidebar", "Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
                statusMenu_->addAction(QIcon(":/resources/content_status_invisible_200.png"), QT_TRANSLATE_NOOP("sidebar", "Invisible"), this, SLOT(menuStateInvisible()));

                statusLabel_ = new LabelEx(statusWidget_);
                statusLabel_->setObjectName("statusLabel");
                statusLayout_->addWidget(statusLabel_);
                statusLayout_->addWidget(statusButton_);
                statusButton_->setMenu(statusMenu_);

                auto horizontalSpacer = new QSpacerItem(Utils::scale_value(40), Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);
                statusLayout_->addItem(horizontalSpacer);

                Utils::grabTouchWidget(statusLabel_);
                Utils::grabTouchWidget(statusMenu_);
                Utils::grabTouchWidget(statusWidget_);
                horLayoutIn->addWidget(statusWidget_);
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                rootLayout->addLayout(horLayoutIn);
            }

            buttonsMargin_ = new QWidget(mainWidget_);
            buttonsMargin_->setFixedHeight(Utils::scale_value(buttons_top_margin));
            Utils::grabTouchWidget(buttonsMargin_);
            rootLayout->addWidget(buttonsMargin_);


            {
                auto horLayoutIn = emptyHLayout();
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(button_size - more_left_margin), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                ignoreWidget_ = new QWidget(mainWidget_);
                ignoreWidget_->setStyleSheet("background: #fbdbd9;");
                {
                    auto ignoreLayout = emptyVLayout(ignoreWidget_);
                    {
                        auto vLayout = new QVBoxLayout();
                        vLayout->setSpacing(Utils::scale_value(ignore_spacing));
                        vLayout->setContentsMargins(Utils::scale_value(more_left_margin + back_button_spacing), Utils::scale_value(ignore_margins), Utils::scale_value(back_button_spacing), Utils::scale_value(ignore_margins));
                        auto label = new QLabel(ignoreWidget_);
                        label->setWordWrap(true);
                        label->setFont(Fonts::appFontScaled(16));
                        QPalette p;
                        p.setColor(QPalette::Foreground, QColor("#000000"));
                        label->setPalette(p);
                        label->setText(QT_TRANSLATE_NOOP("sidebar", "This contact is in the ignore list"));
                        vLayout->addWidget(label);
                        ignoreLabel_ = new LabelEx(ignoreWidget_);
                        ignoreLabel_->setFont(Fonts::appFontScaled(16));
                        p.setColor(QPalette::Foreground, QColor("#e30f04"));
                        ignoreLabel_->setPalette(p);
                        ignoreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Delete"));
                        ignoreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                        vLayout->addWidget(ignoreLabel_);
                        ignoreLayout->addLayout(vLayout);
                    }
                }
                Utils::grabTouchWidget(ignoreLabel_);
                Utils::grabTouchWidget(ignoreWidget_);
                horLayoutIn->addWidget(ignoreWidget_);
                horLayoutIn->addSpacerItem(new QSpacerItem(Utils::scale_value(right_margin), 0, QSizePolicy::Fixed));
                horLayoutIn->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                rootLayout->addLayout(horLayoutIn);
            }

            {
                buttonWidget_ = new QWidget(mainWidget_);
                auto hLayout = new QHBoxLayout(buttonWidget_);
                hLayout->setSpacing(Utils::scale_value(buttons_spacing));
                hLayout->setContentsMargins(0, 0, 0, Utils::scale_value(buttons_margin));
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
                addButton_ = new CustomButton(mainWidget_, ":/resources/contr_addpeople_big_100.png");
                addButton_->setHoverImage(":/resources/contr_addpeople_big_100_hover.png");
                addButton_->setActiveImage(":/resources/contr_addpeople_big_100_active.png");
                addButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                addButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(addButton_);

                chatButton_ = new CustomButton(mainWidget_, ":/resources/contr_writemsg_big_100.png");
                chatButton_->setHoverImage(":/resources/contr_writemsg_big_100_hover.png");
                chatButton_->setActiveImage(":/resources/contr_writemsg_big_100_active.png");
                chatButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                chatButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(chatButton_);

                callButton_ = new CustomButton(mainWidget_, ":/resources/contr_call_big_100.png");
                callButton_->setHoverImage(":/resources/contr_call_big_100_hover.png");
                callButton_->setActiveImage(":/resources/contr_call_big_100_active.png");
                callButton_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                callButton_->setDisabledImage(":/resources/contr_call_bigdisable_100.png");
                callButton_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(callButton_);

                videoCall_button_ = new CustomButton(mainWidget_, ":/resources/contr_videocall_big_100.png");
                videoCall_button_->setHoverImage(":/resources/contr_videocall_big_100_hover.png");
                videoCall_button_->setActiveImage(":/resources/contr_videocall_big_100_active.png");
                videoCall_button_->setFixedSize(Utils::scale_value(big_button_size), Utils::scale_value(big_button_size));
                videoCall_button_->setDisabledImage(":/resources/contr_videocall_bigdisable_100.png");
                videoCall_button_->setCursor(QCursor(Qt::PointingHandCursor));
                hLayout->addWidget(videoCall_button_);

                hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            }
            Utils::grabTouchWidget(addButton_);
            Utils::grabTouchWidget(chatButton_);
            Utils::grabTouchWidget(callButton_);
            Utils::grabTouchWidget(videoCall_button_);
            Utils::grabTouchWidget(buttonWidget_);
            rootLayout->addWidget(buttonWidget_);

            uin_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            uin_->setHeader(QT_TRANSLATE_NOOP("sidebar", "UIN"));
            rootLayout->addWidget(uin_);

            phone_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            phone_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Phone number"));
            rootLayout->addWidget(phone_);

            firstName_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            firstName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "First name"));
            rootLayout->addWidget(firstName_);

            lastName_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            lastName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Last name"));
            rootLayout->addWidget(lastName_);

            nickName_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            nickName_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Nickname"));
            rootLayout->addWidget(nickName_);

            birthday_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            birthday_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Birthday"));
            rootLayout->addWidget(birthday_);

            city_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            city_->setHeader(QT_TRANSLATE_NOOP("sidebar", "City"));
            rootLayout->addWidget(city_);

            country_ = new InfoPlate(mainWidget_, Utils::scale_value(back_button_spacing + button_size));
            country_->setHeader(QT_TRANSLATE_NOOP("sidebar", "Country"));
            rootLayout->addWidget(country_);

            chatEditWidget_ = new QWidget(mainWidget_);
            auto vLayout = emptyVLayout(chatEditWidget_);
            {
                auto hLayout = emptyHLayout();
                hLayout->setAlignment(Qt::AlignLeft);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(button_size + back_button_spacing), 0, QSizePolicy::Fixed));
                auto label = new LabelEx(chatEditWidget_);
                label->setObjectName("editor");
                label->setText(QT_TRANSLATE_NOOP("sidebar", "NAME"));
                hLayout->addWidget(label);
                vLayout->addLayout(hLayout);
            }

            {
                auto hLayout = emptyHLayout();
                hLayout->setAlignment(Qt::AlignLeft);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size), 0, QSizePolicy::Fixed));
                nameEdit_ = new Ui::TextEditEx(chatEditWidget_, Fonts::defaultAppFontFamily(), Utils::scale_value(18), Ui::CommonStyle::getTextCommonColor(), true, true);
                Utils::ApplyStyle(nameEdit_, Ui::CommonStyle::getTextEditStyle());
                nameEdit_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
                nameEdit_->setAutoFillBackground(false);
                nameEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                nameEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
                nameEdit_->setCatchEnter(false);
                nameEdit_->setFrameStyle(QFrame::NoFrame);
                nameEdit_->document()->setDocumentMargin(0);
                nameEdit_->addSpace(Utils::scale_value(4));
                hLayout->addWidget(nameEdit_);
                vLayout->addLayout(hLayout);
            }

            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(name_desc_space), QSizePolicy::Preferred, QSizePolicy::Fixed));

            {
                auto hLayout = emptyHLayout();
                hLayout->setAlignment(Qt::AlignLeft);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(button_size + back_button_spacing), 0, QSizePolicy::Fixed));
                auto label = new LabelEx(chatEditWidget_);
                label->setObjectName("editor");
                label->setText(QT_TRANSLATE_NOOP("sidebar", "DESCRIPTION"));
                hLayout->addWidget(label);
                vLayout->addLayout(hLayout);
            }

            {
                auto hLayout = emptyHLayout();
                hLayout->setAlignment(Qt::AlignLeft);
                hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(back_button_spacing + button_size), 0, QSizePolicy::Fixed));
                descriptionEdit_ = new Ui::TextEditEx(chatEditWidget_, Fonts::defaultAppFontFamily(), Utils::scale_value(18), Ui::CommonStyle::getTextCommonColor(), true, true);
                Utils::ApplyStyle(descriptionEdit_, Ui::CommonStyle::getTextEditStyle());
                descriptionEdit_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
                descriptionEdit_->setAutoFillBackground(false);
                descriptionEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                descriptionEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
                descriptionEdit_->setCatchEnter(false);
                descriptionEdit_->setFrameStyle(QFrame::NoFrame);
                descriptionEdit_->document()->setDocumentMargin(0);
                descriptionEdit_->addSpace(Utils::scale_value(4));
                hLayout->addWidget(descriptionEdit_);
                vLayout->addLayout(hLayout);
            }

            Utils::grabTouchWidget(descriptionEdit_);
            Utils::grabTouchWidget(nameEdit_);
            Utils::grabTouchWidget(chatEditWidget_);
            rootLayout->addWidget(chatEditWidget_);

            Line_ = new LineWidget(mainWidget_, Utils::scale_value(back_button_spacing + button_size), Utils::scale_value(line_vertical_margin), Utils::scale_value(right_margin), Utils::scale_value(line_vertical_margin));
            Utils::grabTouchWidget(Line_);
            rootLayout->addWidget(Line_);

            renameContact_ = new ActionButton(mainWidget_, ":/resources/sidebar_rename_100.png", QT_TRANSLATE_NOOP("sidebar", "Rename"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            renameContact_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(renameContact_);
            rootLayout->addWidget(renameContact_);

            ignoreListButton = new ActionButton(mainWidget_, ":/resources/content_ignorelist_100.png", QT_TRANSLATE_NOOP("sidebar", "Ignored contacts"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            ignoreListButton->setStyleSheet("color: #e30f04;");
            ignoreListButton->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(ignoreListButton);
            rootLayout->addWidget(ignoreListButton);

            attachOldAcc = new ActionButton(mainWidget_, ":/resources/content_oldaccount_100.png", QT_TRANSLATE_NOOP("sidebar", "Connect to ICQ account"), Utils::scale_value(button_height), Utils::scale_value(reverse_margin), Utils::scale_value(button_offset));
            attachOldAcc->setStyleSheet("color: #579e1c;");
            attachOldAcc->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::grabTouchWidget(attachOldAcc);
            rootLayout->addWidget(attachOldAcc);

            rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
            hLayoutMain->addLayout(rootLayout);
            rightWidget_ = new QWidget(mainWidget_);
            rightWidget_->setFixedWidth(0);
            Utils::grabTouchWidget(rightWidget_);
            hLayoutMain->addWidget(rightWidget_);
            vLayoutMain->addLayout(hLayoutMain);

            rootLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(top_margin), QSizePolicy::Preferred, QSizePolicy::Fixed));

        }

        connect(ignoreLabel_, SIGNAL(clicked()), this, SLOT(removeFromIgnore()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);

        connect(addButton_, SIGNAL(clicked()), this, SLOT(addClicked()), Qt::QueuedConnection);
        connect(chatButton_, SIGNAL(clicked()), this, SLOT(chatClicked()), Qt::QueuedConnection);
        connect(callButton_, SIGNAL(clicked()), this, SLOT(callClicked()), Qt::QueuedConnection);
        connect(videoCall_button_, SIGNAL(clicked()), this, SLOT(videoClicked()), Qt::QueuedConnection);
        connect(backButton_, SIGNAL(clicked()), this, SLOT(back()), Qt::QueuedConnection);

        connect(renameContact_, SIGNAL(clicked()), this, SLOT(rename()), Qt::QueuedConnection);
        connect(ignoreListButton, SIGNAL(clicked()), this, SLOT(ignoreList()), Qt::QueuedConnection);
        connect(attachOldAcc, SIGNAL(clicked()), this, SLOT(attachOld()), Qt::QueuedConnection);
        connect(editLabel_, SIGNAL(clicked()), this, SLOT(editClicked()), Qt::QueuedConnection);
        connect(statusLabel_, SIGNAL(clicked()), this, SLOT(statusClicked()), Qt::QueuedConnection);
        connect(saveButton_, SIGNAL(clicked()), this, SLOT(saveClicked()), Qt::QueuedConnection);

        connect(avatar_, &Ui::ContactAvatarWidget::afterAvatarChanged, this, &Ui::ProfilePage::avatarChanged, Qt::QueuedConnection);
    }

    void ProfilePage::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> info)
    {
        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            if (info_->YourRole_ == "admin" || info_->Controlled_ == false)
            {
                avatar_->SetMode(ContactAvatarWidget::Mode::MyProfile);
                if (QApplication::focusWidget() != nameEdit_)
                {
                    nameEdit_->setPlaceholderText(info->Name_);
                    nameEdit_->setPlainText(info->Name_);
                }
                if (QApplication::focusWidget() != descriptionEdit_)
                {
                    descriptionEdit_->setPlaceholderText(info->About_.isEmpty() ? QT_TRANSLATE_NOOP("sidebar", "Add your description") : info_->About_);
                    descriptionEdit_->setPlainText(info->About_);
                }
                chatEditWidget_->show();
            }
            updateWidth();
        }
    }

    void ProfilePage::removeFromIgnore()
    {
        deleteMemberDialog(Logic::getIgnoreModel(), currentAimId_, Logic::IGNORE_LIST, this);
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
        initFor("");
    }

    void ProfilePage::updateStatus()
    {
        statusWidget_->hide();
        if (Logic::getContactListModel()->isChat(currentAimId_))
            return;

        QString state;
        QDateTime lastSeen;
        Logic::ContactItem* cont = Logic::getContactListModel()->getContactItem(currentAimId_);
        if (!cont)
        {
            if (myInfo_)
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
        p.setColor(QPalette::Foreground, QColor(lastSeen.isValid() ? CommonStyle::getTextCommonColor() : "#579e1c"));
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
        Logic::getContactListModel()->addContactToCL(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_profile_page);
    }

    void ProfilePage::chatClicked()
    {
        Logic::getContactListModel()->setCurrent(currentAimId_, -1, true, true);
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

    void ProfilePage::rename()
    {
        QString result_chat_name;

        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);

        auto result = Utils::NameEditor(
            this,
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            QT_TRANSLATE_NOOP("sidebar","Save"),
            isChat ? QT_TRANSLATE_NOOP("sidebar", "Chat name") : QT_TRANSLATE_NOOP("sidebar", "Contact name"),
            result_chat_name);

        if (result)
            Logic::getContactListModel()->renameContact(currentAimId_, result_chat_name);
    }

    void ProfilePage::resizeEvent(QResizeEvent* e)
    {
        mainWidget_->setFixedWidth(e->size().width() - Utils::scale_value(8));
        if (e->size().width() >= Utils::scale_value(596))
        {
            mainBackButtonLayout_->takeAt(0);
            if (mainAvatarLayout_->itemAt(1) && mainAvatarLayout_->itemAt(1)->widget() == avatar_)
                mainAvatarLayout_->takeAt(1);
            avatarBottomSpace_->setVisible(false);
            subBackButtonLayout_->insertWidget(0, backButton_);
            subAvatarLayout_->insertWidget(0, avatar_);
            rightWidget_->setFixedWidth(Utils::scale_value(more_right_margin));
            if (editLayout_->itemAt(1) && editLayout_->itemAt(1)->widget() && editLayout_->itemAt(1)->widget()->property("edit").toBool())
                editLayout_->takeAt(1);
            subEditLayout_->insertWidget(1, editLabel_);
            saveButtonMargin_->hide();
        }
        else
        {
            subBackButtonLayout_->takeAt(0);
            subAvatarLayout_->takeAt(0);
            avatarBottomSpace_->setVisible(true);
            mainBackButtonLayout_->insertWidget(0, backButton_);
            mainAvatarLayout_->insertWidget(1, avatar_);
            rightWidget_->setFixedWidth(Utils::scale_value(0));
            if (subEditLayout_->itemAt(1) && subEditLayout_->itemAt(1)->widget() && subEditLayout_->itemAt(1)->widget()->property("edit").toBool())
                subEditLayout_->takeAt(1);
            editLayout_->insertWidget(1, editLabel_);
            saveButtonMargin_->setVisible(Logic::getContactListModel()->isChat(currentAimId_));
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
        Logic::updateIgnoredModel(temp);
        Logic::getSearchMemberModel()->setChatMembersModel(Logic::getIgnoreModel());

        SelectContactsWidget select_members_dialog_(
            Logic::getIgnoreModel(),
            Logic::MembersWidgetRegim::IGNORE_LIST,
            QT_TRANSLATE_NOOP("popup_window", "Ignored contacts"),
            QT_TRANSLATE_NOOP("popup_window", "Done"), QString(),
            this);
        auto connectId = connect(GetDispatcher(), SIGNAL(recvPermitDeny(bool)), &select_members_dialog_, SLOT(UpdateViewForIgnoreList(bool)), Qt::QueuedConnection);

        Logic::ContactListModel::getIgnoreList();
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
        if (myInfo_)
            statusMenu_->popup(QCursor::pos());
    }

    void ProfilePage::saveClicked()
    {
        if (info_)
        {
            auto name = nameEdit_->getPlainText();
            if (name != info_->Name_ && !name.isEmpty())
            {
                if (Logic::getContactListModel()->isLiveChat(currentAimId_))
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("aimid", currentAimId_);
                    collection.set_value_as_qstring("name", name);
                    Ui::GetDispatcher()->post_message_to_core("chats/mod/name", collection.get());
                }
                else
                {
                    Logic::getContactListModel()->renameChat(currentAimId_, name);
                }
            }
            auto desc = descriptionEdit_->getPlainText();
            if (desc != info_->About_)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("aimid", currentAimId_);
                collection.set_value_as_qstring("about", desc);
                Ui::GetDispatcher()->post_message_to_core("chats/mod/about", collection.get());
            }
        }
        back();
    }

    void ProfilePage::attachOld()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_AttachUin);
    }

    void ProfilePage::recvFlags(int flags)
    {
        connectOldVisible_ = flags & 0x1000;
        attachOldAcc->setVisible(myInfo_ && connectOldVisible_);
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
        if (Logic::getContactListModel()->isChat(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_avatar_changed);
        }
        else
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_avatar_changed);
        }
    }
}
