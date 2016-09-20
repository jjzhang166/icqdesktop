#ifndef __DLG_STATE_H_
#define __DLG_STATE_H_

#pragma once

#include <stdint.h>

//////////////////////////////////////////////////////////////////////////
// dlg_state class
//////////////////////////////////////////////////////////////////////////
namespace core
{
    struct icollection;

    namespace archive
    {
        class storage;
        class history_message;

        class dlg_state
        {
            uint32_t unread_count_;
            int64_t last_msgid_;
            int64_t yours_last_read_;
            int64_t del_up_to_;
            int64_t theirs_last_read_;
            int64_t theirs_last_delivered_;

            bool visible_;

            bool fake_;

            bool official_;

            std::string last_message_friendly_;

            std::unique_ptr<history_message> last_message_;

            std::string friendly_;

            // patch version from getWimHistory (stored in db)
            std::string history_patch_version_;

            // current patch version (not stored in db)
            std::string dlg_state_patch_version_;

        public:

            dlg_state();
            dlg_state(const dlg_state& _state);
            virtual ~dlg_state();

            dlg_state& operator=(const dlg_state& _state);

            void copy_from(const dlg_state& _state);

            void set_unread_count(uint32_t _unread_count) { unread_count_ = _unread_count; }
            uint32_t get_unread_count() const { return unread_count_; }

            void set_last_msgid(const int64_t _value);
            int64_t get_last_msgid() const { return last_msgid_; }
            bool has_last_msgid() const { return (last_msgid_ > 0); }
            void clear_last_msgid() { last_msgid_ = -1; }

            void set_yours_last_read(int64_t _val) { yours_last_read_ = _val; }
            int64_t get_yours_last_read() const { return yours_last_read_; }

            void set_theirs_last_read(int64_t _val) { theirs_last_read_ = _val; }
            int64_t get_theirs_last_read() const { return theirs_last_read_; }

            void set_theirs_last_delivered(int64_t _val) { theirs_last_delivered_ = _val; }
            int64_t	get_theirs_last_delivered() const { return theirs_last_delivered_; }

            void set_visible(bool _val) { visible_ = _val; }
            bool get_visible() const { return visible_; }

            void set_fake(bool _val) { fake_ = _val; }
            bool get_fake() const { return fake_; }

            void set_friendly(const std::string& _friendly) {friendly_ = _friendly;}
            const std::string& get_friendly() const {return friendly_;}

            void set_official(bool _official) {official_ = _official;}
            bool get_official() const {return official_;}

            const history_message& get_last_message() const;
            void set_last_message(const history_message& _message);
            bool has_last_message() const;
            void clear_last_message();

            const std::string& get_last_message_friendly() const;
            void set_last_message_friendly(const std::string& _friendly);

            std::string get_history_patch_version(const std::string& _default = std::string()) const;
            bool has_history_patch_version() const;
            void set_history_patch_version(const std::string& _patch_version);
            void reset_history_patch_version();

            const std::string& get_dlg_state_patch_version() const;
            void set_dlg_state_patch_version(const std::string& _patch_version);

            int64_t get_del_up_to() const;
            void set_del_up_to(const int64_t _msg_id);
            bool has_del_up_to() const;

            bool is_empty() const;

            void serialize(icollection* _collection, const time_t _offset, const time_t _last_successful_fetch, const bool _serialize_message = true) const;
            void serialize(core::tools::binary_stream& _data) const;

            bool unserialize(core::tools::binary_stream& _data);
        };
        //////////////////////////////////////////////////////////////////////////

        struct dlg_state_changes
        {
            dlg_state_changes();

            bool del_up_to_changed_;

            bool history_patch_version_changed_;

            bool initial_fill_;

            bool last_message_changed_;
        };

        class archive_state
        {
            std::unique_ptr<dlg_state>	state_;
            std::unique_ptr<storage>	storage_;
            const std::string           contact_id_;

            bool load();

        public:

            archive_state(const std::wstring& _file_name, const std::string& _contact_id);
            ~archive_state();

            void merge_state(const dlg_state& _new_state, Out dlg_state_changes& _changes);

            bool save();

            const dlg_state& get_state();
            void set_state(const dlg_state& _state, Out dlg_state_changes& _changes);
            void clear_state();
        };
    }
}


#endif //__DLG_STATE_H_