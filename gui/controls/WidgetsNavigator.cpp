#include "stdafx.h"
#include "WidgetsNavigator.h"

namespace Ui
{
    WidgetsNavigator::WidgetsNavigator(QWidget *parent/* = nullptr*/):
        QStackedWidget(parent)
    {
    }
    WidgetsNavigator::~WidgetsNavigator()
    {
    }
    
    void WidgetsNavigator::insertWidget(int index, QWidget *widget)
    {
        assert(indexes.count(widget) == 0);
        indexes.insert(std::make_pair(widget, index));
        QStackedWidget::insertWidget(index, widget);
    }

    void WidgetsNavigator::push(QWidget *widget)
    {
        if (currentWidget() != widget)
        {
            setCurrentWidget(widget);
            history_.push(currentIndex());
        }
    }
    
    void WidgetsNavigator::pop()
    {
        if (!history_.empty())
            history_.pop();
        if (!history_.empty())
        {
            if (history_.top() >= 0 && history_.top() < count())
                setCurrentIndex(history_.top());
            else
                return pop();
        }
        else
            setCurrentIndex(indexes.size() == 0 ? 0 : indexes.begin()->second);
    }
    
    void WidgetsNavigator::poproot()
    {
        while (history_.size()) history_.pop(); // idk, maybe some intermediate stuff will be done in each pop.
        setCurrentIndex(indexes.size() == 0 ? 0 : indexes.begin()->second);
    }

    void WidgetsNavigator::removeWidget(QWidget *widget)
    {
        assert(indexes.count(widget) != 0);
        if (indexes.count(widget) != 0)
            indexes.erase(widget);
        QStackedWidget::removeWidget(widget);
    }
}