#include "stdafx.h"
#include "themes.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../theme_settings.h"
#include "../../utils/log/log.h"

UI_THEMES_NS_BEGIN
std::unique_ptr<cache> t_cache;

theme::theme(int _id, QString _tint_color_string, QByteArray& _imageData, QByteArray& _thumbData, const bool _tile) : id_(_id), position_(0), is_image_loaded_(false) {
    
    initDefault();
    
    tile_ = _tile;
    
    tint_color_ = colorFromString(_tint_color_string.toLatin1().data());
    
    QPixmap pi;
    pi.loadFromData(_imageData);
    image_ = pi;
    if (!image_.isNull())
    {
        is_image_loaded_ = true;
    }
    
    QPixmap pt;
    pt.loadFromData(_thumbData);
    thumb_ = pt;
    Utils::check_pixel_ratio(thumb_);
}

QColor theme::colorFromString(const char* _colorString)
{
    QColor color;
    color.setNamedColor("#" + QString(_colorString));
    return color;
}

theme::theme() : image_(QPixmap()), thumb_(QPixmap()), tile_(false), id_(0), position_(0), is_image_loaded_(true)
{
    initDefault();
}

void theme::initDefault()
{
    tile_ = true;
    tint_color_ = QColor(0xff, 0xff, 0xff, 0x00);
    
    contact_list_item_.bg_color_ = QColor(0xff, 0xff, 0xff, 0xff);
    contact_list_item_.name_color_ = QColor(0x28, 0x28, 0x28, 0xff);
    contact_list_item_.message_color_ = QColor(0x69, 0x69, 0x69, 0xff);
    contact_list_item_.sender_color_ = QColor(0x69, 0x69, 0x69, 0xff);
    contact_list_item_.time_color_ = QColor(0x69, 0x69, 0x69, 0xff);

    incoming_bubble_.bg1_color_ = QColor(0xff, 0xff, 0xff, 0xff);
    incoming_bubble_.bg2_color_ = QColor(0xff, 0xff, 0xff, 0xb8);
    incoming_bubble_.text_color_ = QColor(0x28, 0x28, 0x28, 0xff);
    incoming_bubble_.time_color_ = QColor(0x97, 0x97, 0x97, 0xff);
    incoming_bubble_.link_color_ = QColor(0x57, 0x9e, 0x1c, 0xff);
    incoming_bubble_.info_color_ = QColor(0x28, 0x28, 0x28, 0xff);
    incoming_bubble_.info_link_color_ = QColor(0x57, 0x9e, 0x1c, 0xff);
    
    outgoing_bubble_.bg1_color_ = QColor(0xd8, 0xd4, 0xce, 0xe5);
    outgoing_bubble_.bg2_color_ = QColor(0xd5, 0xd2, 0xce, 0xb8);
    outgoing_bubble_.text_color_ = QColor(0x28, 0x28, 0x28, 0xff);
    outgoing_bubble_.time_color_ = QColor(0x97, 0x97, 0x97, 0xff);
    outgoing_bubble_.link_color_ = QColor(0x57, 0x9e, 0x1c, 0xff);
    outgoing_bubble_.info_color_ = QColor(0x28, 0x28, 0x28, 0xff);
    outgoing_bubble_.info_link_color_ = QColor(0x57, 0x9e, 0x1c, 0xff);
    
    preview_stickers_.time_color_ = QColor(0x97, 0x97, 0x97, 0xff);
    
    date_.bg_color_ = QColor(0x83, 0x83, 0x83, 0xff);
    date_.text_color_ = QColor(0xff, 0xff, 0xff, 0xff);
    
    chat_event_.bg_color_ = QColor(0xff, 0xff, 0xff, 0x00);
    chat_event_.text_color_ = QColor(0x97, 0x97, 0x97, 0xff);
    
    contact_name_.text_color_ = QColor(0x97, 0x97, 0x97, 0xff);
    
    new_messages_bubble_.bg_color_ = QColor(0x57, 0x9e, 0x1c, 0xff);
    new_messages_bubble_.bg_hover_color_ = QColor(0x60, 0xaa, 0x23, 0xff);
    new_messages_bubble_.bg_pressed_color_ = QColor(0x49, 0x88, 0x12, 0xff);
    new_messages_bubble_.text_color_ = QColor(0xff, 0xff, 0xff, 0xff);
    
    new_messages_plate_.bg_color_ = QColor(0x57, 0x9e, 0x1c, 0x7f);
    new_messages_plate_.text_color_ = QColor(0xff, 0xff, 0xff, 0xff);
    
    typing_.text_color_ = QColor(0x57, 0x54, 0x4c, 0xff);
    typing_.light_gif_ = 0;
    
    spinner_color_ = QColor(0x83, 0x86, 0x93, 0xff);
    edges_color_ = QColor(0xc7, 0xc7, 0xc7, 0xff);
    
    
    image_ = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_100.png"));
    Utils::check_pixel_ratio(image_);
    thumb_ = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_thumb_100.png"));
    Utils::check_pixel_ratio(thumb_);
}

QColor theme::get_tint_color()
{
    return tint_color_;
}

void theme::contact_list_item::unserialize(Ui::gui_coll_helper &_coll)
{
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
    name_color_ = colorFromString(_coll.get_value_as_string("name_color"));
    message_color_ = colorFromString(_coll.get_value_as_string("message_color"));
    sender_color_ = colorFromString(_coll.get_value_as_string("sender_color"));
    time_color_ = colorFromString(_coll.get_value_as_string("time_color"));
}

void theme::date::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
}

void theme::bubble::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg1_color_ = colorFromString(_coll.get_value_as_string("bg1_color"));
    bg2_color_ = colorFromString(_coll.get_value_as_string("bg2_color"));
    time_color_ = colorFromString(_coll.get_value_as_string("time_color"));
    link_color_ = colorFromString(_coll.get_value_as_string("link_color"));
    info_color_ = colorFromString(_coll.get_value_as_string("info_color"));
    info_link_color_  = colorFromString(_coll.get_value_as_string("info_link_color"));
}

void theme::preview_stickers::unserialize(Ui::gui_coll_helper &_coll)
{
    time_color_ = colorFromString(_coll.get_value_as_string("time_color"));
}

void theme::chat_event::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
}

void theme::contact_name::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
}

void theme::new_messages::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
}

void theme::new_messages_plate::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
}

void theme::new_messages_bubble::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
    bg_hover_color_ = colorFromString(_coll.get_value_as_string("bg_hover_color"));
    bg_pressed_color_ = colorFromString(_coll.get_value_as_string("bg_pressed_color"));
}

void theme::typing::unserialize(Ui::gui_coll_helper &_coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    light_gif_ = _coll.get_value_as_int("light_gif");
}

void theme::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");
    position_ = _coll.get_value_as_int("position");
    is_image_loaded_ = false;
    tint_color_ = colorFromString(_coll.get_value_as_string("tint_color"));
    tile_ = _coll.get_value_as_bool("tile");
    
    if (_coll.is_value_exist("thumb"))
    {
        core::istream* thumb_stream = _coll.get_value_as_stream("thumb");
        if (thumb_stream)
        {
            int32_t thumb_size = thumb_stream->size();
            load_thumb((char*)thumb_stream->read(thumb_size), thumb_size);
        }
    }

    Ui::gui_coll_helper coll_contact_list_item(_coll.get_value_as_collection("contact_list_item"), false);
    contact_list_item_.unserialize(coll_contact_list_item);

    Ui::gui_coll_helper coll_incoming_bubble(_coll.get_value_as_collection("incoming_bubble"), false);
    incoming_bubble_.unserialize(coll_incoming_bubble);
    
    Ui::gui_coll_helper coll_outgoing_bubble(_coll.get_value_as_collection("outgoing_bubble"), false);
    outgoing_bubble_.unserialize(coll_outgoing_bubble);
    
    Ui::gui_coll_helper coll_preview_stickers(_coll.get_value_as_collection("preview_stickers"), false);
    preview_stickers_.unserialize(coll_preview_stickers);
    
    Ui::gui_coll_helper coll_data(_coll.get_value_as_collection("date"), false);
    date_.unserialize(coll_data);
    
    Ui::gui_coll_helper coll_chat_event(_coll.get_value_as_collection("chat_event"), false);
    chat_event_.unserialize(coll_chat_event);
    
    Ui::gui_coll_helper coll_contact_name(_coll.get_value_as_collection("contact_name"), false);
    contact_name_.unserialize(coll_contact_name);
    
    Ui::gui_coll_helper coll_new_messages(_coll.get_value_as_collection("new_messages"), false);
    new_messages_.unserialize(coll_new_messages);
    
    Ui::gui_coll_helper coll_new_messages_plate(_coll.get_value_as_collection("new_messages_plate"), false);
    new_messages_plate_.unserialize(coll_new_messages_plate);
    
    Ui::gui_coll_helper coll_new_messages_bubble(_coll.get_value_as_collection("new_messages_bubble"), false);
    new_messages_bubble_.unserialize(coll_new_messages_bubble);
    
    Ui::gui_coll_helper coll_typing(_coll.get_value_as_collection("typing"), false);
    typing_.unserialize(coll_typing);
}

void cache::unserialize(const core::coll_helper &_coll)
{
    core::iarray* themes = _coll.get_value_as_array("themes");
    if (!themes)
        return;
    
    auto default_theme = get_qt_theme_settings()->getDefaultTheme();
    int default_theme_id = -1;
    if (default_theme)
    {
        default_theme_id = default_theme->get_id();
    }
    
    for (int32_t i = 0; i < themes->size(); i++)
    {
        core::coll_helper coll_set(themes->get_at(i)->get_as_collection(), false);
        auto theme_to_insert = std::make_shared<theme>();
        theme_to_insert->unserialize(coll_set);

        int theme_id = theme_to_insert->get_id();
        themes_[theme_id] = theme_to_insert;
    }
    
    get_qt_theme_settings()->themes_data_unserialized();
}

void cache::set_theme_data(core::coll_helper _coll)
{
    const qint32 theme_id = _coll.get_value_as_int("theme_id");
    core::istream* image_stream = _coll.get_value_as_stream("image");
    
    auto &theme = themes_[theme_id];
    if (theme && image_stream)
    {
        int32_t image_size = image_stream->size();
        if (image_size > 0)
        {
            QPixmap p;
            p.loadFromData((const uchar*)image_stream->read(image_size), image_size);
            theme->set_image(p);
        }
    }
}

void cache::unload_unused_themes_images(std::set<int> used)
{
    for (auto it = themes_.begin(); it != themes_.end(); ++it)
    {
        auto theme = it->second;
        int theme_id = theme->get_id();
        if (used.find(theme_id) == used.end() && theme->is_image_loaded())
        {
            theme->unload_image();
        }
    }
}

void theme::load_thumb(char* _data, int32_t _size)
{
    thumb_.loadFromData((const uchar*)_data, _size);
    Utils::check_pixel_ratio(thumb_);
}

void theme::unload_image()
{
    is_image_loaded_ = false;
    image_ = QPixmap();
}

cache& get_cache()
{
    if (!t_cache)
        t_cache.reset(new cache());
    
    return (*t_cache);
}

void unserialize(core::coll_helper _coll)
{
    get_cache().unserialize(_coll);
}

void unload_unused_themes_images(std::set<int> used)
{
    get_cache().unload_unused_themes_images(used);
}

void set_theme_data(core::coll_helper _coll)
{
    get_cache().set_theme_data(_coll);
}

themes_dict loaded_themes()
{
    return get_cache().get_themes();
}

UI_THEMES_NS_END