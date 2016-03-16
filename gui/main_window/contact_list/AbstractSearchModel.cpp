#include "stdafx.h"
#include "SearchMembersModel.h"
#include "SearchModel.h"
#include "ChatMembersModel.h"
#include "ContactItem.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

namespace Logic
{
    AbstractSearchModel::AbstractSearchModel(QObject *parent)
        : QAbstractListModel(parent)
    {     
    }
    
    AbstractSearchModel* GetCurrentSearchModel(int _regim)
    {
        if (!Logic::is_delete_members(_regim))
            return GetSearchModel();
        else
            return GetSearchMemberModel();
    }
}