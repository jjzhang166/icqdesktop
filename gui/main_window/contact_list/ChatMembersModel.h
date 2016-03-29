#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "ContactItem.h"

namespace core
{
    struct icollection;
}

namespace Data
{
    class ChatMemberInfo;
}

namespace Logic
{
    const int InitMembersLimit = 20;
    const int MaxMembersLimit = 1000;

    class ChatMembersModel : public CustomAbstractListModel
    {
        Q_OBJECT
        
    Q_SIGNALS:
        void results();

    public Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);

    private Q_SLOTS:
        void avatarLoaded(QString);

    public:
        ChatMembersModel(std::shared_ptr<Data::ChatInfo> &info, QObject *parent);

        ~ChatMembersModel();

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;

        const Data::ChatMemberInfo* getMemberItem(const QString& _aimId) const;
        void remove_contact_from_member_list(const QString& _aimid);
        int get_members_count() const;
        void load_all_members();
        bool is_full_list_loaded_;
        bool is_short_view_;

        bool is_contact_in_chat(const QString& AimdId_) const;
        QString get_chat_aimid() const;
        void update_info(std::shared_ptr<Data::ChatInfo> &_info, bool _is_all_chat_info);
        static qint64  load_all_members(QString _aimid, int _limit, QObject* _recv);
        static bool receive_members(qint64 _send_seq, qint64 _seq, QObject* _recv);
        unsigned int get_visible_rows_count() const;

    private:
        mutable std::vector<Data::ChatMemberInfo>   members_;
        int                                         members_count_;
        std::shared_ptr<Data::ChatInfo>             info_;
        QString                                     AimId_;
        qint64									    ChatInfoSequence_;

        friend class SearchMembersModel;
    };

    ChatMembersModel* GetChatMembersModel();
    void SetChatMembersModel(ChatMembersModel*);

    void UpdateIgnoredModel(const QVector<QString>& _ignored_list);
    ChatMembersModel* GetIgnoreModel();
}