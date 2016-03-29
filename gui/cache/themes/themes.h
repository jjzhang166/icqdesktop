#ifndef themes_hpp
#define themes_hpp
#include "../../../corelib/collection_helper.h"
#include "../../core_dispatcher.h"
#define UI_THEMES_NS_BEGIN namespace Ui { namespace themes {
#define UI_THEMES_NS_END } }

UI_THEMES_NS_BEGIN

class theme
{
    int id_;
    int position_;
    QColor tint_color_;
    QPixmap thumb_;
    QPixmap image_;
    bool tile_;
    QColor spinner_color_;
    QColor edges_color_;
    bool is_image_loaded_;
public:
    theme();
    theme(int _id, QString _tint_color, QByteArray& _imageData, QByteArray& _thumbData, const bool _tile);
    void unserialize(core::coll_helper _coll);
    void load_thumb(char* _data, int32_t _size);
    QPixmap get_thumb() const { return thumb_; }
    void set_thumb(QPixmap pixmap) { thumb_ = pixmap; }
    int get_id() const { return id_; }
    bool is_tile() const { return tile_; }
    QPixmap get_image() const {
        QPixmap ret = image_;
        return ret;
    }
    int get_position() const { return position_; }
    QColor get_tint_color();
    void initDefault();
    void set_image(QPixmap &_pixmap)
    {
        image_ = QPixmap(_pixmap);
        if (!image_.isNull())
        {
            is_image_loaded_ = true;
        }
    }
    bool is_image_loaded() const
    {
        return is_image_loaded_;
    }
    
    void unload_image();
    
    static QColor colorFromString(const char* _colorString);
    
    struct contact_list_item {
        QColor bg_color_;
        QColor name_color_;
        QColor message_color_;
        QColor sender_color_;
        QColor time_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } contact_list_item_;

    struct bubble {
        QColor bg1_color_;
        QColor bg2_color_;
        QColor text_color_;
        QColor time_color_;
        QColor link_color_;
        QColor info_color_;
        QColor info_link_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    };
    bubble incoming_bubble_;
    bubble outgoing_bubble_;
    
    struct date {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } date_;
    
    struct preview_stickers {
        QColor time_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } preview_stickers_;
    
    struct chat_event {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } chat_event_;
    
    struct contact_name {
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } contact_name_;
    
    struct new_messages {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } new_messages_;
    
    struct new_messages_plate {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } new_messages_plate_;
    
    struct new_messages_bubble {
        QColor bg_color_;
        QColor bg_hover_color_;
        QColor bg_pressed_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } new_messages_bubble_;
    
    struct typing {
        int light_gif_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& coll);
    } typing_;
};

typedef std::shared_ptr<theme> themePtr;

typedef std::map<int,themePtr> themes_dict;
typedef std::vector<themePtr> themes_list;
class cache
{
public:
    cache(){}
    
    void unserialize(const core::coll_helper &_coll);
    void set_theme_data(core::coll_helper _coll);
    themes_dict get_themes() { return themes_; }
    void unload_unused_themes_images(std::set<int> used);
    
private:
    themes_dict themes_;
    
};

void unserialize(core::coll_helper _coll);
void set_theme_data(core::coll_helper _coll);
void unload_unused_themes_images(std::set<int> used);
themes_dict loaded_themes();

UI_THEMES_NS_END

#endif /* themes_h */
