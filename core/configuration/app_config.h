#pragma once

#define CORE_CONFIGURATION_NS_BEGIN namespace core { namespace configuration {
#define CORE_CONFIGURATION_NS_END }}

CORE_CONFIGURATION_NS_BEGIN

struct app_config
{
    app_config();

    app_config(
        const bool _is_server_history_enabled,
        const int _forced_dpi, 
        const bool _is_crash_enabled,
        const bool _full_log);

    void serialize(Out core::coll_helper &_collection) const;

    const bool is_server_history_enabled_;

    const int forced_dpi_;

    const bool is_crash_enabled_;

    const bool full_log_;
};

const app_config& get_app_config();

void load_app_config(const boost::filesystem::wpath &_path);

CORE_CONFIGURATION_NS_END