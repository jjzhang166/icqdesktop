#pragma once

namespace Ui
{

class CustomButton : public QPushButton
{
    virtual void paintEvent(QPaintEvent*) override;
    void leaveEvent(QEvent * _e) override;
    void enterEvent(QEvent * _e) override;
    void mousePressEvent(QMouseEvent * _e) override;
    void mouseReleaseEvent(QMouseEvent * _e) override;
    void init();
    
public:
    CustomButton(QWidget* _parent, const QString& _imageName);
    CustomButton(QWidget* _parent, const QPixmap& _pixmap);

    void setAlign(int _flags);
    void setOffsets(int _x, int _y);
	void setOffsetsForActive(int _x, int _y);
    
    void setImage(const QString& _imageName);
    void setHoverImage(const QString& _imageName);
    void setActiveImage(const QString& _imageName);
    void setDisabledImage(const QString& _imageName);
    void setPressedImage(const QString& _imageName);

    void setImage(const QPixmap& _pixmap);
    void setHoverImage(const QPixmap& _pixmap);
    void setActiveImage(const QPixmap& _pixmap);
    void setDisabledImage(const QPixmap& _pixmap);
    void setPressedImage(const QPixmap& _pixmap);

    void setActive(bool _isActive);
	void setFillColor(QColor);
    QSize sizeHint() const override;
    
    void setMenu(QMenu* menu);

private:
    QPixmap		pixmapToDraw_;
    QPixmap		pixmapDefault_;
    QPixmap		pixmapHover_;
    QPixmap		pixmapActive_;
    QPixmap     pixmapDisabled_;
    QPixmap     pixmapPressed_;
    QColor		fillColor_;
    
    int			x_;
    int			y_;
    int			xForActive_;
    int			yForActive_;
    bool		activeState_;
    int         align_;
};
    
}

