#pragma once

namespace Ui {

	class PictureWidget : public QWidget
	{
		QPixmap		pixmapToDraw_;

		int			x_;
		int			y_;
		int         align_;

	protected:
		virtual void paintEvent(QPaintEvent*) override;

	public:

		PictureWidget(QWidget* _parent, const QString& _imageName);

		void setAlign(int flags);
		void setOffsets(int _x, int _y);
		void setImage(const QString& _imageName);
	};

}