#pragma once

namespace Ui
{
    class FlatMenuStyle: public QProxyStyle
    {
        Q_OBJECT
        
    public:
        int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;
    };

    class FlatMenu : public QMenu
    {
    private:
        Qt::Alignment expandDirection_;

    private:
        virtual void showEvent(QShowEvent* event) override;
        virtual void paintEvent(QPaintEvent* event) override;

    public:
        FlatMenu(QWidget* parent = nullptr);
        ~FlatMenu();

        void setExpandDirection(Qt::Alignment direction);

        inline Qt::Alignment expandDirection() const
        {
            return expandDirection_;
        }
    };
}