#ifndef __WIM_ACTIVE_DILOGS
#define __WIM_ACTIVE_DILOGS

#pragma once

namespace core
{
    struct icollection;

    namespace wim
    {
        class active_dialog
        {
            std::string		aimid_;

        public:
            active_dialog();
            active_dialog(const std::string& _aimid);

            std::string get_aimid() const { return aimid_; }

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);
        };

        class active_dialogs
        {
            bool	changed_;

            std::list<active_dialog>	dialogs_;

        public:

            void enumerate(std::function<void(const active_dialog&)> _callback);
            size_t size() const;

            bool is_changed() { return changed_; }
            void set_changed(bool _changed) { changed_ = _changed; }

            active_dialogs();
            virtual ~active_dialogs();

            void update(active_dialog& _dialog);
            void remove(const std::string& _aimid);
            bool contains(const std::string& _aimId);

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);
        };

    }
}


#endif //__WIM_ACTIVE_DILOGS