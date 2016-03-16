#pragma once

#include "ContactItem.h"
#include "ContactList.h"
#include "ChatMembersModel.h"
#include "../../types/contact.h"

namespace Logic
{
    class AbstractSearchModel : public QAbstractListModel
    {
        Q_OBJECT

    public:
        AbstractSearchModel(QObject *parent);
        virtual void setFocus() = 0;
        virtual void emitChanged(int first, int last) = 0;
        virtual void searchPatternChanged(QString p) = 0;
    };
    
    AbstractSearchModel* GetCurrentSearchModel(int _regim);
}