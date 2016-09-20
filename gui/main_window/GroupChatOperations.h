#pragma once

namespace Logic
{
    class ChatMembersModel;
}

namespace Data
{
    struct Quote;
}

namespace Ui
{
    void createGroupChat(QStringList _members_aimIds);
    bool callChatNameEditor(QWidget* _parent, QString& chat_name);

    void postCreateChatInfoToCore(QString chat_name);
    void postAddChatMembersFromCLModelToCore(QString _aimId);

    void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim, QWidget* _parent);
    
    void forwardMessage(QList<Data::Quote> quotes);
}
