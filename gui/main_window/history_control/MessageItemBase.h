#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class MessageItemBase : public HistoryControlPageItem
    {
        Q_OBJECT

    public:

        MessageItemBase(QWidget* _parent);
        virtual ~MessageItemBase();

        virtual bool isOutgoing() const = 0;
    };

}

