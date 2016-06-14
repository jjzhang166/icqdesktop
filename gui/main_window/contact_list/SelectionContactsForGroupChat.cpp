#include "stdafx.h"
#include "SelectionContactsForGroupChat.h"
#include "../../utils/utils.h"
#include "ContactListModel.h"
#include "SearchModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "SearchWidget.h"
#include "ContactList.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/TextEditEx.h"
#include "../../gui_settings.h"
#include "Common.h"
#include "ChatMembersModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/TextEmojiWidget.h"
#include "../settings/ProfileSettingsWidget.h"
#include "../../utils/InterConnector.h"
#include "../MainWindow.h"
#include "../GroupChatOperations.h"

namespace Ui
{
    const double heightPartOfMainWindowForShortView = 0.4;
    const double heightPartOfMainWindowForFullView = 0.6;

    void SelectContactsWidget::itemClicked(const QString& _current)
    {
        if (Logic::is_delete_members_regim(regim_))
        {
            auto global_cursor_pos = main_dialog_->mapFromGlobal(QCursor::pos());
            auto minXofDeleteImage = ::ContactList::GetXOfRemoveImg(false, true, -1);
            if (global_cursor_pos.x() > minXofDeleteImage)
            {
                deleteMemberDialog(chatMembersModel_, _current, regim_, this);
            }
            else if (regim_ != Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                emit ::Utils::InterConnector::instance().profileSettingsShow(_current);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_members_list);
                if (platform::is_apple())
                {
                    main_dialog_->close();
                }
                else
                {
                    show();
                }
            }
        }
        else
        {
            auto chat_members = Logic::GetChatMembersModel();
            if (!!chat_members && chat_members->is_contact_in_chat(_current))
            {
                deleteMemberDialog(chat_members, _current, regim_, this);
            }
            else
            {
                Logic::GetContactListModel()->ChangeChecked(_current);
                main_dialog_->setButtonActive(Logic::GetContactListModel()->GetCheckedContacts().size() > 0);
                contactList_->update();
            }
        }
    }

    SelectContactsWidget::SelectContactsWidget(Logic::ChatMembersModel* _chatMembersModel, int _regim, const QString& _label_text,
        const QString& _button_text, QWidget* _parent)
        : QDialog(_parent)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , is_short_view_(false)
        , main_widget_(new QWidget(this))
    {
        if (chatMembersModel_)
        {
            chatMembersModel_->is_short_view_ = true;
            is_short_view_ = true;
        }

        auto global_layout = new QVBoxLayout(main_widget_);
        global_layout->setContentsMargins(0, 0, 0, 0);
        global_layout->setMargin(0);
        global_layout->setSpacing(0);
        main_widget_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);

        searchWidget_ = new SearchWidget(true, 0, Utils::scale_value(8));
        Testing::setAccessibleName(searchWidget_, "CreateGroupChat");
        global_layout->addWidget(searchWidget_);

        contactList_ = new ContactList(this, (Logic::MembersWidgetRegim)regim_, chatMembersModel_);
        contactList_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        global_layout->addWidget(contactList_);

        QSpacerItem* contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        global_layout->addSpacerItem(contactsLayoutSpacer);

        auto is_show_search = !Logic::is_delete_members_regim(regim_);// || chatMembersModel_->NeedSearch();
        auto is_show_button = !Logic::is_delete_members_regim(regim_);

        horizontalLineWidget_view_all = new QWidget(this);
        horizontalLineWidget_view_all->setFixedHeight(Utils::scale_value(1));
        QString line_style = QString("background-color: #dadada; margin-left: 16dip; margin-right: 16dip;");
        horizontalLineWidget_view_all->setStyleSheet(Utils::ScaleStyle(line_style, Utils::get_scale_coefficient()));
        global_layout->addWidget(horizontalLineWidget_view_all);

        view_all_spacer1 = new QSpacerItem(1, Utils::scale_value(10));
        global_layout->addSpacerItem(view_all_spacer1);

        view_all = new TextEmojiLabel(this);
        if (Logic::is_delete_members_regim(regim_))
            view_all->setText(QT_TRANSLATE_NOOP("groupchat_pages","View all ") + "("+ QString::number(chatMembersModel_->get_members_count()) +")");
        QString view_all_style = "font-family: " + Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI) + "; "
            +"padding: 0 0 0 0dip; "
            +"font-size: 15dip; "
            +"color: #579e1c";
        view_all->setMouseTracking(true);
        view_all->setCursor(Qt::PointingHandCursor);
        view_all->setStyleSheet(Utils::ScaleStyle(view_all_style, Utils::get_scale_coefficient()));
        view_all->setMargin(0);
        view_all->setContentsMargins(0, 0, 0, 0);
        if (!platform::is_apple())
            view_all->setSizeToBaseline(Utils::scale_value(18));
        view_all->setSizeToOffset(Utils::scale_value(58));
        global_layout->addWidget(view_all);

        view_all_spacer2 = new QSpacerItem(1, Utils::scale_value(12));
        global_layout->addSpacerItem(view_all_spacer2);

        connect(view_all, SIGNAL(clicked()), this, SLOT(onViewAllMembers()), Qt::QueuedConnection);

        if (!is_show_search)
            searchWidget_->setVisible(false);

        if (!Logic::is_delete_members_regim(regim_) || chatMembersModel_->get_members_count() <= ::Logic::InitMembersLimit)
        {
            hideShowAllButton();
        }
        // TODO : use SetView here
        main_widget_->setStyleSheet("background-color: white;");
        main_dialog_.reset(new GeneralDialog(main_widget_, Utils::InterConnector::instance().getMainWindow()));

        if (is_show_search)
        {
            main_dialog_->addHead();
            main_dialog_->addLabel(_label_text);
        }

        if (is_show_button)
            main_dialog_->addAcceptButton(_button_text, Utils::scale_value(16));

        Testing::setAccessibleName(main_dialog_.get(), "SelectContactsWidget");

        connect(contactList_, &ContactList::searchEnd, searchWidget_, &SearchWidget::searchCompleted, Qt::QueuedConnection);
        connect(contactList_, &ContactList::itemSelected, this, &SelectContactsWidget::itemClicked, Qt::QueuedConnection);

        connect(searchWidget_, &SearchWidget::searchBegin, this,  &SelectContactsWidget::searchBegin, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::searchEnd, this,  &SelectContactsWidget::searchEnd, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::enterPressed, contactList_, &ContactList::searchResult, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::upPressed, contactList_, &ContactList::searchUpPressed, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::downPressed, contactList_, &ContactList::searchDownPressed, Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::GetCurrentSearchModel(regim_), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::searchEnd, this, &SelectContactsWidget::searchEnd, Qt::QueuedConnection);

        if (Logic::is_delete_members_regim(regim_))
            connect(contactList_, SIGNAL(addContactClicked()), this, SLOT(onViewAllMembers()), Qt::QueuedConnection);

        contactList_->changeTab(Ui::CurrentTab::SEARCH);
        Logic::GetCurrentSearchModel(regim_)->searchPatternChanged("");
        searchWidget_->SetShowButton(false);
    }

    SelectContactsWidget::~SelectContactsWidget()
    {
        main_dialog_.reset();
    }

    void SelectContactsWidget::hideShowAllButton()
    {
        horizontalLineWidget_view_all->setVisible(false);
        view_all->setVisible(false);
        view_all_spacer1->changeSize(1, 0);
        view_all_spacer2->changeSize(1, 0);
    }

    void SelectContactsWidget::searchEnd()
    {
        Logic::GetCurrentSearchModel(regim_)->searchPatternChanged("");
        //contactList_->setSearchMode(false);
    }

    void SelectContactsWidget::searchBegin()
    {
        contactList_->setSearchMode(true);
    }

    QRect SelectContactsWidget::CalcSizes() const
    {
        int new_width = -1;
        if (Logic::is_delete_members_regim(regim_))
            new_width = ::ContactList::ItemWidth(false /* fromAlert */, false /* _isWithCheckBox */, is_short_view_).px();
        else
            new_width = ::ContactList::ItemWidth(false /* fromAlert */, true /* _isWithCheckBox */, false /* _isShortView */).px();

        auto is_show_search = !Logic::is_delete_members_regim(regim_) || !is_short_view_;
        double koeff_height = !is_show_search ? heightPartOfMainWindowForShortView : heightPartOfMainWindowForFullView;

        auto new_height = ::ContactList::ItemLength(false, koeff_height, ::ContactList::dip(0)).px();
        auto extra_height = is_show_search ? searchWidget_->sizeHint().height() : 0;
        if (!Logic::is_delete_members_regim(regim_))
            extra_height += ::ContactList::dip(42).px();

        if (Logic::is_delete_members_regim(regim_)
            && is_short_view_
            && chatMembersModel_->get_members_count() >= Logic::InitMembersLimit)
        {
            extra_height += ::ContactList::dip(52).px();
        }

        auto itemHeight = ::ContactList::ContactItemHeight();
        int count = (new_height - extra_height) / itemHeight;

        int cl_height = (count + 0.5) * itemHeight;
        if (Logic::is_delete_members_regim(regim_))
        {
            count = chatMembersModel_->rowCount();
            cl_height = std::min(cl_height, count * itemHeight + ::ContactList::dip(1).px());
            if (count == 0)
                extra_height += itemHeight / 2;
        }

        if (regim_ == Logic::MembersWidgetRegim::IGNORE_LIST && chatMembersModel_->rowCount() == 0)
        {
            extra_height += itemHeight / 2 + 5;
        }
        new_height = extra_height + cl_height;
        
        return QRect(0, 0, new_width, new_height);
    }

    bool SelectContactsWidget::show(int _x, int _y)
    {
        x_ = _x;
        y_ = _y;

        Utils::setWidgetPopup(main_dialog_.get(), true);//(platform::is_apple() && !is_short_view_) ? false : true);

        Logic::GetContactListModel()->setIsWithCheckedBox(true);

        if (Logic::is_delete_members_regim(regim_))
            contactList_->changeTab(ALL);
        else
            Logic::GetCurrentSearchModel(regim_)->searchPatternChanged("");

        main_dialog_->setButtonActive(false);

        auto new_rect = CalcSizes();
        main_widget_->setFixedWidth(new_rect.width());
        main_widget_->setFixedHeight(new_rect.height());

        auto result = main_dialog_->showInPosition(x_, y_);
        if (result == QDialog::Accepted)
        {
            if (Logic::GetContactListModel()->GetCheckedContacts().size() == 0)
            {
                result = QDialog::Rejected;
            }
        }
        else
        {
            Logic::GetContactListModel()->clearChecked();
        }
        Logic::GetContactListModel()->setIsWithCheckedBox(false);
        searchWidget_->ClearInput();
        return result;
    }

    bool SelectContactsWidget::show()
    {
        return show(default_invalid_coord, default_invalid_coord);
    }

    void SelectContactsWidget::setView(bool _is_short_view)
    {
        chatMembersModel_->is_short_view_ = _is_short_view;
        searchWidget_->setVisible(!_is_short_view);
        hideShowAllButton();
        is_short_view_ = _is_short_view;
    }

    void SelectContactsWidget::onViewAllMembers()
    {
        setView(false);
        chatMembersModel_->load_all_members();
        x_ = y_ = default_invalid_coord;
        UpdateView();
    }

    void SelectContactsWidget::UpdateView()
    {
        auto new_rect = CalcSizes();
        main_widget_->setFixedWidth(new_rect.width());
        main_widget_->setFixedHeight(new_rect.height());
    }

    void SelectContactsWidget::UpdateViewForIgnoreList(bool _is_empty_ignore_list)
    {
        contactList_->setEmptyIgnoreLabelVisible(_is_empty_ignore_list);
        UpdateView();
    }

    void SelectContactsWidget::UpdateMembers()
    {
        main_dialog_->update();
    }
}