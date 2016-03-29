#pragma once

#include "CustomAbstractListModel.h"

namespace Logic
{
    class AbstractSearchModel : public CustomAbstractListModel
    {
        Q_OBJECT

    public:
        AbstractSearchModel(QObject *parent = 0);
        virtual void setFocus() = 0;
        virtual void emitChanged(int first, int last) = 0;
        virtual void searchPatternChanged(QString p) = 0;
    };
    
    AbstractSearchModel* GetCurrentSearchModel(int _regim);
}