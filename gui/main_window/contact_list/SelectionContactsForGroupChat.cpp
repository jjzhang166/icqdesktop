#include "stdafx.h"
#include "SelectionContactsForGroupChat.h"

#include "ChatMembersModel.h"
#include "ContactList.h"
#include "ContactListItemRenderer.h"
#include "ContactListModel.h"
#include "SearchModel.h"
#include "SearchWidget.h"
#include "../GroupChatOperations.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    const double heightPartOfMainWindowForFullView = 0.6;

    bool SelectContactsWidget::forwardConfirmed(QString aimId)
    {
        puts(aimId.toStdString().c_str());
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to forward messages to <USER>?");
        if (auto contactItemWrapper = Logic::getContactListModel()->getContactItem(aimId))
        {
            if (auto contactItem = contactItemWrapper->Get())
            {
                text.replace("<USER>", contactItem->GetDisplayName());
            }
        }
        text.replace("<USER>", contactList_->getSelectedAimid()); // Just in case - if previously it couldn't change placeholder
     
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
                                                            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                                                            QT_TRANSLATE_NOOP("popup_window", "Yes"),
                                                            text,
                                                            QT_TRANSLATE_NOOP("popup_window", "Confirmation"),
                                                            NULL);

        return confirmed;
    }

    void SelectContactsWidget::enterPressed()
    {
        assert(contactList_);

        if (isShareLinkMode() || isShareTextMode())
        {
            selectedContact_ = contactList_->getSelectedAimid();
            bool confirmed = true;
            if (!selectedContact_.isEmpty())
            {
                mainDialog_->close();
                confirmed = forwardConfirmed(selectedContact_);
            }
            if (confirmed)
            {
                if (isShareLinkMode())
                {
                    assert(!bottomText_.isEmpty());
                    mainDialog_->accept();
                }
                else if (isShareTextMode())
                {
                    mainDialog_->accept();
                }
            }
            return;
        }

        contactList_->searchResult();
    }

    void SelectContactsWidget::itemClicked(const QString& _current)
    {
        selectedContact_ = _current;

        if (isShareLinkMode() || isShareTextMode())
        {
            bool confirmed = true;
            if (!selectedContact_.isEmpty())
            {
                mainDialog_->close();
                confirmed = forwardConfirmed(selectedContact_);
            }
            if (confirmed)
            {
                if (isShareLinkMode())
                {
                    assert(!bottomText_.isEmpty());
                    mainDialog_->accept();
                }
                else if (isShareTextMode())
                {
                    mainDialog_->accept();
                }
            }
            return;
        }

        if (Logic::is_delete_members_regim(regim_))
        {
            auto globalCursorPos = mainDialog_->mapFromGlobal(QCursor::pos());
            auto minXofDeleteImage = ::ContactList::GetXOfRemoveImg(false, true, -1);

            if (globalCursorPos.x() > minXofDeleteImage)
            {
                deleteMemberDialog(chatMembersModel_, _current, regim_, this);
                return;
            }

            if (regim_ != Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                emit ::Utils::InterConnector::instance().profileSettingsShow(_current);
                if (platform::is_apple())
                {
                    mainDialog_->close();
                }
                else
                {
                    show();
                }
            }

            return;
        }

        auto chatMembers = Logic::getChatMembersModel();
        if (!!chatMembers && chatMembers->isContactInChat(_current))
        {
            deleteMemberDialog(chatMembers, _current, regim_, this);
            return;
        }

        Logic::getContactListModel()->setChecked(_current, !Logic::getContactListModel()->getIsChecked(_current));
        mainDialog_->setButtonActive(Logic::getContactListModel()->GetCheckedContacts().size() > 0);
        contactList_->update();
    }

    SelectContactsWidget::SelectContactsWidget(const QString& _labelText, QWidget* _parent)
        : QDialog(_parent)
        , regim_(Logic::MembersWidgetRegim::SHARE_TEXT)
        , chatMembersModel_(nullptr)
        , isShortView_(false)
        , mainWidget_(new QWidget(this))
        , sortCL_(true)
        , handleKeyPressEvents_(true)
    {
        init(_labelText);
    }

    SelectContactsWidget::SelectContactsWidget(Logic::ChatMembersModel* _chatMembersModel, int _regim, const QString& _labelText,
        const QString& _buttonText, const QString& _bottomText, QWidget* _parent, bool _handleKeyPressEvents/* = true*/)
        : QDialog(_parent)
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , isShortView_(false)
        , mainWidget_(new QWidget(this))
        , bottomText_(_bottomText)
        , handleKeyPressEvents_(_handleKeyPressEvents)
    {
        init(_labelText, _buttonText);
    }

    void SelectContactsWidget::init(const QString& _labelText, const QString& _buttonText)
    {
        if (chatMembersModel_)
        {
            chatMembersModel_->isShortView_ = true;
            isShortView_ = true;
        }

        auto globalLayout = new QVBoxLayout(mainWidget_);
        globalLayout->setContentsMargins(0, 0, 0, 0);
        globalLayout->setMargin(0);
        globalLayout->setSpacing(0);
        mainWidget_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);

        searchWidget_ = new SearchWidget(true, 0, Utils::scale_value(8));
        Testing::setAccessibleName(searchWidget_, "CreateGroupChat");
        globalLayout->addWidget(searchWidget_);

        contactList_ = new ContactList(this, (Logic::MembersWidgetRegim)regim_, chatMembersModel_);
        contactList_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        contactList_->setTransparent(true);
        globalLayout->addWidget(contactList_);

        QSpacerItem* contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        globalLayout->addSpacerItem(contactsLayoutSpacer);

        // TODO : use SetView here
        mainDialog_.reset(new GeneralDialog(mainWidget_, Utils::InterConnector::instance().getMainWindow(), handleKeyPressEvents_));

        connect(mainDialog_.get(), &QDialog::finished, this, &SelectContactsWidget::finished);

        mainDialog_->addHead();
        mainDialog_->addLabel(_labelText);

        const auto bottomPanelMargins = Utils::scale_value(16);

        if (!isShareTextMode() && !bottomText_.isEmpty())
        {
            mainDialog_->addBottomLabel(bottomText_, bottomPanelMargins);
        }

        const auto is_show_button = !isShareTextMode() && !Logic::is_delete_members_regim(regim_);
        if (is_show_button)
        {
            mainDialog_->addAcceptButton(_buttonText, bottomPanelMargins, isShareLinkMode());
        }

        Testing::setAccessibleName(mainDialog_.get(), "SelectContactsWidget");

        connect(contactList_, &ContactList::searchEnd, searchWidget_, &SearchWidget::searchCompleted, Qt::QueuedConnection);
        connect(contactList_, &ContactList::itemSelected, this, &SelectContactsWidget::itemClicked, Qt::QueuedConnection);

        connect(searchWidget_, &SearchWidget::searchBegin, this,  &SelectContactsWidget::searchBegin, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::searchEnd, this,  &SelectContactsWidget::searchEnd, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::enterPressed, this, &SelectContactsWidget::enterPressed, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::upPressed, contactList_, &ContactList::searchUpPressed, Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::downPressed, contactList_, &ContactList::searchDownPressed, Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::getCurrentSearchModel(regim_), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(searchWidget_, &SearchWidget::searchEnd, this, &SelectContactsWidget::searchEnd, Qt::QueuedConnection);

        if (Logic::is_delete_members_regim(regim_))
            connect(contactList_, SIGNAL(addContactClicked()), this, SLOT(onViewAllMembers()), Qt::QueuedConnection);

        contactList_->changeTab(Ui::CurrentTab::SEARCH);
        // Logic::getCurrentSearchModel(regim_)->searchPatternChanged("");
        searchWidget_->setShowButton(false);
    }

    const QString& SelectContactsWidget::getSelectedContact() const
    {
        return selectedContact_;
    }

    SelectContactsWidget::~SelectContactsWidget()
    {
    }

    bool SelectContactsWidget::isCheckboxesVisible() const
    {
        if (Logic::is_delete_members_regim(regim_))
        {
            return false;
        }

        if (isShareLinkMode() || isShareTextMode())
        {
            return false;
        }

        return true;
    }

    bool SelectContactsWidget::isShareLinkMode() const
    {
        return (regim_ == Logic::MembersWidgetRegim::SHARE_LINK);
    }

    bool SelectContactsWidget::isShareTextMode() const
    {
        return (regim_ == Logic::MembersWidgetRegim::SHARE_TEXT);
    }

    void SelectContactsWidget::searchEnd()
    {
        Logic::getCurrentSearchModel(regim_)->searchPatternChanged("");
        //contactList_->setSearchMode(false);
    }

    void SelectContactsWidget::searchBegin()
    {
        contactList_->setSearchMode(true);
    }

    QRect SelectContactsWidget::CalcSizes() const
    {
        int newWidth = -1;
        if (Logic::is_delete_members_regim(regim_))
            newWidth = ::ContactList::ItemWidth(false /* fromAlert */, isCheckboxesVisible() /* _isWithCheckBox */, isShortView_).px();
        else
            newWidth = ::ContactList::ItemWidth(false /* fromAlert */, isCheckboxesVisible() /* _isWithCheckBox */, false /* _isShortView */).px();

        auto newHeight = ::ContactList::ItemLength(false, heightPartOfMainWindowForFullView, ::ContactList::dip(0)).px();
        auto extraHeight = searchWidget_->sizeHint().height();
        if (!Logic::is_delete_members_regim(regim_))
            extraHeight += ::ContactList::dip(42).px();

        if (Logic::is_delete_members_regim(regim_)
            && isShortView_
            && chatMembersModel_->getMembersCount() >= Logic::InitMembersLimit)
        {
            extraHeight += ::ContactList::dip(52).px();
        }

        auto itemHeight = ::ContactList::ContactItemHeight();
        int count = (newHeight - extraHeight) / itemHeight;

        int clHeight = (count + 0.5) * itemHeight;
        if (Logic::is_delete_members_regim(regim_))
        {
            count = chatMembersModel_->rowCount();
            clHeight = std::min(clHeight, count * itemHeight + ::ContactList::dip(1).px());
            if (count == 0)
                extraHeight += itemHeight / 2;
        }

        if (regim_ == Logic::MembersWidgetRegim::IGNORE_LIST && chatMembersModel_->rowCount() == 0)
        {
            extraHeight += itemHeight / 2 + 5;
        }
        newHeight = extraHeight + clHeight;

        return QRect(0, 0, newWidth, newHeight);
    }

    bool SelectContactsWidget::show(int _x, int _y)
    {
        x_ = _x;
        y_ = _y;

#ifdef __APPLE__
        Utils::setWidgetPopup(mainDialog_.get(), true);
#endif //__APPLE__

        Logic::getContactListModel()->setIsWithCheckedBox(!isShareLinkMode() && !isShareTextMode());

        if (Logic::is_delete_members_regim(regim_))
            contactList_->changeTab(ALL);
        else
            Logic::getCurrentSearchModel(regim_)->searchPatternChanged("");

        mainDialog_->setButtonActive(isShareLinkMode() || isShareTextMode());

        auto newRect = CalcSizes();
        mainWidget_->setFixedWidth(newRect.width());
        mainWidget_->setFixedHeight(newRect.height());

        searchWidget_->setFocus();

        auto result = mainDialog_->showInPosition(x_, y_);
        if (result)
        {
            if (!isShareTextMode()
                && !isShareLinkMode()
                && Logic::getContactListModel()->GetCheckedContacts().empty())
            {
                result = false;
            }
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }
        Logic::getContactListModel()->setIsWithCheckedBox(false);
        searchWidget_->clearInput();
        return result;
    }

    bool SelectContactsWidget::show()
    {
        return show(defaultInvalidCoord, defaultInvalidCoord);
    }

    void SelectContactsWidget::setView(bool _isShortView)
    {
        chatMembersModel_->isShortView_ = _isShortView;
        searchWidget_->setVisible(!_isShortView);
        isShortView_ = _isShortView;
    }

    void SelectContactsWidget::onViewAllMembers()
    {
        setView(false);
        chatMembersModel_->loadAllMembers();
        x_ = y_ = defaultInvalidCoord;
        UpdateView();
    }

    void SelectContactsWidget::UpdateView()
    {
        auto newRect = CalcSizes();
        mainWidget_->setFixedWidth(newRect.width());
        mainWidget_->setFixedHeight(newRect.height());
    }

    void SelectContactsWidget::UpdateViewForIgnoreList(bool _isEmptyIgnoreList)
    {
        contactList_->setEmptyIgnoreLabelVisible(_isEmptyIgnoreList);
        UpdateView();
    }

    void SelectContactsWidget::UpdateMembers()
    {
        mainDialog_->update();
    }

    void SelectContactsWidget::setSort(bool _isClSorting)
    {
        sortCL_ = _isClSorting;
        Logic::getCurrentSearchModel(regim_)->setSort(sortCL_);
    }

    void SelectContactsWidget::finished()
    {
        Logic::getCurrentSearchModel(regim_)->setSort(true);
    }
}