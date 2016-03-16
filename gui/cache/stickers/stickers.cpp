#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

#include "stickers.h"

UI_STICKERS_NS_BEGIN

std::unique_ptr<cache> g_cache;
cache& get_cache();

sticker::sticker()
    : id_(0)
{

}

sticker::sticker(const int32_t _id)
    : id_(_id)
{
    assert(id_ > 0);
}

void sticker::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");
}

int32_t sticker::get_id() const
{
    return id_;
}

QImage sticker::get_image(const core::sticker_size _size) const
{
    const auto found = images_.find(_size);
    if (found == images_.cend())
    {
        return QImage();
    }

    return std::get<0>(found->second);
}

void sticker::set_image(const core::sticker_size _size, const QImage &_image)
{
    assert(!_image.isNull());

    auto &image_data = images_[_size];

    if (std::get<0>(image_data).isNull())
    {
        image_data = std::make_tuple(_image, false);
    }
}

bool sticker::is_image_requested(const core::sticker_size _size) const
{
    const auto found = images_.find(_size);
    if (found == images_.end())
    {
        return false;
    }

    return std::get<1>(found->second);
}

void sticker::set_image_requested(const core::sticker_size _size, const bool _val)
{
    auto &image_data = images_[_size];

    image_data = std::make_tuple(
        std::get<0>(image_data),
        _val
        );

    assert(!_val || std::get<0>(image_data).isNull());
}

void sticker::clear_cache()
{
    for (auto &pair : images_)
    {
        auto &image = std::get<0>(pair.second);

        pair.second = std::make_tuple(image, false);

        image.loadFromData(0, 0);
    }
}

set::set(int32_t _max_size)
    :	max_size_(_max_size),
    id_(-1)
{

}


QImage set::get_sticker_image(const int32_t _sticker_id, const core::sticker_size _size)
{
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    auto iter = stickers_tree_.find(_sticker_id);
    if (iter == stickers_tree_.end())
    {
        return QImage();
    }

    auto image = iter->second->get_image(_size);

    if (image.isNull() && !iter->second->is_image_requested(_size))
    {
        const auto set_id = get_id();
        assert(set_id > 0);

        const auto sticker_id = iter->second->get_id();
        assert(sticker_id);

        Ui::GetDispatcher()->getSticker(set_id, sticker_id, _size);

        iter->second->set_image_requested(_size, true);
    }

    return image;
}

void set::set_sticker_image(const int32_t _sticker_id, const core::sticker_size _size, QImage _image)
{
    assert(_sticker_id > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    sticker_sptr update_sticker;

    auto iter = stickers_tree_.find(_sticker_id);
    if (iter == stickers_tree_.end())
    {
        update_sticker.reset(new sticker(_sticker_id));
        stickers_tree_[_sticker_id] = update_sticker;
    }
    else
    {
        update_sticker = iter->second;
    }

    update_sticker->set_image(_size, _image);
}

void set::set_id(int32_t _id)
{
    id_= _id;
}

int32_t set::get_id() const
{
    return id_;
}

bool set::empty() const
{
    return stickers_.empty();
}

void set::set_name(const QString& _name)
{
    name_ = _name;
}

QString set::get_name() const
{
    return name_;
}

void set::load_icon(char* _data, int32_t _size)
{
    icon_.loadFromData((const uchar*)_data, _size);
    Utils::check_pixel_ratio(icon_);
}

QPixmap set::get_icon() const
{
    return icon_;
}

sticker_sptr set::get_sticker(int32_t _sticker_id) const
{
    auto iter = stickers_tree_.find(_sticker_id);
    if (iter != stickers_tree_.end())
    {
        return iter->second;
    }

    return nullptr;
}

int32_t set::get_count() const
{
    return (int32_t) stickers_.size();
}

int32_t set::get_sticker_pos(int32_t _sticker_id) const
{
    for (int32_t i = 0; i < (int32_t) stickers_.size(); ++i)
    {
        if (stickers_[i] == _sticker_id)
            return i;
    }

    return -1;
}

void set::unserialize(core::coll_helper _coll)
{
    set_id(_coll.get_value_as_int("id"));
    set_name(_coll.get_value_as_string("name"));

    if (_coll.is_value_exist("icon"))
    {
        core::istream* icon_stream = _coll.get_value_as_stream("icon");
        if (icon_stream)
        {
            int32_t icon_size = icon_stream->size();
            if (icon_size > 0)
            {
                load_icon((char*)icon_stream->read(icon_size), icon_size);
            }
        }
    }

    core::iarray* sticks = _coll.get_value_as_array("stickers");

    stickers_.reserve(sticks->size());

    for (int32_t i = 0; i < sticks->size(); i++)
    {
        core::coll_helper coll_sticker(sticks->get_at(i)->get_as_collection(), false);

        auto inserted_sticker = std::make_shared<sticker>();
        inserted_sticker->unserialize(coll_sticker);

        stickers_tree_[inserted_sticker->get_id()] = inserted_sticker;
        stickers_.push_back(inserted_sticker->get_id());
    }
}

void set::reset_flag_requested(const int32_t _sticker_id, const core::sticker_size _size)
{
    auto iter = stickers_tree_.find(_sticker_id);
    if (iter != stickers_tree_.end())
    {
        iter->second->set_image_requested(_size, false);
    }
}

void set::clear_cache()
{
    for (auto iter = stickers_tree_.begin(); iter != stickers_tree_.end(); ++iter)
    {
        iter->second->clear_cache();
    }
}

const stickers_array& set::get_stickers() const
{
    return stickers_;
}

void unserialize(core::coll_helper _coll)
{
    get_cache().unserialize(_coll);
}

void set_sticker_data(core::coll_helper _coll)
{
    get_cache().set_sticker_data(_coll);
}

const sets_ids_array& get_stickers_sets()
{
    return get_cache().get_sets();
}

void clear_cache()
{
    get_cache().clear_cache();
}

std::shared_ptr<sticker> get_sticker(uint32_t _set_id, uint32_t _sticker_id)
{
    return get_cache().get_sticker(_set_id, _sticker_id);
}

cache::cache()
{

}

void cache::set_sticker_data(core::coll_helper _coll)
{
    const qint32 set_id = _coll.get_value_as_int("set_id");

    set_sptr sticker_set;

    auto iter_set = sets_tree_.find(set_id);
    if (iter_set == sets_tree_.end())
    {
        sticker_set = std::make_shared<set>();
        sticker_set->set_id(set_id);
        sets_tree_[set_id] = sticker_set;
    }
    else
    {
        sticker_set = iter_set->second;
    }

    
    const qint32 sticker_id = _coll.get_value_as_int("sticker_id");

    const auto load_data =
        [&_coll, sticker_set, sticker_id](const char *_id, const core::sticker_size _size)
    {
        if (!_coll->is_value_exist(_id))
        {
            return;
        }

        auto data = _coll.get_value_as_stream(_id);
        const auto data_size = data->size();

        QImage image;
        if (image.loadFromData(data->read(data_size), data_size))
        {
            sticker_set->set_sticker_image(sticker_id, _size, std::move(image));
        }
    };

    load_data("data/small", core::sticker_size::small);
    load_data("data/medium", core::sticker_size::medium);
    load_data("data/large", core::sticker_size::large);

    sticker_set->reset_flag_requested(sticker_id, core::sticker_size::small);
    sticker_set->reset_flag_requested(sticker_id, core::sticker_size::medium);
    sticker_set->reset_flag_requested(sticker_id, core::sticker_size::large);
}

void cache::unserialize(const core::coll_helper &_coll)
{
    core::iarray* sets = _coll.get_value_as_array("sets");
    if (!sets)
        return;

    sets_.clear();
    sets_.reserve(sets->size());

    for (int32_t i = 0; i < sets->size(); i++)
    {
        core::coll_helper coll_set(sets->get_at(i)->get_as_collection(), false);

        auto inserted_set = std::make_shared<set>();

        inserted_set->unserialize(coll_set);

        sets_tree_[inserted_set->get_id()] = inserted_set;

        sets_.push_back(inserted_set->get_id());
    }
}

const sets_ids_array& cache::get_sets() const
{
    return sets_;
}

set_sptr cache::get_set(int32_t _set_id) const
{
    auto iter = sets_tree_.find(_set_id);
    if (iter == sets_tree_.end())
        return nullptr;

    return iter->second;
}

set_sptr cache::insert_set(int32_t _set_id)
{
    auto iter = sets_tree_.find(_set_id);
    if (iter != sets_tree_.end())
    {
        assert(false);
        return iter->second;
    }

    auto inserted_set = std::make_shared<set>();
    inserted_set->set_id(_set_id);
    sets_tree_[_set_id] = inserted_set;

    return inserted_set;
}

std::shared_ptr<sticker> cache::get_sticker(uint32_t _set_id, uint32_t _sticker_id) const
{
    auto iter_set = sets_tree_.find(_set_id);
    if (iter_set == sets_tree_.end())
        return nullptr;

    return iter_set->second->get_sticker(_sticker_id);
}

void cache::clear_cache()
{
    for (auto iter = sets_tree_.begin(); iter != sets_tree_.end(); ++iter)
    {
        iter->second->clear_cache();
    }
}

const int32_t get_set_stickers_count(int32_t _set_id)
{
    auto search_set = g_cache->get_set(_set_id);
    assert(search_set);
    if (!search_set)
        return 0;

    return search_set->get_count();
}

const int32_t get_sticker_pos_in_set(int32_t _set_id, int32_t _sticker_id)
{
    auto search_set = g_cache->get_set(_set_id);
    assert(search_set);
    if (!search_set)
        return -1;

    return search_set->get_sticker_pos(_sticker_id);
}

const stickers_array& get_stickers(int32_t _set_id)
{
    auto search_set = g_cache->get_set(_set_id);
    assert(search_set);
    if (!search_set)
    {
        return g_cache->insert_set(_set_id)->get_stickers();
    }

    return search_set->get_stickers();
}

QImage get_sticker_image(int32_t _set_id, int32_t _sticker_id, const core::sticker_size _size)
{
    auto search_set = g_cache->get_set(_set_id);
    if (!search_set)
    {
        return QImage();
    }

    return search_set->get_sticker_image(_sticker_id, _size);
}

QPixmap get_set_icon(int32_t _set_id)
{
    auto search_set = g_cache->get_set(_set_id);
    assert(search_set);
    if (!search_set)
    {
        return QPixmap();
    }

    return search_set->get_icon();
}

QString get_set_name(int32_t _set_id)
{
    auto search_set = g_cache->get_set(_set_id);
    assert(search_set);
    if (!search_set)
    {
        return QString();
    }

    return search_set->get_name();
}

void reset_cache()
{
    if (g_cache)
        g_cache.reset();
}

cache& get_cache()
{
    if (!g_cache)
        g_cache.reset(new cache());

    return (*g_cache);
}

UI_STICKERS_NS_END