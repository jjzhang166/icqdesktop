#include "stdafx.h"
#include "core.h"
#include "async_task.h"
#include "main_thread.h"
#include "network_log.h"
#include "configuration/app_config.h"

#include "connections/im_container.h"
#include "connections/base_im.h"
#include "connections/login_info.h"
#include "connections/im_login.h"

#include "Voip/VoipManager.h"

#include "../corelib/collection_helper.h"
#include "core_settings.h"
#include "gui_settings.h"
#include "themes/theme_settings.h"
#include "utils.h"
#include "http_request.h"
#include "scheduler.h"
#include "archive/local_history.h"
#include "log/log.h"
#include "profiling/profiler.h"
#include "updater/updater.h"
#include "crash_sender.h"
#include "../common.shared/crash_handler.h"
#include "statistics.h"
#include "tools/md5.h"
#include "../corelib/enumerations.h"
#include "../corelib/common.h"
#include "tools/system.h"
#include "proxy_settings.h"

using namespace core;

std::unique_ptr<core::core_dispatcher>	core::g_core;


core_dispatcher::core_dispatcher()
    :	gui_connector_(nullptr),
    core_factory_(nullptr),
    core_thread_(nullptr)
{
    http_request_simple::init_global();
#ifdef _WIN32
    std::locale loc = boost::locale::generator().generate("");
    std::locale::global(loc);
#endif
    search_count_.store(0);
        
    __LOG(log::init(utils::get_logs_path(), false);)
            
    profiler::enable(::build::is_debug());
}


core_dispatcher::~core_dispatcher()
{
    profiler::flush_logs();

    __LOG(log::shutdown();)

    http_request_simple::shutdown_global();
}

std::string core::core_dispatcher::get_uniq_device_id()
{
    return settings_->get_value<std::string>(core_settings_values::csv_device_id, std::string());
}

void core::core_dispatcher::excute_core_context(std::function<void()> func)
{
    if (!core_thread_)
    {
        assert(!"core thread empty");
        return;
    }

    core_thread_->excute_core_context(func);
}

std::shared_ptr<base_im> core::core_dispatcher::find_im_by_id(unsigned id) {
    assert(!!im_container_);
    if (!!im_container_) {
        return im_container_->get_im_by_id(id);
    }
    return NULL;
}

uint32_t core::core_dispatcher::add_timer(std::function<void()> _func, uint32_t _timeout)
{
    if (!scheduler_)
    {
        assert(false);
        return 0;
    }

    return scheduler_->push_timer(_func, _timeout);
}

void core::core_dispatcher::stop_timer(uint32_t _id)
{
    if (scheduler_)
        scheduler_->stop_timer(_id);
}

std::shared_ptr<async_task_handlers> core::core_dispatcher::save_async(std::function<int32_t()> task)
{
    assert(!!save_thread_);
    if (!save_thread_)
        return std::make_shared<async_task_handlers>();
    return save_thread_->run_async_function(task);
}


void core::core_dispatcher::link_gui(icore_interface* _core_face)
{
    // called from main thread

    gui_connector_ = _core_face->get_gui_connector();
    core_factory_ = _core_face->get_factory();
    core_thread_ = new main_thread();

    excute_core_context([this]
    {
        start();
    });
}

void core::core_dispatcher::start()
{
    // called from core thread
    network_log_.reset(new network_log(utils::get_logs_path()));

    const boost::filesystem::wpath product_data_root = utils::get_product_data_path();

    configuration::load_app_config(product_data_root / L"app.ini");

    http_handles_.reset(http_request_simple::create_http_handlers());

    settings_ = std::make_shared<core::core_settings>(product_data_root / L"settings/core.stg");
    if (!settings_->load())
    {
        settings_->init_default();
        settings_->save();
    }

    save_thread_.reset(new async_executer());
    scheduler_.reset(new scheduler());

    load_gui_settings();
    load_theme_settings();
    load_proxy_settings();
    post_theme_settings();
    post_gui_settings();
    post_app_config();
#ifndef STRIP_VOIP
    voip_manager_.reset(new(std::nothrow) voip_manager::VoipManager(*this));
    assert(!!voip_manager_);
#endif //__STRIP_VOIP
    im_container_.reset(new im_container(*voip_manager_));
    im_container_->create();

    updater_.reset(new update::updater());

#ifdef _WIN32
    core::dump::set_product_data_path(core::utils::get_product_data_path());
    core::dump::set_os_version(core::tools::system::get_os_version_string());
    report_sender_.reset(new dump::report_sender(g_core->get_login_after_start()));
    report_sender_->send_report();
#endif

    load_statistics();
}


void core::core_dispatcher::unlink_gui()
{
    excute_core_context([this]
    {
        // NOTE : this order is important!
        
        voip_manager_.reset();
        im_container_.reset();
        gui_settings_.reset();
        scheduler_.reset();
        if (is_stats_enabled())
            statistics_.reset();
        save_thread_.reset();
        updater_.reset();
        report_sender_.reset();
        network_log_.reset();
        http_handles_.reset();
        proxy_settings_manager_.reset();
        theme_settings_.reset();
    });

    delete core_thread_;
    core_thread_ = nullptr;

    gui_connector_->release();
    gui_connector_ = nullptr;

    core_factory_->release();
    core_factory_ = nullptr;
}


void core::core_dispatcher::post_message_to_gui(const char * _message, int64_t _seq, icollection* _message_data)
{
    __LOG(core::log::info("core", boost::format("post message to gui, message=%1%\nparameters: %2%") % _message % (_message_data ? _message_data->log() : ""));)

        if (!gui_connector_)
        {
            assert(!"gui unlinked");
            return;
        }

        gui_connector_->receive(_message, _seq, _message_data);
}


icollection* core::core_dispatcher::create_collection()
{
    if (!core_factory_)
    {
        assert(!"core factory empty");
        return nullptr;
    }

    return core_factory_->create_collection();
}


void core_dispatcher::load_gui_settings()
{
    gui_settings_ = std::make_shared<core::gui_settings>(
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(gui_settings_file_name),
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(settings_export_file_name));

    gui_settings_->load();
    gui_settings_->start_save();
}

void core_dispatcher::load_proxy_settings()
{
    proxy_settings_manager_ = std::make_shared<core::proxy_settings_manager>();
}

bool core_dispatcher::is_stats_enabled() const
{
    return true;
}

void core_dispatcher::load_statistics()
{
    if (!is_stats_enabled())
        return;

    statistics_ = std::make_shared<core::stats::statistics>(utils::get_product_data_path() + L"/stats/stats.stg");
    g_core->excute_core_context([this]
    {
        statistics_->init();
        start_session_stats();
    });
}

void core_dispatcher::start_session_stats()
{
    core::stats::event_props_type props;

    std::string uin = "";
    auto im = im_container_->get_im_by_id(1);
    if (im)
    {
        uin = g_core->get_root_login();
    }

    auto user_key = uin + g_core->get_uniq_device_id();
    auto hashed_user_key = core::tools::md5(user_key.c_str(), (int32_t)user_key.length());
    props.emplace_back(std::make_pair("hashed_user_key", hashed_user_key));
    props.emplace_back(std::make_pair("Sys_RAM", std::to_string(core::tools::system::get_memory_size_mb())));

    std::locale loc("");
    props.emplace_back(std::make_pair("Sys_Language", loc.name()));
    props.emplace_back(std::make_pair("Sys_OS_Version", core::tools::system::get_os_version_string()));
    
    insert_event(core::stats::stats_event_names::start_session, props);
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event)
{
    core::stats::event_props_type props;
    insert_event(_event, props);
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event, const core::stats::event_props_type& _props)
{
    if (!is_stats_enabled())
        return;

    auto wr_stats = std::weak_ptr<core::stats::statistics>(statistics_);

    g_core->excute_core_context([wr_stats, _event, _props]
    {
        auto ptr_stats = wr_stats.lock();
        if (!ptr_stats)
            return;

        ptr_stats->insert_event(_event, _props);
    });
}

void core_dispatcher::load_theme_settings()
{
    theme_settings_ = std::make_shared<core::theme_settings>(utils::get_product_data_path() + L"/settings/theme.stg");
    theme_settings_->load();
    theme_settings_->start_save();
}

void core_dispatcher::post_gui_settings()
{
    coll_helper cl_coll(create_collection(), true);
    gui_settings_->serialize(cl_coll);

    cl_coll.set_value_as_string("data_path", core::tools::from_utf16(core::utils::get_product_data_path()));
    cl_coll.set_value_as_string("os_version", core::tools::system::get_os_version_string());
    post_message_to_gui("gui_settings", 0, cl_coll.get());
}

void core_dispatcher::post_theme_settings()
{
    coll_helper cl_coll(create_collection(), true);
    
    theme_settings_->serialize(cl_coll);
    
    post_message_to_gui("theme_settings", 0, cl_coll.get());
}

void core_dispatcher::post_app_config()
{
    coll_helper cl_coll(create_collection(), true);

    configuration::get_app_config().serialize(Out cl_coll);

    post_message_to_gui("app_config", 0, cl_coll.get());
}

void core::core_dispatcher::on_message_update_gui_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");

    tools::binary_stream bs_data;
    int size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);

    gui_settings_->set_value(value_name, bs_data);
}

void core::core_dispatcher::on_message_update_theme_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");
    
    tools::binary_stream bs_data;
    int size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);
    
    theme_settings_->set_value(value_name, bs_data);
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::on_message_set_default_theme_id(int64_t _seq, coll_helper _params)
{
    const int theme_id = _params.get_value_as_int("id");
    auto im = im_container_->get_im_by_id(1);
    const themes::theme *theme = im->get_theme_from_cache(theme_id);
    if (theme)
    {
        theme_settings_->set_value("default_theme", *theme);
        theme_settings_->save_if_needed();
    }
}

void core::core_dispatcher::on_message_log(coll_helper _params) const
{
    const auto type = _params.get_value_as_string("type");
    const auto area = _params.get_value_as_string("area");
    const auto text = _params.get_value_as_string("text");

    if (!::strcmp(type, "trace"))
    {
        log::trace(area, text);
        return;
    }

    if (!::strcmp(type, "info"))
    {
        log::info(area, text);
        return;
    }

    if (!::strcmp(type, "warn"))
    {
        log::warn(area, text);
        return;
    }

    if (!::strcmp(type, "error"))
    {
        log::error(area, text);
        return;
    }

    assert(!"unknown log record type");
}

void core::core_dispatcher::on_message_profiler_proc_start(coll_helper _params) const
{
    const auto name = _params.get_value_as_string("name");
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_started(name, id, ts);
}

void core::core_dispatcher::on_message_profiler_proc_stop(coll_helper _params) const
{
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_stopped(id, ts);
}

void core::core_dispatcher::receive_message_from_gui(const char * _message, int64_t _seq, icollection* _message_data)
{
    // called from main thread
    std::string message_string = _message;

    __LOG(
        if (message_string != "log")
        {
            core::log::info("core", boost::format("message from gui, message=%1%\nparameters: %2%") % _message % (_message_data ? _message_data->log() : ""));
        })	

        if (_message_data)
            _message_data->addref();

        if (message_string == "search")
            begin_search();

        excute_core_context([this, message_string, _seq, _message_data]
        {
            coll_helper params(_message_data, true);

            if (message_string == "settings/value/set")
            {
                on_message_update_gui_settings_value(_seq, params);
            }
            else if (message_string == "log")
            {
                on_message_log(params);
            }
            else if (message_string == "profiler/proc/start")
            {
                on_message_profiler_proc_start(params);
            }
            else if (message_string == "profiler/proc/stop")
            {
                on_message_profiler_proc_stop(params);
            }
            else if (message_string == "themes/settings/set")
            {
                on_message_update_theme_settings_value(_seq, params);
            }
            else if (message_string == "themes/default/id")
            {
                on_message_set_default_theme_id(_seq, params);
            }
            else
            {
                im_container_->on_message_from_gui(message_string.c_str(), _seq, params);
            }
        });
}

std::string core::core_dispatcher::get_root_login()
{
    auto im = im_container_->get_im_by_id(1);
    if (!im)
    {
        assert(false);
        return "";
    }

    return im->get_login();
}

std::string core::core_dispatcher::get_login_after_start()
{
    if (!im_container_)
    {
        assert(false);
        return "";
    }
    return im_container_->get_first_login();
}

std::string core::core_dispatcher::get_contact_friendly_name(unsigned _id, const std::string& _contact_login) {
    assert(!!im_container_);
    if (!!im_container_) {
        auto im = im_container_->get_im_by_id(_id);

        assert(!!im);
        if (!!im) {
            return im->get_contact_friendly_name(_contact_login);
        }
    }

    return "";
}

bool core::core_dispatcher::is_valid_search()
{
    return search_count_.load() == 1;
}

void core::core_dispatcher::begin_search()
{
    ++search_count_;
}

unsigned core::core_dispatcher::end_search()
{
    return --search_count_;
}

void core::core_dispatcher::update_login(im_login_id& _login)
{
    im_container_->update_login(_login);
    start_session_stats();
}

void core::core_dispatcher::post_voip_message(unsigned _id, const voip_manager::VoipProtoMsg& msg) {
    excute_core_context([this, _id, msg] {
        auto im = im_container_->get_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_msg_to_server(msg);
        }
    });
}

void core::core_dispatcher::post_voip_alloc(unsigned _id, const char* _data, size_t _len) {
    std::string data_str(_data, _len);
    excute_core_context([this, _id, data_str] {
        auto im = im_container_->get_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_alloc_to_server(data_str);
        }
    });
}

void core::core_dispatcher::on_thread_finish()
{
    assert(http_handles_.get());

    if (http_handles_)
    {
        http_handles_->on_thread_shutdown();
    }
}

void core::core_dispatcher::unlogin()
{
    excute_core_context([this]()
    {
        im_container_->unlogin();
    });
}

network_log& core::core_dispatcher::get_network_log()
{
    return (*network_log_);
}

proxy_settings core_dispatcher::get_proxy_settings()
{
    return proxy_settings_manager_->get();
}

proxy_settings core_dispatcher::get_registry_proxy_settings()
{
    return proxy_settings_manager_->get();
}

void core_dispatcher::switch_proxy_settings()
{
    proxy_settings_manager_->switch_settings();
}
