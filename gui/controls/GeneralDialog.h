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
        void on_resize_child(int _delta_w, int _delta_h);

    public Q_SLOTS:
        void setButtonActive(bool _active);
        void updateParams(int _width, int _height, int _x, int _y, bool _is_semi_window);

	public:
		GeneralDialog(bool _isShowLabel, bool _isShowButton, QString _text_label, QString _button_label, QWidget* _main_widget, QWidget* _parent, int _button_margin_dip);
		~GeneralDialog();
		bool showWithFixedSizes(int _width, int _height, int _x, int _y, bool _is_semi_window);

        static bool GetConfirmationWithTwoButtons(const QString& _button_left, const QString& _button_right,
            const QString& _text, const QString& _label, QWidget* _parent, QWidget* _parent_mac);

    private:
        void updateParamsRoutine(int _width, int _height, int _x, int _y, bool _is_semi_window);
        
	private:
		qt_gui_settings*    qt_setting_;
		double              koeff_height_;
		double              koeff_width_;
		QWidget*            main_widget_;
		int                 addWidth_;
        bool                isShowLabel_;
        bool                isShowButton_;
        QPushButton*        next_button_;
        int                 button_margin_;
        QString             button_text_;
        SemitransparentWindow* semi_window_;
        std::unique_ptr<Utils::SignalsDisconnector> disconnector_;

        int width_;
        int height_;
        int x_;
        int y_;
        bool is_semi_window_;
        
    protected:
        virtual void showEvent(QShowEvent *e) override;
        virtual void hideEvent(QHideEvent *e) override;
        virtual void mousePressEvent(QMouseEvent *e) override;
        virtual void keyPressEvent(QKeyEvent *e) override;

	};
}
