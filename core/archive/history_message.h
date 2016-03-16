#pragma once

#include "message_flags.h"

#include "../../corelib/iserializable.h"

namespace core
{
	struct icollection;
	class coll_helper;

	enum class message_type;
	enum class chat_event_type;
    enum class voip_event_type;

	namespace archive
	{
		typedef core::Str2StrMap persons_map;

		//////////////////////////////////////////////////////////////////////////
		// message_header class
		//////////////////////////////////////////////////////////////////////////


		// header store format
		// version		- uint8_t
		// flags		- uint32_t
		// time			- uint64_t
		// msgid		- int64_t
		// data_offset	- int64_t
		// data_size	- uint32_t

		class message_header
		{
			int64_t			id_;
			uint8_t			version_;
			message_flags	flags_;
			uint64_t		time_;
			int64_t			prev_id_;
			int64_t			data_offset_;
			uint32_t		data_size_;

			uint32_t data_sizeof() const;

		public:

			message_header();
			message_header(
				message_flags _flags,
				uint64_t _time,
				int64_t _id,
				int64_t _prev_id,
				int64_t _data_offset,
				uint32_t _data_size);

			uint64_t get_time() const { return time_; }
			void set_time(uint64_t _value) { time_ = _value; }

			int64_t get_id() const { return id_; }
			void set_id(int64_t _id) { id_ = _id; }

			message_flags get_flags() const { return flags_; }
			void set_flags(message_flags _flags) { flags_ = _flags; }

			int64_t get_data_offset() const { return data_offset_; }
			void set_data_offset(int64_t _value) { data_offset_ = _value; }

			uint32_t get_data_size() const { return data_size_; }
			void set_data_size(uint32_t _value) { data_size_ = _value; }

			int64_t get_prev_msgid() const { return prev_id_; }
			void set_prev_msgid(int64_t _value) { prev_id_ = _value; }

			void serialize(core::tools::binary_stream& _data) const;
			bool unserialize(core::tools::binary_stream& _data);

			friend bool operator<(const message_header& _header1, const message_header& _header2) throw()
			{
				return(_header1.get_id() < _header2.get_id());
			}

			friend bool operator<(const message_header& _header1, const int64_t& _id) throw()
			{
				return(_header1.get_id() < _id);
			}
		};
		//////////////////////////////////////////////////////////////////////////



		class sticker_data
		{
			std::string		id_;

		public:
			sticker_data();

			sticker_data(const std::string& _id);

			const std::string& get_id() const { return id_; }

			void serialize(icollection* _collection);
			void serialize(core::tools::tlvpack& _pack);
			int32_t unserialize(const rapidjson::Value& _node);
			int32_t unserialize(core::tools::tlvpack& _pack);
		};

		class mult_data
		{
		public:
			void serialize(icollection* _collection) {}
			void serialize(core::tools::tlvpack& _pack) {}
			int32_t unserialize(const rapidjson::Value& _node) { return 0; }
			int32_t unserialize(core::tools::tlvpack& _pack) { return 0; }
		};

		class voip_data
            : public core::iserializable
            , public core::tools::iserializable_tlv
		{
		public:
            voip_data();

            void apply_persons(const archive::persons_map &_persons);

            bool unserialize(const rapidjson::Value &_node, const std::string &_sender_aimid);

            virtual void serialize(Out coll_helper &_coll) const override;
            virtual bool unserialize(const coll_helper &_coll) override;

            virtual void serialize(Out core::tools::tlvpack &_pack) const override;
			virtual bool unserialize(const core::tools::tlvpack &_pack) override;

        private:
            voip_event_type type_;

            std::string sender_aimid_;

            std::string sender_friendly_;

            int32_t duration_sec_;

            int32_t is_incoming_;

            virtual void unserialize_duration(const rapidjson::Value &_node);

		};

        typedef std::unique_ptr<voip_data> voip_data_uptr;

		class chat_data
		{
			std::string		sender_;
			std::string		friendly_;
			std::string		name_;

		public:

			void apply_persons(const archive::persons_map &_persons);

			const std::string& get_sender() const { return sender_; }
			const std::string& get_name() const { return name_; }

			const std::string& get_friendly() const { return friendly_; }
			void set_friendly(const std::string& _friendly) { friendly_ = _friendly; }

			void serialize(core::tools::tlvpack& _pack);
			void serialize(icollection* _collection);
			int32_t unserialize(const rapidjson::Value& _node);
			int32_t unserialize(core::tools::tlvpack& _pack);
		};

		typedef std::unique_ptr<class file_sharing_data> file_sharing_data_uptr;

		class file_sharing_data
		{
		public:
			file_sharing_data(const std::string &_local_path, const std::string &_uri);

			file_sharing_data(icollection* _collection);

			file_sharing_data(const core::tools::tlvpack &_pack);

			void serialize(Out icollection* _collection, const std::string &_internal_id, const bool _is_outgoing) const;

			void serialize(Out core::tools::tlvpack& _pack) const;

			const std::string& get_local_path() const;

			std::string to_log_string() const;

		private:
			std::string uri_;

			std::string local_path_;

		};

		typedef std::unique_ptr<class chat_event_data> chat_event_data_uptr;

		class chat_event_data
		{
		public:
			static chat_event_data_uptr make_added_to_buddy_list(const std::string &_sender_aimid);

			static chat_event_data_uptr make_mchat_event(const rapidjson::Value& _node);

			static chat_event_data_uptr make_modified_event(const rapidjson::Value& _node);

			static chat_event_data_uptr make_from_tlv(const tools::tlvpack& _pack);

			static chat_event_data_uptr make_simple_event(const chat_event_type _type);

			void apply_persons(const archive::persons_map &_persons);

			void serialize(Out icollection* _collection, const bool _is_outgoing) const;

			void serialize(Out tools::tlvpack& _pack) const;

		private:
			chat_event_data(const chat_event_type _type);

			chat_event_data(const tools::tlvpack &_pack);

			void deserialize_chat_modifications(const tools::tlvpack &_pack);

			void deserialize_mchat_members(const tools::tlvpack &_pack);

			bool has_mchat_members() const;

			bool has_chat_modifications() const;

			bool has_sender_aimid() const;

			void serialize_chat_modifications(Out coll_helper &_coll) const;

			void serialize_chat_modifications(Out tools::tlvpack &_pack) const;

			void serialize_mchat_members(Out coll_helper &_coll) const;

			void serialize_mchat_members(Out tools::tlvpack &_pack) const;

			chat_event_type type_;

			std::string sender_aimid_;

			std::string sender_friendly_;

			struct
			{
				StrSet members_;
				StrSet members_friendly_;
			} mchat_;

			struct
			{
				std::string new_name_;
			} chat_;

		};

		class history_message
		{
			int64_t			msgid_;
			int64_t			prev_msg_id_;
			message_flags	flags_;
			time_t			time_;
			std::string		text_;
			int64_t			data_offset_;
			uint32_t		data_size_;
			std::string		wimid_;
			std::string		internal_id_;
			std::string		sender_friendly_;

			std::unique_ptr<sticker_data>		sticker_;
			std::unique_ptr<mult_data>			mult_;
			voip_data_uptr			            voip_;
			std::unique_ptr<chat_data>			chat_;
			file_sharing_data_uptr				file_sharing_;
			chat_event_data_uptr				chat_event_;

			void copy(const history_message& _message);

			void init_default();

		public:

			history_message();
			history_message(const history_message& _message);
			history_message& operator=(const history_message& _message);
			virtual ~history_message();

			void serialize(icollection* _collection, const time_t _offset, bool _serialize_message = true) const;
			void serialize(core::tools::binary_stream& _data) const;
			int32_t unserialize(const rapidjson::Value& _node,
								const std::string &_sender_aimid);
			int32_t unserialize(core::tools::binary_stream& _data);

			const int64_t get_msgid() const { return msgid_; }
			message_flags get_flags() const;
			time_t get_time() const { return time_; }

			int64_t get_data_offset() const { return data_offset_; }
			void set_data_offset(int64_t _value) { data_offset_ = _value; }

			uint32_t get_data_size() const { return data_size_; }
			void set_data_size(uint32_t _value) { data_size_ = _value; }

			int64_t get_prev_msgid() const { return prev_msg_id_; }
			void set_prev_msgid(int64_t _value) { prev_msg_id_ = _value; }

			std::string get_text() const;
			void set_text(const std::string& _text);

			void set_wimid(const std::string& _wimid) { wimid_ = _wimid; }
			std::string get_wimid() const { return wimid_; }

			archive::chat_data* get_chat_data();
			const archive::chat_data* get_chat_data() const;

			void set_internal_id(const std::string& _internal_id) { internal_id_ = _internal_id; }
			const std::string& get_internal_id() const { return internal_id_; }

			void set_sender_friendly(const std::string& _friendly) { sender_friendly_ = _friendly; }
			const std::string& get_sender_friendly() const { return sender_friendly_; }

			void set_time(uint64_t time) { time_ = time; }

			void set_outgoing(bool _outgoing);

			bool is_sms() const { return false; }
			bool is_sticker() const { return (bool)sticker_; }
			bool is_file_sharing() const { return (bool)file_sharing_; }
			bool is_chat_event() const { return (bool)chat_event_; }

			void init_file_sharing_from_local_path(const std::string &_local_path);
			void init_file_sharing_from_link(const std::string &_uri);
			void init_sticker_from_text(const std::string &_text);
			const file_sharing_data_uptr& get_file_sharing_data() const;

            static bool is_file_sharing_uri(const std::string &_uri);
            static bool is_image(const std::string& _id);
            static std::string extract_new_file_sharing_file_id(const std::string &uri);

			chat_event_data_uptr& get_chat_event_data();
            voip_data_uptr& get_voip_data();

			message_type get_type() const;
		};

		typedef std::shared_ptr<history_message> history_message_sptr;
	}
}