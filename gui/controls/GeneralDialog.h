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
        virtual void reject() override;

    public:
        GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents = false);
        ~GeneralDialog();
        bool showInPosition(int _x, int _y);

        void addAcceptButton(QString _buttonText, int _buttonMarginPx, const bool _isEnabled);
        void addButtonsPair(QString _buttonTextLeft, QString _buttonTextRight, int _marginPx, int _buttonBetweenPx);

        QPushButton* takeAcceptButton();

        void addBottomLabel(const QString& _text, const int32_t _marginPx);
        void addLabel(const QString& _text);
        void addHead();
        void addText(QString _messageText, int _upperMarginPx);
        void addError(QString _messageText);
        void setKeepCenter(bool _isKeepCenter);

    protected:
       virtual void resizeEvent(QResizeEvent *) override;

    private:
        QLayout *initBottomLayout(const int32_t _buttonMarginPx);

        void moveToPosition(int _x, int _y);

    private:
        double                  koeffHeight_;
        double                  koeffWidth_;
        QWidget*                mainWidget_;
        int                     addWidth_;
        QPushButton*            nextButton_;
        SemitransparentWindow*  semiWindow_;
        QWidget*                bottomWidget_;
        QWidget*                labelHost_;
        QWidget*                textHost_;
        QWidget*                errorHost_;
        QWidget*                headHost_;
        bool                    keepCenter_;
        int                     x_;
        int                     y_;
        QLabel*                 bottomLabel_;
        
        bool                    ignoreKeyPressEvents_;

        std::unique_ptr<Utils::SignalsDisconnector> disconnector_;

    protected:
        virtual void mousePressEvent(QMouseEvent* _e) override;
        virtual void keyPressEvent(QKeyEvent* _e) override;

    };
}
