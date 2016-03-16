#ifndef __COMMON_DEFS_H_
#define __COMMON_DEFS_H_

class QWidget;
class QString;

namespace Testing
{

    void setAccessibleName(QWidget* target, const QString& name);

    bool isAccessibleRole(int _role);

}

#endif // __COMMON_DEFS_H_