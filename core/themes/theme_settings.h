#pragma once

#include "../gui_settings.h"
#include "themes.h"

namespace core
{
    class theme_settings : public gui_settings
    {        
    public:
        theme_settings(const std::wstring& _file_name) : gui_settings(_file_name, L"") {}
        virtual void set_value(const std::string& _name, const tools::binary_stream& _data) override;
        void set_value(const std::string& _name, const themes::theme& _theme);
    };
    
}