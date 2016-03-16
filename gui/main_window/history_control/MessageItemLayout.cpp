#include "stdafx.h"

#include "../../utils/profiling/auto_stop_watch.h"

#include "MessageItem.h"

#include "MessageItemLayout.h"

namespace Ui
{
    MessageItemLayout::MessageItemLayout(MessageItem *parent)
        : QLayout(parent)
        , IsDirty_(false)
    {
        assert(parent);
    }

    void MessageItemLayout::setGeometry(const QRect &r)
    {
        QLayout::setGeometry(r);

        const auto sizeChanged = (LastSize_ != r.size());
        if (!sizeChanged && !IsDirty_)
        {
            return;
        }

        LastSize_ = r.size();

        IsDirty_ = false;

        auto item = qobject_cast<MessageItem*>(parent());
        item->manualUpdateGeometry(LastSize_.width());
    }

    void MessageItemLayout::addItem(QLayoutItem *item)
    {
        item;
    }

    QLayoutItem* MessageItemLayout::itemAt(int index) const
    {
        index;
        return nullptr;
    }

    QLayoutItem* MessageItemLayout::takeAt(int index)
    {
        index;
        return nullptr;
    }

    int MessageItemLayout::count() const
    {
        return 0;
    }

    QSize MessageItemLayout::sizeHint() const
    {
        auto item = qobject_cast<MessageItem*>(parent());
        return item->sizeHint();
    }

    void MessageItemLayout::invalidate()
    {
        QLayout::invalidate();
    }

    void MessageItemLayout::setDirty()
    {
        IsDirty_ = true;
    }
}