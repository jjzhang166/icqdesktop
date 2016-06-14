#pragma once

namespace Utils
{
    class LoadPixmapFromFileTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedSignal(QPixmap pixmap);

    public:
        explicit LoadPixmapFromFileTask(const QString& path);

        virtual ~LoadPixmapFromFileTask();

        void run();

    private:
        const QString Path_;

    };
}