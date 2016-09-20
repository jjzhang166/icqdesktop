#include "stdafx.h"

#include "../core_dispatcher.h"

#include "../utils/gui_coll_helper.h"

#include "ImageCache.h"

namespace
{
    const int fetchSize = 100;
}

Previewer::ImageCache::ImageCache(const QString& _aimId)
    : QObject(nullptr)
    , aimId_(_aimId)
{
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getImagesResult, this, &Previewer::ImageCache::onGetImagesResult, Qt::QueuedConnection);

    loadImages(-1);
}

void Previewer::ImageCache::repair()
{
    images_.clear();

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", aimId_);
    Ui::GetDispatcher()->post_message_to_core("archive/images/repair", collection.get());

    loadImages(-1);
}

void Previewer::ImageCache::onGetImagesResult(Data::ImageListPtr _result)
{
    if (_result->empty())
    {
        emit imagesGetted(true);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);

        Data::Image prev;
        for (int i = _result->size() - 1; i >= 0; --i)
        {
            const auto& current = (*_result)[i];
            if (prev.isNull() || prev != current) // skip duplicates
                images_.prepend(current);
            prev = current;
        }
    }

    emit imagesGetted(false);

    const auto from = _result->begin()->msgId_;
    loadImages(from);
}

void Previewer::ImageCache::loadImages(uint64_t _from)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", aimId_);
    collection.set_value_as_int64("from", _from);
    collection.set_value_as_int64("count", fetchSize);
    Ui::GetDispatcher()->post_message_to_core("archive/images/get", collection.get());
}
