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
        std::unordered_map<QWidget*, int> indexes;
        
    public:
        explicit WidgetsNavigator(QWidget *parent = nullptr);
        void insertWidget(int index, QWidget *widget);
        void removeWidget(QWidget *widget);
        ~WidgetsNavigator();
    };
}