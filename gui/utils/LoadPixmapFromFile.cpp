#include "stdafx.h"

#include "utils.h"

#include "LoadPixmapFromFileTask.h"

namespace Utils
{
    LoadPixmapFromFileTask::LoadPixmapFromFileTask(const QString& path)
        : Path_(path)
    {
        assert(!Path_.isEmpty());
        assert(QFile::exists(Path_));
    }

    LoadPixmapFromFileTask::~LoadPixmapFromFileTask()
    {
    }

    void LoadPixmapFromFileTask::run()
    {
        QPixmap preview;
        Utils::loadPixmap(Path_, Out preview);

        emit loadedSignal(preview);
    }
}