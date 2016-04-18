#pragma once

namespace Utils
{
    class SignalsDisconnector;
}

namespace Ui
{
    class qt_gui_settings;
    class SemitransparentWindow;

    class GeneralDialog : public QDialog
    {
        Q_OBJECT

    private Q_SLOTS:

    public Q_SLOTS:
        void setButtonActive(bool _active);

    public:
        GeneralDialog(QWidget* _main_widget, QWidget* _parent);
        ~GeneralDialog();
        bool showInPosition(int _x, int _y);

        void addAcceptButton(QString _button_text, int _button_margin_dip);
        void addLabel(QString _text_label);
        void addHead();
        void setKeepCenter(bool _is_keep_center);

    protected:
       virtual void resizeEvent(QResizeEvent *);

    private:
        void moveToPosition(int _x, int _y);

    private:
        double              koeff_height_;
        double              koeff_width_;
        QWidget*            main_widget_;
        int                 addWidth_;
        QPushButton*        next_button_;
        SemitransparentWindow* semi_window_;
        QWidget*            bottom_widget_;
        QWidget*            label_host_;
        QWidget*            head_host_;
        bool                keep_center_;
        int x_;
        int y_;

        std::unique_ptr<Utils::SignalsDisconnector> disconnector_;

    protected:
        virtual void mousePressEvent(QMouseEvent *e) override;
        virtual void keyPressEvent(QKeyEvent *e) override;

    };
}
