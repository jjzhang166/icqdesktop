#include "stdafx.h"

#include "../corelib/collection_helper.h"

#include "utils/profiling/auto_stop_watch.h"

#include "LoadPixmapFromDataTask.h"

namespace Ui
{
    LoadPixmapFromDataTask::LoadPixmapFromDataTask(const qint64 seq, const QString &uri, core::istream *stream, const QString& local)
        : Stream_(stream)
        , Seq_(seq)
        , Uri_(uri)
        , Local_(local)
    {
        assert(Stream_);
        assert(Seq_ > 0);
        assert(!Uri_.isEmpty());

        Stream_->addref();
    }

    LoadPixmapFromDataTask::~LoadPixmapFromDataTask()
    {
        Stream_->release();
    }

    void LoadPixmapFromDataTask::run()
    {
        const auto size = Stream_->size();
        assert(size > 0);

        QPixmap preview;
        if (!preview.loadFromData((uchar*)Stream_->read(size), size))
        {
            preview = QPixmap();
        }

        emit loadedSignal(Seq_, Uri_, preview, Local_);
    }
}