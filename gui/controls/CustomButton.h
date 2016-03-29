#pragma once

namespace Ui
{
#ifdef __APPLE__
    const QString close_button_style =
        "QPushButton { border-image: url(:/resources/main_window/contr_close_100.png); width: 24dip; height: 24dip; background-color: transparent; padding: 0; margin: 0; border: none; } "
        "QPushButton:hover { border-image: url(:/resources/main_window/contr_close_100_hover.png); background-color: #e81123; } "
        "QPushButton:pressed { border-image: url(:/resources/main_window/contr_close_100_active.png); background-color: #d00516; }";
#else
	const QString close_button_style =
		"QPushButton { background-image: url(:/resources/main_window/contr_close_100.png); background-color: transparent; background-repeat: no-repeat; background-position: center; background-color: transparent; padding-top: 2dip; padding-bottom: 2dip; width: 24dip; height: 24dip; padding-left: 11dip; padding-right: 12dip; border: none; } QPushButton:hover { background-image: url(:/resources/main_window/contr_close_100_hover.png); background-color: #e81123; } QPushButton:pressed { background-image: url(:/resources/main_window/contr_close_100_active.png); background-color: #d00516; } ";
#endif
    
	const QString main_button_style =
		"QPushButton { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #ffffff; font-size: 16dip; background-color: #579e1c; border-style: solid; border-width: 2dip; border-color: #427a13; margin: 0; padding-left: 20dip; padding-right: 20dip; min-width: 100dip; max-height: 28dip; min-height: 28dip; } QPushButton:hover { background-color: #57a813; } QPushButton:pressed { background-color: #50901b; } ";
	
	const QString grey_button_style =
		"QPushButton { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #282828; font-size: 16dip; background-color: #c5c5c5; border-style: none; margin: 0; padding-left: 20dip; padding-right: 20dip; min-width: 100dip; max-height: 32dip; min-height: 32dip; } QPushButton:hover { background-color: #d2d2d2; } QPushButton:pressed { background-color: #bbbbbb; } ";

	const QString disable_button_style =
		"QPushButton { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #a9a9a9; font-size: 16dip; background-color: #e4e4e4; border-style: none; margin: 0; padding-left: 20dip; padding-right: 20dip; min-width: 100dip; max-height: 32dip; min-height: 32dip; } ";

	const QString main_button_noborder_style =
		"QPushButton { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #ffffff; font-size: 16dip; background-color: #579e1c; border-style: none; margin: 0; padding-left: 20dip; padding-right: 20dip; /*min-width: 100dip;*/ max-height: 32dip; min-height: 32dip; } QPushButton:hover { background-color: #57a813; } QPushButton:pressed { background-color: #50901b; } ";


    
class CustomButton : public QPushButton
{
    virtual void paintEvent(QPaintEvent*) override;

    QPixmap		pixmapToDraw_;
    QPixmap		pixmapDefault_;
    QPixmap		pixmapHover_;
    QPixmap		pixmapActive_;
	QColor		fillColor_;

	int			x_;
	int			y_;
	int			xForActive_;
	int			yForActive_;
	bool		activeState_;
    int         align_;

    void leaveEvent(QEvent * _e) override;
    void enterEvent(QEvent * _e) override;
    void init();
    
public:
    
    CustomButton(QWidget* _parent, const QString& _imageName);
    CustomButton(QWidget* _parent, const QPixmap& _pixmap);

    void setAlign(int flags);
    void setOffsets(int _x, int _y);
	void setOffsetsForActive(int _x, int _y);
    void setImage(const QString& _imageName);
    void setHoverImage(const QString& _imageName);
    void setActiveImage(const QString& _imageName);
    void setActive(bool _isActive);
	void setFillColor(QColor);
    QSize sizeHint() const override;
};
    
}

