#pragma once

namespace core
{
    struct proxy_settings
    {
        bool			use_proxy_;
        std::wstring	proxy_server_;
        bool			need_auth_;
        std::wstring	login_;
        std::wstring	password_;
        int32_t			proxy_type_;

        proxy_settings()
            :
            use_proxy_(false),
            need_auth_(false),
            proxy_type_(-1)
        {
        }

    };

	class proxy_settings_manager
	{
        std::mutex      mtx_;
		proxy_settings	settings_;
        proxy_settings  registry_settings_;
        bool switched_;

	public:
		proxy_settings_manager();
		virtual ~proxy_settings_manager();

        proxy_settings get();
        proxy_settings get_registry_settings();
        void switch_settings();

    private:
		void init_from_registry();
	};

}