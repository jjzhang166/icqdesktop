#include "stdafx.h"
#include "core_settings.h"

#include "tools/system.h"
#include "proxy_settings.h"

using namespace core;

core_settings::core_settings(const boost::filesystem::wpath& _path)
    : file_name_(_path.wstring())
{
}


core_settings::~core_settings()
{
}

void core_settings::init_default()
{
    set_value<std::string>(core_settings_values::csv_device_id, core::tools::system::generate_guid());

    std::locale loc = boost::locale::generator().generate("");
    std::string locale, lang, country;
    lang = std::use_facet<boost::locale::info>(loc).language();
    country = std::use_facet<boost::locale::info>(loc).country();
    if (!lang.empty() && !country.empty())
        locale = lang + "-" + country;

    set_value(core_settings_values::core_settings_locale, locale.empty() ? locale : boost::locale::to_lower(locale, loc));
}

bool core_settings::save()
{
    core::tools::binary_stream bstream;
    serialize(bstream);
    return bstream.save_2_file(file_name_);
}

bool core_settings::load()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(file_name_))
        return false;

    return unserialize(bstream);
}

void core_settings::set_user_proxy_settings(const proxy_settings& _user_proxy_settings)
{
    tools::binary_stream bs_data;
    _user_proxy_settings.serialize(bs_data);
    set_value(core_settings_values::core_settings_proxy, bs_data);
    save();
}

void core_settings::set_locale_was_changed(bool _was_changed)
{
    set_value(core_settings_values::core_settings_locale_was_changed, _was_changed);
    save();
}

bool core_settings::get_locale_was_changed() const
{
    return get_value(core_settings_values::core_settings_locale_was_changed, true);
}

void core_settings::set_locale(const std::string& _locale)
{
    set_value(core_settings_values::core_settings_locale, _locale);
    save();
}

std::string core_settings::get_locale() const
{
    return get_value<std::string>(core_settings_values::core_settings_locale, std::string());
}

proxy_settings core_settings::get_user_proxy_settings()
{
    proxy_settings core_proxy_settings;
    auto stream = get_value<tools::binary_stream>(core_settings_values::core_settings_proxy, tools::binary_stream());
    core_proxy_settings.unserialize(stream);
    return core_proxy_settings;
}

bool core_settings::get_voip_mute_fix_flag() const
{
	bool res = false;
	get_value(core_settings_values::voip_mute_fix_flag, &res);
	return res;
}

void core_settings::set_voip_mute_fix_flag(bool bValue)
{
    set_value(core_settings_values::voip_mute_fix_flag, bValue);
    save();
}
