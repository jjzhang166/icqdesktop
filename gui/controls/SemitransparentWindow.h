#pragma once

namespace Ui
{
    class qt_gui_settings;

    class SemitransparentWindow : public QWidget
    {
        Q_OBJECT

    public:
        SemitransparentWindow(QWidget* _parent);
        ~SemitransparentWindow();

    protected:
        virtual void paintEvent(QPaintEvent*) override;

    protected:
        virtual void mousePressEvent(QMouseEvent *e) override;
        
    private:
        bool main_;
    };
}
