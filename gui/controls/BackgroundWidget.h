#pragma once

namespace Ui
{

	class BackgroundWidget : public QStackedWidget
	{
    private:
		QPixmap		pixmapToDraw_;
        quint64     pixmapToDrawCacheKey_;

        QPixmap     cachedPixmap_;
        quint64     cachedPixmapCacheKey_;

        struct cachedPixmapParams
        {
            int x_;
            int y_;
            int w_;
            int h_;
            bool cacheAndCheckIfChanged(int _x, int _y, int _w, int _h);
            void invalidate() { x_ = y_ = w_ = h_ = 0; };
        }
        cachedPixmapParams_;
        
        bool tiling_;
        void resizeEvent(QResizeEvent *_event) override;
        
	protected:
		virtual void paintEvent(QPaintEvent*) override;
        QSize currentSize_;
        void drawPixmap(int _x, int _y, int _w, int _h);

	public:
		BackgroundWidget(QWidget* _parent, const QString& _imageName);

        void setImage(const QPixmap& _pixmap, const bool _tiling);
	};

}