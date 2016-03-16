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
            setCurrentIndex(0);
    }
    
    void WidgetsNavigator::poproot()
    {
        while (history_.size()) history_.pop(); // idk, maybe some intermediate stuff will be done in each pop.
        setCurrentIndex(0);
    }

}