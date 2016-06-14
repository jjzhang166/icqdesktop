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

    Q_SIGNALS:
		void leftButtonClicked();
		void rightButtonClicked();

    private Q_SLOTS:
        void leftButtonClick();
        void rightButtonClick();

    public Q_SLOTS:
        void setButtonActive(bool _active);

    public:
        GeneralDialog(QWidget* _main_widget, QWidget* _parent);
        ~GeneralDialog();
        bool showInPosition(int _x, int _y);

        void addAcceptButton(QString _button_text, int _button_margin_px);
        void addButtonsPair(QString _button_text_left, QString _button_text_right, int _margin_px, int _button_between_px);

        void addLabel(QString _text_label);
        void addHead();
        void addText(QString _message_text, int _upper_margin_px);
        void addError(QString _message_text);
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
        QWidget*            text_host_;
        QWidget*            error_host_;
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
