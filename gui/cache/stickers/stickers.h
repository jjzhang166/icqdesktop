#pragma once

#include "../../../corelib/collection_helper.h"

#define UI_STICKERS_NS_BEGIN namespace Ui { namespace Stickers {
#define UI_STICKERS_NS_END } }

namespace core
{
    enum class sticker_size;
}


UI_STICKERS_NS_BEGIN

class Sticker
{
    typedef std::tuple<QImage, bool> image_data;

    uint32_t id_;

    std::map<core::sticker_size, image_data> images_;

public:
    Sticker();
    Sticker(const int32_t _id);

    int32_t getId() const;

    QImage getImage(const core::sticker_size _size) const;
    void setImage(const core::sticker_size _size, const QImage& _image);

    bool isImageRequested(const core::sticker_size _size) const;
    void setImageRequested(const core::sticker_size _size, const bool _val);

    void unserialize(core::coll_helper _coll);
    void clearCache();

};

typedef std::shared_ptr<Sticker> stickerSptr;

typedef std::map<int32_t, stickerSptr> stickersMap;

typedef std::vector<int32_t> stickersArray;

class Set
{
    int32_t id_;
    QString name_;
    QPixmap icon_;

    stickersArray stickers_;

    stickersMap stickersTree_;

    int32_t maxSize_;

public:

    typedef std::shared_ptr<Set> sptr;

    Set(int32_t _maxSize = -1);

    void setId(int32_t _id);
    int32_t getId() const;

    void setName(const QString& _name);
    QString getName() const;

    void loadIcon(char* _data, int32_t _size);
    QPixmap getIcon() const;

    int32_t getCount() const;
    int32_t getStickerPos(int32_t _stickerId) const;

    const stickersArray& getStickers() const;

    bool empty() const;

    QImage getStickerImage(const int32_t _stickerId, const core::sticker_size _size);
    void setStickerImage(const int32_t _stickerId, const core::sticker_size _size, const QImage _image);
    void resetFlagRequested(const int32_t _stickerId, const core::sticker_size _size);

    stickerSptr getSticker(int32_t _stickerId) const;
    void unserialize(core::coll_helper _coll);

    void clearCache();
};

typedef std::shared_ptr<Set> setSptr;

typedef std::vector<int32_t> setsIdsArray;

typedef std::map<int32_t, setSptr> setsMap;

class Cache
{
public:

    Cache();

    void unserialize(const core::coll_helper &_coll);

    void setStickerData(core::coll_helper _coll);

    stickerSptr getSticker(uint32_t _setId, uint32_t _stickerId) const;

    const setsIdsArray& getSets() const;

    setSptr getSet(int32_t _setId) const;

    setSptr insertSet(int32_t _setId);

    void clearCache();

private:

    setsMap setsTree_;

    setsIdsArray sets_;

};

void unserialize(core::coll_helper _coll);

void setStickerData(core::coll_helper _coll);

std::shared_ptr<Sticker> getSticker(uint32_t _setId, uint32_t _stickerId);

const setsIdsArray& getStickersSets();

const stickersArray& getStickers(int32_t _setId);

const int32_t getSetStickersCount(int32_t _setId);

const int32_t getStickerPosInSet(int32_t _setId, int32_t _stickerId);

QImage getStickerImage(int32_t _setId, int32_t _stickerId, const core::sticker_size _size);

QPixmap getSetIcon(int32_t _setId);

QString getSetName(int32_t _setId);

void clearCache();
void resetCache();

UI_STICKERS_NS_END