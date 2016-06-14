#pragma once

class QWidget;
class QString;

namespace Testing
{

    void setAccessibleName(QWidget* target, const QString& name);

    bool isAccessibleRole(int _role);

}
