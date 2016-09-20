#pragma once

#include "../async_task.h"
#include "../namespaces.h"

#include "options.h"

CORE_TOOLS_NS_BEGIN

struct url_info;

typedef std::vector<url_info> url_vector_t;

CORE_TOOLS_NS_END

namespace core
{
    class coll_helper;

    namespace archive
    {
        class contact_archive;
        class image_data;
        class message_header;
        class history_message;
        class dlg_state;
        struct dlg_state_changes;
        class archive_hole;
        class not_sent_message;
        class not_sent_messages;

        typedef std::shared_ptr<not_sent_message> not_sent_message_sptr;
        typedef std::shared_ptr<history_message> history_message_sptr;
        typedef std::unordered_map<std::string, std::shared_ptr<contact_archive>> archives_map;
        typedef std::vector<history_message_sptr> history_block;
        typedef std::shared_ptr<history_block> history_block_sptr;
        typedef std::list<image_data> image_list;
        typedef std::list<message_header> headers_list;
        typedef std::list<int64_t> msgids_list;

        struct request_images_handler
        {
            std::function<void(std::shared_ptr<image_list>)> on_result;

            request_images_handler()
            {
                on_result = [](std::shared_ptr<image_list>){};
            }
        };

        struct request_headers_handler
        {
            std::function<void(std::shared_ptr<headers_list>)>	on_result;

            request_headers_handler()
            {
                on_result = [](std::shared_ptr<headers_list>){};
            }
        };

        struct request_buddies_handler
        {
            std::function<void(std::shared_ptr<history_block>)>	on_result;

            request_buddies_handler()
            {
                on_result = [](std::shared_ptr<history_block>){};
            }
        };

        struct request_dlg_state_handler
        {
            std::function<void(const dlg_state& _state)>	on_result;

            request_dlg_state_handler()
            {
                on_result = [](const dlg_state& _state){};
            }
        };

        struct set_dlg_state_handler
        {
            std::function<void(const dlg_state& _state, const dlg_state_changes& _changes)> on_result;

            set_dlg_state_handler()
            {
                on_result = [](const dlg_state&, const dlg_state_changes&){};
            }
        };

        struct request_next_hole_handler
        {
            std::function<void(std::shared_ptr<archive_hole> _hole)>	on_result;

            request_next_hole_handler()
            {
                on_result = [](std::shared_ptr<archive_hole> _hole){};
            }
        };

        struct update_history_handler
        {
            std::function<void(std::shared_ptr<headers_list> _new_ids, const dlg_state &_state, const archive::dlg_state_changes&)> on_result;

            update_history_handler()
            {
                on_result = [](std::shared_ptr<headers_list>, const dlg_state&, const archive::dlg_state_changes&){};
            }
        };

        struct not_sent_messages_handler
        {
            std::function<void(not_sent_message_sptr _message)> on_result;

            not_sent_messages_handler()
            {
                on_result = [](not_sent_message_sptr _message){};
            }
        };

        struct pending_messages_handler
        {
            std::function<void(const std::list<not_sent_message_sptr>& _messages)> on_result;

            pending_messages_handler()
            {
                on_result = [](const std::list<not_sent_message_sptr>&){};
            }
        };

        struct has_not_sent_handler
        {
            std::function<void(bool _has)> on_result;

            has_not_sent_handler()
            {
                on_result = [](bool){};
            }
        };

        struct find_previewable_links_handler
        {
            std::function<void(const tools::url_vector_t &_uris)> on_result_;
        };

        class local_history : public std::enable_shared_from_this<local_history>
        {
            archives_map archives_;
            const std::wstring archive_path_;
            std::unique_ptr<not_sent_messages> not_sent_messages_;

            std::shared_ptr<contact_archive> get_contact_archive(const std::string& _contact);

            not_sent_messages& get_pending_messages();

        public:

            local_history(const std::wstring& _archive_path);
            virtual ~local_history();

            void optimize_contact_archive(const std::string& _contact);

            void get_images(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ image_list& _images);
            bool repair_images(const std::string& _contact);
            void get_messages_index(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ headers_list& _headers);
            void get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids, /*out*/ std::shared_ptr<history_block> _messages);
            bool get_messages(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ std::shared_ptr<history_block> _messages);
            void get_dlg_state(const std::string& _contact, /*out*/dlg_state& _state);
            void set_dlg_state(const std::string& _contact, const dlg_state& _state, Out dlg_state& _result, Out dlg_state_changes& _changes);
            bool clear_dlg_state(const std::string& _contact);
            std::shared_ptr<archive_hole> get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth = -1);
            void update_history(
                const std::string& _contact,
                history_block_sptr _data,
                Out headers_list& _inserted_messages,
                Out dlg_state& _state,
                Out dlg_state_changes& _state_changes
                );

            not_sent_message_sptr get_first_message_to_send();
            not_sent_message_sptr get_not_sent_message_by_iid(const std::string& _iid);
            int32_t insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            int32_t update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            int32_t remove_messages_from_not_sent(const std::string& _contact, std::shared_ptr<archive::history_block> _data);
            void mark_message_duplicated(const std::string _message_internal_id);
            void update_message_post_time(
                const std::string& _message_internal_id, 
                const std::chrono::system_clock::time_point& _time_point);

            bool has_not_sent_messages(const std::string& _contact);
            void get_not_sent_messages(const std::string& _contact, /*out*/ std::shared_ptr<history_block> _messages);
            void get_pending_file_sharing(std::list<not_sent_message_sptr>& _messages);
            not_sent_message_sptr update_pending_with_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id);

            void failed_pending_message(const std::string& _message_internal_id);

            void delete_messages_up_to(const std::string& _contact, const int64_t _id);

            void find_previewable_links(
                const archive::history_block_sptr &_block,
                Out tools::url_vector_t &_uris);

            static void serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll);
            static void serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll);
        };

        class face : public std::enable_shared_from_this<face>
        {
            std::shared_ptr<local_history> history_cache_;
            std::shared_ptr<core::async_executer> thread_;

        public:

            explicit face(const std::wstring& _archive_path);

            std::shared_ptr<update_history_handler> update_history(const std::string& _contact, std::shared_ptr<archive::history_block> _data);
            std::shared_ptr<request_images_handler> get_images(const std::string& _contact, int64_t _from, int64_t _count);
            std::shared_ptr<async_task_handlers> repair_images(const std::string& _contact);
            std::shared_ptr<request_headers_handler> get_messages_index(const std::string& _contact, int64_t _from, int64_t _count);
            std::shared_ptr<request_buddies_handler> get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids);
            std::shared_ptr<request_buddies_handler> get_messages(const std::string& _contact, int64_t _from, int64_t _count);
            std::shared_ptr<request_dlg_state_handler> get_dlg_state(const std::string& _contact);
            std::shared_ptr<set_dlg_state_handler> set_dlg_state(const std::string& _contact, const dlg_state& _state);
            std::shared_ptr<async_task_handlers> clear_dlg_state(const std::string& _contact);
            std::shared_ptr<request_next_hole_handler> get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth = -1);
            std::shared_ptr<async_task_handlers> sync_with_history();

            std::shared_ptr<not_sent_messages_handler> get_pending_message();
            std::shared_ptr<not_sent_messages_handler> get_not_sent_message_by_iid(const std::string& _iid);
            std::shared_ptr<async_task_handlers> insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            std::shared_ptr<async_task_handlers> update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(const std::string& _contact, std::shared_ptr<archive::history_block> _data);
            std::shared_ptr<async_task_handlers> remove_message_from_not_sent(const std::string& _contact, const history_message_sptr _data);
            std::shared_ptr<async_task_handlers> mark_message_duplicated(const std::string& _message_internal_id);

            std::shared_ptr<async_task_handlers> update_message_post_time(
                const std::string& _message_internal_id, 
                const std::chrono::system_clock::time_point& _time_point);

            std::shared_ptr<not_sent_messages_handler> update_pending_messages_by_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id);

            std::shared_ptr<async_task_handlers> failed_pending_message(const std::string& _message_internal_id);

            std::shared_ptr<has_not_sent_handler> has_not_sent_messages(const std::string& _contact);
            std::shared_ptr<request_buddies_handler> get_not_sent_messages(const std::string& _contact);
            std::shared_ptr<pending_messages_handler> get_pending_file_sharing();

            std::shared_ptr<async_task_handlers> delete_messages_up_to(const std::string& _contact, const int64_t _id);

            std::shared_ptr<find_previewable_links_handler> find_previewable_links(const archive::history_block_sptr &_block);

            static void serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll);
            static void serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll);
        };
    }
}