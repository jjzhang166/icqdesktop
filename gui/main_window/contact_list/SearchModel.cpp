#include "stdafx.h"
#include "SearchModel.h"
#include "ContactListModel.h"
#include "ContactItem.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    SearchModel::SearchModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , searchRequested_(false)
    {
        connect(Ui::GetDispatcher(), SIGNAL(searchResult(QStringList)), this, SLOT(searchResult(QStringList)), Qt::QueuedConnection);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(contactRemoved(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);
    }

    int SearchModel::rowCount(const QModelIndex &) const
    {
        return (int)match_.size();
    }
    
    QVariant SearchModel::data(const QModelIndex & _ind, int _role) const
    {
        if (!_ind.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || (unsigned)_ind.row() >= match_.size())
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return match_[_ind.row()].get_aimid();

        return QVariant::fromValue(match_[_ind.row()].Get());
    }

    Qt::ItemFlags SearchModel::flags(const QModelIndex &) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    void SearchModel::setFocus()
    {
        match_.clear();
    }

    const QStringList& SearchModel::getPattern() const
    {
        return searchPatterns_;
    }

    void SearchModel::searchPatternChanged(QString _p)
    {
        searchPatterns_ = Utils::GetPossibleStrings(_p);
        if (_p.isEmpty())
        {
            unsigned size = (unsigned)match_.size();
            match_ = getContactListModel()->getSearchedContacts(isClSorting());
            emit dataChanged(index(0), index(size));
            return;
        }

        if (!searchRequested_)
        {
            QTimer::singleShot(200, [this]()
            {
                searchRequested_ = false;
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                core::ifptr<core::iarray> patternsArray(collection->create_array());
                patternsArray->reserve(searchPatterns_.size());
                for (auto iter = searchPatterns_.begin(); iter != searchPatterns_.end(); ++iter)
                {
                    core::coll_helper coll(collection->create_collection(), true);
                    coll.set_value_as_string("pattern", iter->toUtf8().data(), iter->toUtf8().size());
                    core::ifptr<core::ivalue> val(collection->create_value());
                    val->set_as_collection(coll.get());
                    patternsArray->push_back(val.get());
                }
                collection.set_value_as_array("search_patterns", patternsArray.get());
                Ui::GetDispatcher()->post_message_to_core("search", collection.get());
            });
            searchRequested_ = true;
        }
    }

    bool SearchModel::less(const ContactItem& _first, const ContactItem& _second)
    {
        if (_first.Get() && _second.Get())
            return _first.Get()->GetDisplayName().compare(_second.Get()->GetDisplayName(), Qt::CaseInsensitive) < 0;
        else 
            return _first.get_aimid().compare(_second.get_aimid()) < 0; 
    }

    void SearchModel::searchResult(QStringList _result)
    {
        unsigned size = (unsigned)match_.size();
        match_ = getContactListModel()->getSearchedContacts(_result.toStdList());
        std::sort(match_.begin(), match_.end(), [this](const ContactItem& _first, const ContactItem& _second)
        {
           // TODO : use isClSorting here
           return less(_first, _second);
        }
        );
        emit dataChanged(index(0), index(size));
        if (!_result.isEmpty())
            emit results();
    }

    void SearchModel::avatarLoaded(QString _aimId)
    {
        int i = 0;
        for (auto iter : match_)
        {
            if (iter.Get() && iter.Get()->AimId_ == _aimId)
            {
                emit dataChanged(index(i), index(i));
                break;
            }
            ++i;
        }
    }

    void SearchModel::contactRemoved(QString _contact)
    {
        match_.erase(
            std::remove_if(match_.begin(), match_.end(), [_contact](const ContactItem& item)
        {
            return item.Get()->AimId_ == _contact;
        }),
            match_.end()
        );
        emitChanged(0, (int)match_.size());
    }

    void SearchModel::emitChanged(int _first, int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    SearchModel* getSearchModel()
    {    
        static std::unique_ptr<SearchModel> model(new SearchModel(0));
        return model.get();
    }
}