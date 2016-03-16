#pragma once

namespace Ui
{
    class BackButton : public QPushButton
    {
    private:
        enum
        {
            UseNormalColor  = 0,
            UseHoverColor,
            UsePressedColor,
        };
        
        QColor normalColor_;
        QColor hoverColor_;
        QColor pressedColor_;
        
        int current_;
        
    private:
        void paintEvent(QPaintEvent* event) override;
        bool event(QEvent* event) override;
        
    public:
        BackButton(QWidget* parent = nullptr);
        ~BackButton();

        inline QColor normalColor() const
        {
            return normalColor_;
        }
        void setNormalColor(const QColor& color)
        {
            normalColor_ = color;
        }

        inline QColor hoverColor() const
        {
            return hoverColor_;
        }
        void setHoverColor(const QColor& color)
        {
            hoverColor_ = color;
        }
        
        inline QColor pressedColor() const
        {
            return pressedColor_;
        }
        void setPressedColor(const QColor& color)
        {
            pressedColor_ = color;
        }
    };
}