#pragma once

namespace Ui
{
	class qt_gui_settings;

	class SemitransparentWindow : public QWidget
	{
		Q_OBJECT

    Q_SIGNALS:
        void clicked();
        
	public:
		SemitransparentWindow(qt_gui_settings* _qui_settings, QWidget* _parent);
		~SemitransparentWindow();
        void setShow(bool _is_show);

#ifdef __linux__
    protected:
        virtual void paintEvent(QPaintEvent*);

    private:
        bool Is_Show_;
#endif // __linux__
        
    protected:
        virtual void mousePressEvent(QMouseEvent *e) override;
	};
}
