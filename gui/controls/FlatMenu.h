#pragma once

namespace Ui
{
    class FlatMenuStyle: public QProxyStyle
    {
        Q_OBJECT
        
    public:
        int pixelMetric(PixelMetric _metric, const QStyleOption* _option = 0, const QWidget* _widget = 0) const;
    };

    class FlatMenu : public QMenu
    {
    private:
        Qt::Alignment expandDirection_;
        bool iconSticked_;

        static int shown_;

    private:
        virtual void showEvent(QShowEvent* _event) override;
        virtual void hideEvent(QHideEvent* _event) override;
        virtual void paintEvent(QPaintEvent* _event) override;

    public:
        FlatMenu(QWidget* _parent = nullptr);
        ~FlatMenu();

        void setExpandDirection(Qt::Alignment _direction);
        void stickToIcon();

        static int shown() { return FlatMenu::shown_; }
        
        inline Qt::Alignment expandDirection() const
        {
            return expandDirection_;
        }
    };
}