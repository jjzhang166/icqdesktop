#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class MessageItemBase : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        MessageItemBase(QWidget* _parent);

        virtual ~MessageItemBase() = 0;

        virtual bool isOutgoing() const = 0;

    };

}

