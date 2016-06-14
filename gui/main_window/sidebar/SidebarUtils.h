#pragma once

namespace Ui
{
    class CustomButton;

    class LineWidget : public QWidget
    {
        Q_OBJECT
    public:
        LineWidget(QWidget* parent, int leftMargin, int topMargin, int rightMargin, int bottomMargin);
        void setLineWidth(int width);
    private:
        QWidget* line_;
    };


    class ActionButton : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        ActionButton(QWidget* parent, const QString& image, const QString& text, int height, int leftMargin, int textOffset);

        void setImage(const QString& image);
        void setText(const QString& text);

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void enterEvent(QEvent*);
        virtual void leaveEvent(QEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);

    private:
        bool Hovered_;
        CustomButton* Button_;
    };


    class ClickedWidget : public QWidget
    {
        Q_OBJECT
Q_SIGNALS:
        void clicked();

    public:
        ClickedWidget(QWidget* parent);

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void enterEvent(QEvent*);
        virtual void leaveEvent(QEvent*);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        bool Hovered_;
    };

    QHBoxLayout* emptyHLayout(QWidget* parent = 0);
    QVBoxLayout* emptyVLayout(QWidget* parent = 0);
}
