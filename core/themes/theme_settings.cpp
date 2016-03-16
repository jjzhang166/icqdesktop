#include "stdafx.h"

#include "theme_settings.h"

void core::theme_settings::set_value(const std::string& _name, const tools::binary_stream& _data)
{
    values_[_name] = _data;
    changed_ = true;
}

void core::theme_settings::set_value(const std::string& _name, const themes::theme& _theme)
{
    tools::binary_stream tc_bs_data;
    std::string tint_color = _theme.get_tint_color();
    uint32_t size = (uint32_t)tint_color.length();
    if (size)
    {
        tc_bs_data.write((const char*)tint_color.c_str(), size);
        set_value("tint_color", tc_bs_data);
    }

    tools::binary_stream image_bs_data;
    const std::wstring image_normal_name = _theme.get_image_path();
    if (image_bs_data.load_from_file(image_normal_name))
    {
        set_value("image", image_bs_data);
    }
    
    tools::binary_stream thumb_bs_data;
    const std::wstring thumb_name = _theme.get_thumb_path();
    if (thumb_bs_data.load_from_file(thumb_name))
    {
        set_value("thumb", thumb_bs_data);
    }
    
    const int theme_id = _theme.get_theme_id();   
    tools::binary_stream bs_id;
    bs_id.write((const char*)&theme_id, sizeof(int));
    set_value("id", bs_id);
    
    const bool tile = _theme.is_tile();
    tools::binary_stream bs_tile;
    bs_tile.write((const char*)&tile, 1);
    set_value("tile", bs_tile);
    
    changed_ = true;
}