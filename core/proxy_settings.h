#pragma once

#include "../corelib/enumerations.h"

namespace core
{
    class coll_helper;

    struct proxy_settings
    {
        static const int32_t default_proxy_port;

        bool			use_proxy_;
        std::wstring	proxy_server_;
        int32_t     	proxy_port_;
        bool			need_auth_;
        std::wstring	login_;
        std::wstring	password_;
        int32_t			proxy_type_;

        proxy_settings()
            : use_proxy_(false)
            , need_auth_(false)
            , proxy_type_((int32_t)core::proxy_types::auto_proxy)
            , proxy_port_(default_proxy_port)
        {
        }

        void serialize(tools::binary_stream& _bs) const;
        bool unserialize(tools::binary_stream& _bs);

        void serialize(core::coll_helper _collection) const;
    };

    class proxy_settings_manager
    {
        std::mutex      mtx_;
        proxy_settings	settings_;
        proxy_settings  registry_settings_;
        proxy_settings  user_settings_;
        bool switched_;

    public:
        proxy_settings_manager();
        virtual ~proxy_settings_manager();

        proxy_settings get();
        proxy_settings get_registry_settings();
        void switch_settings();
        proxy_settings get_user_proxy_settings();

    private:
        void init_from_registry();
    };

}