#pragma once

#include "ContactItem.h"
#include "ContactList.h"
#include "ChatMembersModel.h"
#include "AbstractSearchModel.h"
#include "../../types/contact.h"

namespace Logic
{
    class SearchMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

Q_SIGNALS:
        void results();

        public Q_SLOTS:
            void searchPatternChanged(QString) override;
            void searchResult(QStringList);

    public:
        SearchMembersModel(QObject *parent);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        void setFocus() override;
        const QStringList& GetPattern() const;
        void emitChanged(int first, int last) override;
        void SetChatMembersModel(ChatMembersModel* _members_model);

    private:
        mutable std::vector<Data::ChatMemberInfo> Match_;
        QStringList SearchPatterns_;
        bool SearchRequested_;
        ChatMembersModel* chat_members_model_;
    };
    
    SearchMembersModel* GetSearchMemberModel();
}
