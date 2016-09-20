#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

#include "stickers.h"

UI_STICKERS_NS_BEGIN

std::unique_ptr<Cache> g_cache;
Cache& getCache();

Sticker::Sticker()
    : id_(0)
{

}

Sticker::Sticker(const int32_t _id)
    : id_(_id)
{
    assert(id_ > 0);
}

void Sticker::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");
}

int32_t Sticker::getId() const
{
    return id_;
}

QImage Sticker::getImage(const core::sticker_size _size) const
{
    const auto found = images_.find(_size);
    if (found == images_.cend())
    {
        return QImage();
    }

    return std::get<0>(found->second);
}

void Sticker::setImage(const core::sticker_size _size, const QImage &_image)
{
    assert(!_image.isNull());

    auto &imageData = images_[_size];

    if (std::get<0>(imageData).isNull())
    {
        imageData = std::make_tuple(_image, false);
    }
}

bool Sticker::isImageRequested(const core::sticker_size _size) const
{
    const auto found = images_.find(_size);
    if (found == images_.end())
    {
        return false;
    }

    return std::get<1>(found->second);
}

void Sticker::setImageRequested(const core::sticker_size _size, const bool _val)
{
    auto &imageData = images_[_size];

    imageData = std::make_tuple(
        std::get<0>(imageData),
        _val
        );

    assert(!_val || std::get<0>(imageData).isNull());
}

void Sticker::clearCache()
{
    for (auto &pair : images_)
    {
        auto &image = std::get<0>(pair.second);

        pair.second = std::make_tuple(image, false);

        image.loadFromData(0, 0);
    }
}

Set::Set(int32_t _maxSize)
    :	maxSize_(_maxSize),
    id_(-1)
{

}


QImage Set::getStickerImage(const int32_t _stickerId, const core::sticker_size _size)
{
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        return QImage();
    }

    auto image = iter->second->getImage(_size);

    if (image.isNull() && !iter->second->isImageRequested(_size))
    {
        const auto setId = getId();
        assert(setId > 0);

        const auto stickerId = iter->second->getId();
        assert(stickerId);

        Ui::GetDispatcher()->getSticker(setId, stickerId, _size);

        iter->second->setImageRequested(_size, true);
    }

    return image;
}

void Set::setStickerImage(const int32_t _stickerId, const core::sticker_size _size, QImage _image)
{
    assert(_stickerId > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    stickerSptr updateSticker;

    auto iter = stickersTree_.find(_stickerId);
    if (iter == stickersTree_.end())
    {
        updateSticker.reset(new Sticker(_stickerId));
        stickersTree_[_stickerId] = updateSticker;
    }
    else
    {
        updateSticker = iter->second;
    }

    updateSticker->setImage(_size, _image);
}

void Set::setId(int32_t _id)
{
    id_= _id;
}

int32_t Set::getId() const
{
    return id_;
}

bool Set::empty() const
{
    return stickers_.empty();
}

void Set::setName(const QString& _name)
{
    name_ = _name;
}

QString Set::getName() const
{
    return name_;
}

void Set::loadIcon(char* _data, int32_t _size)
{
    icon_.loadFromData((const uchar*)_data, _size);
    Utils::check_pixel_ratio(icon_);
}

QPixmap Set::getIcon() const
{
    return icon_;
}

stickerSptr Set::getSticker(int32_t _stickerId) const
{
    auto iter = stickersTree_.find(_stickerId);
    if (iter != stickersTree_.end())
    {
        return iter->second;
    }

    return nullptr;
}

int32_t Set::getCount() const
{
    return (int32_t) stickers_.size();
}

int32_t Set::getStickerPos(int32_t _stickerId) const
{
    for (int32_t i = 0; i < (int32_t) stickers_.size(); ++i)
    {
        if (stickers_[i] == _stickerId)
            return i;
    }

    return -1;
}

void Set::unserialize(core::coll_helper _coll)
{
    setId(_coll.get_value_as_int("id"));
    setName(_coll.get_value_as_string("name"));

    if (_coll.is_value_exist("icon"))
    {
        core::istream* iconStream = _coll.get_value_as_stream("icon");
        if (iconStream)
        {
            int32_t iconSize = iconStream->size();
            if (iconSize > 0)
            {
                loadIcon((char*)iconStream->read(iconSize), iconSize);
            }
        }
    }

    core::iarray* sticks = _coll.get_value_as_array("stickers");

    stickers_.reserve(sticks->size());

    for (int32_t i = 0; i < sticks->size(); i++)
    {
        core::coll_helper coll_sticker(sticks->get_at(i)->get_as_collection(), false);

        auto insertedSticker = std::make_shared<Sticker>();
        insertedSticker->unserialize(coll_sticker);

        stickersTree_[insertedSticker->getId()] = insertedSticker;
        stickers_.push_back(insertedSticker->getId());
    }
}

void Set::resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size)
{
    auto iter = stickersTree_.find(_stickerId);
    if (iter != stickersTree_.end())
    {
        iter->second->setImageRequested(_size, false);
    }
}

void Set::clearCache()
{
    for (auto iter = stickersTree_.begin(); iter != stickersTree_.end(); ++iter)
    {
        iter->second->clearCache();
    }
}

const stickersArray& Set::getStickers() const
{
    return stickers_;
}

void unserialize(core::coll_helper _coll)
{
    getCache().unserialize(_coll);
}

void setStickerData(core::coll_helper _coll)
{
    getCache().setStickerData(_coll);
}

const setsIdsArray& getStickersSets()
{
    return getCache().getSets();
}

void clearCache()
{
    getCache().clearCache();
}

std::shared_ptr<Sticker> getSticker(uint32_t _setId, uint32_t _stickerId)
{
    return getCache().getSticker(_setId, _stickerId);
}

Cache::Cache()
{

}

void Cache::setStickerData(core::coll_helper _coll)
{
    const qint32 setId = _coll.get_value_as_int("set_id");

    setSptr stickerSet;

    auto iterSet = setsTree_.find(setId);
    if (iterSet == setsTree_.end())
    {
        stickerSet = std::make_shared<Set>();
        stickerSet->setId(setId);
        setsTree_[setId] = stickerSet;
    }
    else
    {
        stickerSet = iterSet->second;
    }

    
    const qint32 stickerId = _coll.get_value_as_int("sticker_id");

    const auto loadData =
        [&_coll, stickerSet, stickerId](const char *_id, const core::sticker_size _size)
    {
        if (!_coll->is_value_exist(_id))
        {
            return;
        }

        auto data = _coll.get_value_as_stream(_id);
        const auto dataSize = data->size();

        QImage image;
        if (image.loadFromData(data->read(dataSize), dataSize))
        {
            stickerSet->setStickerImage(stickerId, _size, std::move(image));
        }
    };

    loadData("data/small", core::sticker_size::small);
    loadData("data/medium", core::sticker_size::medium);
    loadData("data/large", core::sticker_size::large);

    stickerSet->resetFlagRequested(stickerId, core::sticker_size::small);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::medium);
    stickerSet->resetFlagRequested(stickerId, core::sticker_size::large);
}

void Cache::unserialize(const core::coll_helper &_coll)
{
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    sets_.clear();
    sets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); i++)
    {
        core::coll_helper collSet(sets->get_at(i)->get_as_collection(), false);

        auto insertedSet = std::make_shared<Set>();

        insertedSet->unserialize(collSet);

        setsTree_[insertedSet->getId()] = insertedSet;

        sets_.push_back(insertedSet->getId());
    }
}

const setsIdsArray& Cache::getSets() const
{
    return sets_;
}

setSptr Cache::getSet(int32_t _setId) const
{
    auto iter = setsTree_.find(_setId);
    if (iter == setsTree_.end())
        return nullptr;

    return iter->second;
}

setSptr Cache::insertSet(int32_t _setId)
{
    auto iter = setsTree_.find(_setId);
    if (iter != setsTree_.end())
    {
        assert(false);
        return iter->second;
    }

    auto insertedSet = std::make_shared<Set>();
    insertedSet->setId(_setId);
    setsTree_[_setId] = insertedSet;

    return insertedSet;
}

std::shared_ptr<Sticker> Cache::getSticker(uint32_t _setId, uint32_t _stickerId) const
{
    auto iterSet = setsTree_.find(_setId);
    if (iterSet == setsTree_.end())
        return nullptr;

    return iterSet->second->getSticker(_stickerId);
}

void Cache::clearCache()
{
    for (auto iter = setsTree_.begin(); iter != setsTree_.end(); ++iter)
    {
        iter->second->clearCache();
    }
}

const int32_t getSetStickersCount(int32_t _setId)
{
    auto searchSet = g_cache->getSet(_setId);
    assert(searchSet);
    if (!searchSet)
        return 0;

    return searchSet->getCount();
}

const int32_t getStickerPosInSet(int32_t _setId, int32_t _stickerId)
{
    auto searchSet = g_cache->getSet(_setId);
    assert(searchSet);
    if (!searchSet)
        return -1;

    return searchSet->getStickerPos(_stickerId);
}

const stickersArray& getStickers(int32_t _setId)
{
    auto searchSet = g_cache->getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return g_cache->insertSet(_setId)->getStickers();
    }

    return searchSet->getStickers();
}

QImage getStickerImage(int32_t _setId, int32_t _stickerId, const core::sticker_size _size)
{
    auto searchSet = g_cache->getSet(_setId);
    if (!searchSet)
    {
        return QImage();
    }

    return searchSet->getStickerImage(_stickerId, _size);
}

QPixmap getSetIcon(int32_t _setId)
{
    auto searchSet = g_cache->getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return QPixmap();
    }

    return searchSet->getIcon();
}

QString getSetName(int32_t _setId)
{
    auto searchSet = g_cache->getSet(_setId);
    assert(searchSet);
    if (!searchSet)
    {
        return QString();
    }

    return searchSet->getName();
}

void resetCache()
{
    if (g_cache)
        g_cache.reset();
}

Cache& getCache()
{
    if (!g_cache)
        g_cache.reset(new Cache());

    return (*g_cache);
}

UI_STICKERS_NS_END