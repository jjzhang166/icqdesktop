#include "stdafx.h"

#include "../../corelib/collection_helper.h"
#include "../tools/system.h"

#include "app_config.h"

CORE_CONFIGURATION_NS_BEGIN

    namespace pt = boost::property_tree;

namespace
{
    std::unique_ptr<app_config> config_;

    const IntSet& valid_dpi_values();
}

app_config::app_config()
    : is_server_history_enabled_(true)
    , forced_dpi_(0)
    , is_crash_enabled_(false)
    , full_log_(false)
{

}

app_config::app_config(const bool _is_server_history_enabled,
                       const int _forced_dpi,
                       const bool _is_crash_enabled,
                       const bool _full_log)
                       : is_server_history_enabled_(_is_server_history_enabled)
                       , forced_dpi_(_forced_dpi)
                       , is_crash_enabled_(_is_crash_enabled)
                       , full_log_(_full_log)
{
    assert(valid_dpi_values().count(forced_dpi_) > 0);
}

void app_config::serialize(Out core::coll_helper &_collection) const
{
    _collection.set_value_as_bool("history.is_server_history_enabled", is_server_history_enabled_);

    if (forced_dpi_ != 0)
    {
        assert(valid_dpi_values().count(forced_dpi_) > 0);
        _collection.set_value_as_int("gui.forced_dpi", forced_dpi_);
    }
}

const app_config& get_app_config()
{
    assert(config_);

    return *config_;
}

void load_app_config(const boost::filesystem::wpath &_path)
{
    assert(!config_);

    if (!core::tools::system::is_exist(_path))
    {
        config_.reset(new app_config());
        return;
    }

    pt::ptree options;
    pt::ini_parser::read_ini(_path.string(), Out options);

    const auto disable_server_history = options.get<bool>("disable_server_history", false);

    auto forced_dpi = options.get<int>("gui.force_dpi", 0);
    if (valid_dpi_values().count(forced_dpi) == 0)
    {
        forced_dpi = 0;
    }

    const auto enable_crash = options.get<bool>("enable_crash", false);
    const auto full_log = options.get<bool>("fulllog", false);
    
    config_.reset(new app_config(
        !disable_server_history, 
        forced_dpi, 
        enable_crash, 
        full_log));
}

namespace
{
    const IntSet& valid_dpi_values()
    {
        static IntSet values;

        if (values.empty())
        {
            values.insert(0);
            values.insert(100);
            values.insert(125);
            values.insert(150);
            values.insert(200);
        }

        return values;
    }
}

CORE_CONFIGURATION_NS_END