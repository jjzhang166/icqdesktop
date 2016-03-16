#pragma once

namespace Ui {

	class BackgroundWidget : public QStackedWidget
	{
		QPixmap		pixmapToDraw_;
        QPixmap     cachedPixmap_;
        struct cachedPixmapParams {
            int x_;
            int y_;
            int w_;
            int h_;
            bool cacheAndCheckIfChanged(int x, int y, int w, int h);
            void invalidate() { x_ = y_ = w_ = h_ = 0; };
        } cachedPixmapParams_;
        
        bool tiling_;
        void resizeEvent(QResizeEvent *_event) override;
	protected:
		virtual void paintEvent(QPaintEvent*) override;
        QSize currentSize_;
        void drawPixmap(int x, int y, int w, int h, const QPixmap& _pm);

	public:

		BackgroundWidget(QWidget* _parent, const QString& _imageName);

        void setImage(const QPixmap& _pixmap, const bool _tiling);
	};

}