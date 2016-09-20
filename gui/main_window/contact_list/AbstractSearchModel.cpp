#include "stdafx.h"
#include "SearchMembersModel.h"
#include "SearchModel.h"

namespace Logic
{
    AbstractSearchModel::AbstractSearchModel(QObject* _parent/* = 0*/)
        : CustomAbstractListModel(_parent)
        , isClSorting_(true)
    {
    }
    
    AbstractSearchModel* getCurrentSearchModel(int _regim)
    {
        if (!Logic::is_delete_members_regim(_regim))
            return getSearchModel();
        else
            return getSearchMemberModel();
    }

    void AbstractSearchModel::setSort(bool _isClSorting)
    {
        isClSorting_ = _isClSorting;
    }

    bool AbstractSearchModel::isClSorting() const
    {
        return isClSorting_;
    }
}
