#include "stdafx.h"

namespace Testing
{
    void setAccessibleName(QWidget* target, const QString& name)
    {
        assert(!!target);
        if (target != NULL)
            target->setAccessibleName(name);
    }

    bool isAccessibleRole(int _role)
    {
        return _role == Qt::AccessibleDescriptionRole || _role == Qt::AccessibleTextRole;
    }
}