#ifndef __CORE_H__
#define __CORE_H__

#pragma once

#include "../corelib/core_face.h"
#include "../common.shared/common_defs.h"
#include "Voip/VoipManagerDefines.h"

namespace coretools
{
    class settings;
}

namespace voip_manager {
    class VoipManager;
}

namespace core
{
    class main_thread;
    class im_container;
    class coll_helper;
    class core_settings;
    class gui_settings;
    class theme_settings;
    class base_im;
    class scheduler;
    class async_task;
    class async_executer;
    struct async_task_handlers;
    class im_login_id;
    class ithread_callback;
    class network_log;
    struct proxy_settings;
    class proxy_settings_manager;

    namespace archive
    {
        class local_history;
    }

    namespace update
    {
        class updater;
    }

    namespace stats
    {
        class statistics;
    }

    namespace dump
    {
        class report_sender;
    }

    namespace stats
    {
        enum class stats_event_names;
    }

    class core_dispatcher
    {
        main_thread* core_thread_;
        std::unique_ptr<network_log> network_log_;
        std::unique_ptr<ithread_callback> http_handles_;
        std::unique_ptr<scheduler> scheduler_;

        std::shared_ptr<im_container> im_container_;
        std::shared_ptr<voip_manager::VoipManager> voip_manager_;

        std::shared_ptr<core::core_settings> settings_;
        std::shared_ptr<core::gui_settings> gui_settings_;
        std::shared_ptr<core::theme_settings> theme_settings_;
        std::shared_ptr<core::stats::statistics> statistics_;
        std::shared_ptr<core::proxy_settings_manager> proxy_settings_manager_;

        std::shared_ptr<dump::report_sender> report_sender_;

        // gui interfaces
        iconnector* gui_connector_;
        icore_factory* core_factory_;
        std::unique_ptr<async_executer> save_thread_;
        std::shared_ptr<archive::local_history> local_history_;

        std::mutex user_proxy_mutex_;

        // updater
        std::unique_ptr<update::updater> updater_;

        common::core_gui_settings core_gui_settings_;

        std::atomic_uchar search_count_;

        void load_gui_settings();
        void load_proxy_settings();
        void post_gui_settings();
        void post_app_config();
        void on_message_update_gui_settings_value(int64_t _seq, coll_helper _params);
        void on_message_log(coll_helper _params) const;
        void on_message_profiler_proc_start(coll_helper _params) const;
        void on_message_profiler_proc_stop(coll_helper _params) const;

        void post_data_path();
        void load_theme_settings();
        void post_theme_settings();
        void on_message_update_theme_settings_value(int64_t _seq, coll_helper _params);
        void on_message_set_default_theme_id(int64_t _seq, coll_helper _params);

        void load_statistics();

    public:

        core_dispatcher();
        virtual ~core_dispatcher();

        void start(const common::core_gui_settings&);
        std::string get_uniq_device_id();
        void excute_core_context(std::function<void()> _func);
        void excute_core_context_priority(std::function<void()> _func);

        uint32_t add_timer(std::function<void()> _func, uint32_t _timeout);
        void stop_timer(uint32_t _id);

        std::shared_ptr<async_task_handlers> save_async(std::function<int32_t()> task);

        icollection* create_collection();

        void link_gui(icore_interface* _core_face, const common::core_gui_settings& _settings);
        void unlink_gui();

        void post_message_to_gui(const char * _message, int64_t _seq, icollection* _message_data);
        void receive_message_from_gui(const char * _message, int64_t _seq, icollection* _message_data);

        const common::core_gui_settings& get_core_gui_settings() const;

        bool is_valid_search();
        void begin_search();
        unsigned end_search();

        void unlogin();

        std::string get_root_login();
        std::string get_login_after_start();
        std::string get_contact_friendly_name(unsigned _id, const std::string& _contact_login);
        std::shared_ptr<base_im> find_im_by_id(unsigned _id);

        void update_login(im_login_id& _login);
        void replace_uin_in_login(im_login_id& old_login, im_login_id& new_login);

        void post_voip_message(unsigned _id, const voip_manager::VoipProtoMsg& msg);
        void post_voip_alloc(unsigned _id, const char* _data, size_t _len);

        void insert_event(core::stats::stats_event_names _event);
        void insert_event(core::stats::stats_event_names _event, const core::stats::event_props_type& _props);
        void start_session_stats();
        bool is_stats_enabled() const;

        void on_thread_finish();

        network_log& get_network_log();

        proxy_settings get_auto_proxy_settings();
        proxy_settings get_proxy_settings();
        proxy_settings get_registry_proxy_settings();
        proxy_settings get_user_proxy_settings();

        void set_user_proxy_settings(const proxy_settings& _user_proxy_settings);

        bool locale_was_changed() const;
        void set_locale(const std::string& _locale);
        std::string get_locale() const;

        void switch_proxy_settings();

        proxy_settings get_core_proxy_settings();
        void post_user_proxy_to_gui();

        std::thread::id get_core_thread_id() const;
        bool is_core_thread() const;

        void save_themes_etag(const std::string &etag);
        std::string load_themes_etag();

        //template<typename __ParamType> void on_voip_signal(voip_manager::eNotificationTypes type, const __ParamType* param) {
        //    coll_helper coll(create_collection(), true);
        //    type >> coll;
        //    if (param) {
        //        *param >> coll;
        //    }
        //    post_message_to_gui(L"voip_signal", 0, coll.get());
        //}
        //void on_voip_signal(voip_manager::eNotificationTypes type) {
        //    coll_helper coll(create_collection(), true);
        //    type >> coll;
        //    post_message_to_gui(L"voip_signal", 0, coll.get());
        //}

		// We save fix voip mute flag in settings,
		// Because we want to call this fix once.
		void set_voip_mute_fix_flag(bool bValue);
		bool get_voip_mute_fix_flag();
    };

    extern std::unique_ptr<core::core_dispatcher>		g_core;
}



#endif // __CORE_H__

