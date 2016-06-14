#include "stdafx.h"

#include "GroupChatOperations.h"
#include "../core_dispatcher.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/ChatMembersModel.h"
#include "../utils/utils.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "MainPage.h"
#include "../gui_settings.h"
#include "contact_list/ContactList.h"
#include "contact_list/SelectionContactsForGroupChat.h"

namespace Ui
{
    void createGroupChat(QStringList _members_aimIds)
    {
        SelectContactsWidget select_members_dialog(nullptr, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("groupchat_pages", "Create Groupchat"), QT_TRANSLATE_NOOP("groupchat_pages", "Next"), Ui::MainPage::instance());

        for (auto& member_aimId : _members_aimIds)
        {
            Logic::GetContactListModel()->setChecked(member_aimId);
        }

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            if (Logic::GetContactListModel()->GetCheckedContacts().size() == 1)
            {
                const auto& aimid = Logic::GetContactListModel()->GetCheckedContacts()[0].get_aimid();
                Ui::MainPage::instance()->selectRecentChat(aimid);
            }
            else
            {
                QString chat_name;
                if (callChatNameEditor(Ui::MainPage::instance(), chat_name))
                {
                    Ui::MainPage::instance()->openCreatedGroupChat();
                    postCreateChatInfoToCore(chat_name);

                    // contact_list_widget_->changeTab(Ui::CurrentTab::RECENTS);
                    // QString chat_id = "*@chat.agent";
                    // emit contactlListWidget_->itemSelected(chat_id);
                }
            }
        }
        Logic::GetContactListModel()->clearChecked();
        Ui::MainPage::instance()->clearSearchMembers();
    }

    bool callChatNameEditor(QWidget* _parent, QString& chat_name)
    {
        auto selectedContacts = Logic::GetContactListModel()->GetCheckedContacts();
        chat_name = MyInfo()->friendlyName();
        if (chat_name.isEmpty())
            chat_name = MyInfo()->aimId();

        for (int i = 0; i < 2; ++i)
        {
            chat_name += ", " + selectedContacts[i].Get()->GetDisplayName();
        }

        QString result_chat_name;

        auto result = Utils::NameEditor(
            _parent,
            chat_name,
            QT_TRANSLATE_NOOP("groupchat_pages","Done"),
            QT_TRANSLATE_NOOP("popup_window", "Chat name"),
            result_chat_name);

        if (chat_name != result_chat_name)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_create_rename);

        chat_name = result_chat_name.isEmpty() ? chat_name : result_chat_name;
        return result;
    }

    void postCreateChatInfoToCore(QString chat_name)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        QStringList chat_members;
        auto selectedContacts = Logic::GetContactListModel()->GetCheckedContacts();
        for (const auto& contact : selectedContacts)
        {
            chat_members.push_back(contact.get_aimid());
        }

        collection.set_value_as_string("m_chat_name", chat_name.toUtf8().data(), chat_name.toUtf8().size());

        core::ifptr<core::iarray> members_array(collection->create_array());
        members_array->reserve((int)chat_members.size());
        for (int i = 0; i < chat_members.size(); ++i)
        {
            auto member = chat_members[i];
            core::ifptr<core::ivalue> val(collection->create_value());
            val->set_as_string(member.toStdString().c_str(), (int)member.length());
            members_array->push_back(val.get());
        }
        collection.set_value_as_array("m_chat_members", members_array.get());
        Ui::GetDispatcher()->post_message_to_core("add_chat", collection.get());
    }

    void postAddChatMembersFromCLModelToCore(QString _aimId)
    {
        auto selectedContacts = Logic::GetContactListModel()->GetCheckedContacts();
        Logic::GetContactListModel()->clearChecked();

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        QStringList chat_members;
        for (const auto& contact : selectedContacts)
        {
            chat_members.push_back(contact.get_aimid());
        }
        collection.set_value_as_string("aimid", _aimId.toUtf8().data(), _aimId.toUtf8().size());
        collection.set_value_as_string("m_chat_members_to_add", chat_members.join(";").toStdString());
        Ui::GetDispatcher()->post_message_to_core("add_members", collection.get());
    }

    void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim, QWidget* _parent)
    {
        QString text;
        
        if (_regim == Logic::MembersWidgetRegim::DELETE_MEMBERS || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this chat?");
        else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?");

        auto left_button_text = QT_TRANSLATE_NOOP("popup_window", "Cancel");
        auto right_button_text = QT_TRANSLATE_NOOP("popup_window", "Delete");

        auto member = _model->getMemberItem(current);
        auto user_name = member->NickName_;
        auto label = user_name.isEmpty() ? member->AimId_ : user_name;

        if (Utils::GetConfirmationWithTwoButtons(left_button_text, right_button_text, text, label, _parent))
        {
            if (_regim == Logic::MembersWidgetRegim::DELETE_MEMBERS || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            {
                auto aimId_ = _model->get_chat_aimid();
                ::Ui::gui_coll_helper collection(::Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_string("aimid", aimId_.toUtf8().data(), aimId_.toUtf8().size());
                collection.set_value_as_string("m_chat_members_to_remove", current.toStdString());
                Ui::GetDispatcher()->post_message_to_core("remove_members", collection.get());
            }
            else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                Logic::GetContactListModel()->ignore_contact(current, false);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_remove);
                // NOTE : when delete from ignore list, we don't need add contact to CL
            }
        }
    }
}
