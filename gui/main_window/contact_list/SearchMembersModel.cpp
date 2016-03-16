#include "stdafx.h"
#include "SearchMembersModel.h"
#include "SearchModel.h"
#include "ChatMembersModel.h"
#include "ContactItem.h"
#include "AbstractSearchModel.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

namespace Logic
{
    SearchMembersModel::SearchMembersModel(QObject *parent)
        : AbstractSearchModel(parent)
        , SearchRequested_(false)
        , chat_members_model_(NULL)
    {
        // connect(Ui::GetDispatcher(), SIGNAL(searchResult(QStringList)), this, SLOT(searchResult(QStringList)), Qt::QueuedConnection);
    }

    int SearchMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)Match_.size();
    }

    QVariant SearchMembersModel::data(const QModelIndex & ind, int role) const
    {
        auto current_count = Match_.size();

        if (!ind.isValid() || (role != Qt::DisplayRole && !Testing::isAccessibleRole(role)) || (unsigned)ind.row() > current_count)
            return QVariant();

        Data::ChatMemberInfo* ptr = &(Match_[ind.row()]);
        
        if (Testing::isAccessibleRole(role))
            return Match_[ind.row()].AimdId_;
        
        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags SearchMembersModel::flags(const QModelIndex &) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    void SearchMembersModel::setFocus()
    {
        Match_.clear();
    }

    const QStringList& SearchMembersModel::GetPattern() const
    {
        return SearchPatterns_;
    }

    void SearchMembersModel::searchPatternChanged(QString p)
    {
        Match_.clear();

        if (chat_members_model_ == NULL)
            return;

        if (p.isEmpty())
        {
            for (auto item : chat_members_model_->members_)
            {
                Match_.emplace_back(item);
            }
        }
        else
        {
            p = p.toLower();
            auto searchPatterns = Utils::GetPossibleStrings(p);
            for (auto item : chat_members_model_->members_)
            {
                for (auto iter = searchPatterns.begin(); iter != searchPatterns.end(); ++iter)
                {
                    if (item.AimdId_.contains(*iter)
                        || item.NickName_.toLower().contains(*iter) 
                        || item.FirstName_.toLower().contains(*iter)
                        || item.LastName_.toLower().contains(*iter))
                    {
                        Match_.emplace_back(item);
                        break;
                    }
                }
            }
        }
        unsigned size = (unsigned)Match_.size();
        emit dataChanged(index(0), index(size));
    }

    void SearchMembersModel::searchResult(QStringList result)
    {
        assert(!!"Methon is not implemented. Search in members don't use corelib.");
        return;
    }

    void SearchMembersModel::emitChanged(int first, int last)
    {
        emit dataChanged(index(first), index(last));
    }

    void SearchMembersModel::SetChatMembersModel(ChatMembersModel* _chat_members_model)
    {
        chat_members_model_ = _chat_members_model;
    }

    SearchMembersModel* GetSearchMemberModel()
    {
        static std::unique_ptr<SearchMembersModel> model(new SearchMembersModel(0));
        return model.get();
    }
}