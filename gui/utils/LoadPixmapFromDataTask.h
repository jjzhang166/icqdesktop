#pragma once

namespace core
{
    struct istream;
}

namespace Utils
{
    class LoadPixmapFromDataTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedSignal(QPixmap pixmap);

    public:
        LoadPixmapFromDataTask(core::istream *stream);

        virtual ~LoadPixmapFromDataTask();

        void run();

    private:
        core::istream *Stream_;

    };

}

