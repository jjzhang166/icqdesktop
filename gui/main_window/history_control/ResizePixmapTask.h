#pragma once

namespace HistoryControl
{

    class ResizePixmapTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void resizedSignal(QPixmap);

    public:
        ResizePixmapTask(const QPixmap &preview, const QSize &size);

        void run();

    private:
        QPixmap Preview_;

        QSize Size_;

    };

}