#pragma once

namespace Previewer
{
    class AbstractViewer;

    class ImageViewerWidget final
        : public QLabel
    {
        Q_OBJECT
    public:
        explicit ImageViewerWidget(QWidget* _parent);
        ~ImageViewerWidget();

        void showImage(const QPixmap& _preview, const QString& _fileName);

        void reset();

    signals:
        void imageClicked();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;

    private:
        std::unique_ptr<AbstractViewer> viewer_;
    };
}
