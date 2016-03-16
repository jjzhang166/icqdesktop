#include "stdafx.h"

#include "../../utils/profiling/auto_stop_watch.h"

#include "ResizePixmapTask.h"

namespace HistoryControl
{

    ResizePixmapTask::ResizePixmapTask(const QPixmap &preview, const QSize &size)
        : Preview_(preview)
        , Size_(size)
    {
        assert(Preview_);
        assert(!Size_.isEmpty());
    }

    void ResizePixmapTask::run()
    {
        Preview_ = Preview_.scaled(
            Size_,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );

        emit resizedSignal(Preview_);
    }

}