#pragma once

namespace Ui
{
    class WidgetsNavigator: public QStackedWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void push(QWidget *widget);
        void pop();
        void poproot();

    private:
        std::stack< int > history_;
        
    public:
        explicit WidgetsNavigator(QWidget *parent = nullptr);
        ~WidgetsNavigator();
    };
}