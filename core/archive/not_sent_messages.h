#pragma once

namespace core
{

    enum class message_type;
    class coll_helper;

    namespace tools
    {
        class tlvpack;
    }

    namespace archive
    {
        class history_message;
        class storage;
        class not_sent_message;

        typedef std::shared_ptr<history_message> history_message_sptr;
        typedef std::vector<history_message_sptr> history_block;
        typedef std::shared_ptr<history_block> history_block_sptr;
        typedef std::shared_ptr<not_sent_message> not_sent_message_sptr;

        class not_sent_message
        {
        public:
            static not_sent_message_sptr make(const core::tools::tlvpack& _pack);

            static not_sent_message_sptr make(const not_sent_message_sptr& _message, const std::string& _wimid, const uint64_t _time);

            static not_sent_message_sptr make(
                const std::string& _aimid,
                const std::string& _message,
                const message_type _type,
                const uint64_t _time,
                const std::string& _internal_id);

            static not_sent_message_sptr make_outgoing_file_sharing(
                const std::string& _aimid,
                const uint64_t _time,
                const std::string& _local_path);

            static not_sent_message_sptr make_incoming_file_sharing(
                const std::string& _aimid,
                const uint64_t _time,
                const std::string& _uri,
                const std::string &_internal_id);

            virtual ~not_sent_message();

            void serialize(core::tools::tlvpack& _pack) const;
            void serialize(core::coll_helper& _coll, const time_t _offset) const;

            const history_message_sptr& get_message() const;

            const std::string& get_aimid() const;

            const std::string& get_internal_id() const;

            const std::string& get_file_sharing_local_path() const;

            core::archive::quotes_vec get_quotes() const;

            void attach_quotes(core::archive::quotes_vec _quotes);

            bool is_ready_to_send() const;

            void mark_duplicated();

            void set_failed();

            void update_post_time(const std::chrono::system_clock::time_point& _time_point);

            const std::chrono::system_clock::time_point& get_post_time() const;

        private:
            std::string aimid_;

            std::shared_ptr<history_message> message_;

            bool duplicated_;

            std::chrono::system_clock::time_point post_time_;

            not_sent_message();

            not_sent_message(
                const not_sent_message_sptr& _message, 
                const std::string& _wimid, 
                const uint64_t _time);

            not_sent_message(
                const std::string& _aimid,
                const std::string& _message,
                const message_type _type,
                const uint64_t _time,
                const std::string& _internal_id);

            void copy_from(const not_sent_message_sptr& _message);

            bool unserialize(const core::tools::tlvpack& _pack);
        };

        typedef std::list<not_sent_message_sptr> not_sent_messages_list;

        class not_sent_messages
        {
            std::map<std::string, not_sent_messages_list> messages_by_aimid_;

            std::unique_ptr<storage> storage_;

            bool is_loaded_;

        public:
            not_sent_messages(const std::wstring& _file_name);
            virtual ~not_sent_messages();

            void load_if_need();
            void remove(const std::string &_aimid, const history_block_sptr &_data);
            void remove(const std::string& _internal_id);
            void insert(const std::string &_aimid, const not_sent_message_sptr &_message);
            void update_if_exist(const std::string &_aimid, const not_sent_message_sptr &_message);
            bool exist(const std::string& _aimid) const;
            not_sent_message_sptr get_first_ready_to_send() const;
            void get_pending_file_sharing_messages(not_sent_messages_list& _messages) const;
            not_sent_message_sptr get_by_internal_id(const std::string& _iid) const;
            void mark_duplicated(const std::string& _message_internal_id);

            void update_message_post_time(
                const std::string& _message_internal_id, 
                const std::chrono::system_clock::time_point& _time_point);

            not_sent_message_sptr update_with_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id);
            void failed_pending_message(const std::string& _message_internal_id);
            void get_messages(const std::string &_aimid, Out history_block &_messages);

            bool save();
            bool load();
        };

    }
}