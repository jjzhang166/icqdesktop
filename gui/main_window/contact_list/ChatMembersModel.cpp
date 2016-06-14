#include "stdafx.h"
#include "ChatMembersModel.h"
#include "SearchMembersModel.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "../history_control/MessagesModel.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "contact_profile.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../my_info.h"
#include "ContactListModel.h"
#include "../../types/chat.h"
#include "../../my_info.h"

namespace Logic
{
    ChatMembersModel::ChatMembersModel(QObject *parent)
        : is_short_view_(true)
        , selectEnabled_(true)
    {
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
    }

    ChatMembersModel::ChatMembersModel(std::shared_ptr<Data::ChatInfo>& _info, QObject *_parent)
        : CustomAbstractListModel(_parent)
        , is_short_view_(true)
        , selectEnabled_(true)
    {
        update_info(_info, false);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
    }

    ChatMembersModel::~ChatMembersModel()
    {
    }

    unsigned int ChatMembersModel::get_visible_rows_count() const
    {
        int current_count = (int)members_.size();

        if (is_short_view_)
            current_count = std::min(Logic::InitMembersLimit, current_count);
        return current_count;
    }

    int ChatMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)get_visible_rows_count();
    }

    QVariant ChatMembersModel::data(const QModelIndex & ind, int role) const
    {
        int current_count = get_visible_rows_count();

        if (!ind.isValid() || (role != Qt::DisplayRole && !Testing::isAccessibleRole(role)) || ind.row() > current_count)
            return QVariant();
        Data::ChatMemberInfo* ptr = &(members_[ind.row()]);

        if (Testing::isAccessibleRole(role))
            return ptr->AimId_;

        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags ChatMembersModel::flags(const QModelIndex &) const
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled;
        if (selectEnabled_)
            return flags |= Qt::ItemIsSelectable;
        
        return flags;
    }

    void ChatMembersModel::setSelectEnabled(bool value)
    {
        selectEnabled_ = value;
    }

    const Data::ChatMemberInfo* ChatMembersModel::getMemberItem(const QString& _aimId) const
    {
        for (auto& item : members_)
        {
            if (item.AimId_ == _aimId)
                return &item;
        }

        return nullptr;
    }

    int ChatMembersModel::get_members_count() const
    {
        return members_count_;
    }

    void ChatMembersModel::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (receive_members(ChatInfoSequence_, _seq, this))
        {
            update_info(_info, true);
            is_full_list_loaded_ = true;
            emit dataChanged(index(0), index((int)members_.size()));
            emit results();
        }
    }

    void ChatMembersModel::avatarLoaded(QString aimid)
    {
        int i = 0;
        for (auto iter : members_)
        {
            if (iter.AimId_ == aimid)
            {
                emit dataChanged(index(i), index(i));
                break;
            }
            ++i;
        }
    }

    void ChatMembersModel::chatBlocked(QList<Data::ChatMemberInfo> blocked)
    {
        members_.clear();
        for (auto iter : blocked)
            members_.push_back(iter);

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::load_all_members(const QString& aimId, int count)
    {
        ChatInfoSequence_ = load_all_members(aimId, count, this);
    }

    void ChatMembersModel::admins_only()
    {
        for (auto iter = members_.begin(); iter != members_.end();)
        {
            if (iter->Role_ != "admin" && iter->Role_ != "moder")
                iter = members_.erase(iter);
            else
                ++iter;
        }

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::load_blocked()
    {
        members_.clear();
        emit dataChanged(index(0), index(0));
        connect(Ui::GetDispatcher(), SIGNAL(chatBlocked(QList<Data::ChatMemberInfo>)), this, SLOT(chatBlocked(QList<Data::ChatMemberInfo>)), Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        Ui::GetDispatcher()->post_message_to_core("chats/blocked/get", collection.get());
    }

    void ChatMembersModel::init_for_single(const QString& aimId)
    {
        members_.clear();
        Data::ChatMemberInfo info;
        info.AimId_ = aimId;
        info.NickName_ = Logic::GetContactListModel()->getDisplayName(aimId);
        members_.push_back(info);
        info.AimId_ = Ui::MyInfo()->aimId();
        info.NickName_ = Ui::MyInfo()->friendlyName();
        members_.push_back(info);
        emit dataChanged(index(0), index(1));
    }

    void ChatMembersModel::load_all_members()
    {
        ChatInfoSequence_ = load_all_members(AimId_, members_count_, this);
    }

    bool ChatMembersModel::receive_members(qint64 _send_seq, qint64 _seq, QObject* _recv)
    {
        if (_seq != _send_seq)
            return false;
        disconnect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), _recv, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)));
        disconnect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), _recv, SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)));
        return true;
    }

    qint64 ChatMembersModel::load_all_members(QString _aimid, int _limit, QObject* _recv)
    {
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), _recv, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), _recv,
            SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)), Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", _aimid);
        collection.set_value_as_int("limit", _limit);
        return Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
    }

    void ChatMembersModel::update_info(std::shared_ptr<Data::ChatInfo>& _info, bool _is_all_chat_info)
    {
        AimId_ = _info->AimId_;
        YourRole_ = _info->YourRole_;
        members_count_ = _info->MembersCount_;
        members_.clear();
        for (auto item : _info->Members_)
        {
            members_.emplace_back(item);
        }

        emit dataChanged(index(0), index(members_.size()));

        is_full_list_loaded_ = members_count_ <= InitMembersLimit;

        if (_is_all_chat_info)
            is_full_list_loaded_ = _is_all_chat_info;
    }

    static ChatMembersModel* ChatMembersModel_ = NULL;

    ChatMembersModel* GetChatMembersModel()
    {
        return ChatMembersModel_;
    }

    void SetChatMembersModel(ChatMembersModel* _model)
    {
        ChatMembersModel_ = _model;
        GetSearchMemberModel()->SetChatMembersModel(ChatMembersModel_);
    }

    bool ChatMembersModel::is_contact_in_chat(const QString& _aimdId) const
    {
        // TODO : use hash-table here
        for (auto& member : members_)
            if (member.AimId_ == _aimdId)
                return true;
        return false;
    }

    QString ChatMembersModel::get_chat_aimid() const
    {
        return AimId_;
    }

    bool ChatMembersModel::is_admin() const
    {
        return (YourRole_ == "admin");
    }

    bool ChatMembersModel::is_moder() const
    {
        return (YourRole_ == "moder");
    }

    void UpdateIgnoredModel(const QVector<QString>& _ignored_list)
    {
        auto il = _ignored_list;

        auto info = std::make_shared<Data::ChatInfo>();
        info->Members_.reserve(il.size());
        info->MembersCount_ = il.size();
        for (auto aimid : il)
        {
            Data::ChatMemberInfo member;
            member.AimId_ = aimid;
            info->Members_.append(member);
        }

        GetIgnoreModel()->update_info(info, true);
        emit GetIgnoreModel()->results();
    }

    ChatMembersModel* GetIgnoreModel()
    {
        auto chatInfo = std::make_shared<Data::ChatInfo>();
        static std::unique_ptr<ChatMembersModel> ignore_model(new ChatMembersModel(chatInfo, NULL));
        return ignore_model.get();
    }
}