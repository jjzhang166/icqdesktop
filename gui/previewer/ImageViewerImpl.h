#pragma once

namespace Previewer
{
    class AbstractViewer
        : public QObject
    {
        Q_OBJECT
    public:
        virtual ~AbstractViewer();

        bool canScroll() const;

        QRect rect() const;

        void scale(double _newScaleFactor);
        void move(const QPoint& _offset);

        void paint(QPainter& _painter);

        double getPrefferedScaleFactor() const;
        double getScaleFactor() const;

    protected:
        explicit AbstractViewer(const QSize& _viewportSize, QWidget* _parent);

        void init(const QRect& _imageRect);

    protected slots:
        void repaint();

    private:
        virtual void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) = 0;

        double getPrefferedScaleFactor(const QSize& _imageSize) const;

        void fixBounds(const QSize& _bounds, QRect& _child);

        QRect getViewportRect(double _scaleFactor) const;

    protected:
        bool canScroll_;

        QRect imageRect_;

    private:
        QRect viewportRect_;

        QRect viewport_;
        QRect fragment_;

        QPoint offset_;

        double prefferedScaleFactor_;
        double scaleFactor_;
    };

    class GifViewer
        : public AbstractViewer
    {
        Q_OBJECT
    public:
        static std::unique_ptr<AbstractViewer> create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

    private:
        GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        std::unique_ptr<QMovie> gif_;
    };

    class JpegPngViewer
        : public AbstractViewer
    {
    public:
        static std::unique_ptr<AbstractViewer> create(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent);

    private:
        JpegPngViewer(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QPixmap originalImage_;
    };
}
