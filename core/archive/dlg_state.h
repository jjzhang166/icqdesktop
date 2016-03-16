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
			uint32_t							unread_count_;
			int64_t								last_msgid_;
			int64_t								yours_last_read_;
			int64_t								theirs_last_read_;
			int64_t								theirs_last_delivered_;
			bool								visible_;
			std::string							last_message_friendly_;
			std::unique_ptr<history_message>	last_message_;


		public:

			dlg_state();
			dlg_state(const dlg_state& _state);
			virtual ~dlg_state();

			dlg_state& operator=(const dlg_state& _state);

			void copy_from(const dlg_state& _state);

			void set_unread_count(uint32_t _unread_count) { unread_count_ = _unread_count; }
			uint32_t get_unread_count() const { return unread_count_; }
			
			void set_last_msgid(int64_t _last_msgid) { last_msgid_ = _last_msgid; }
			int64_t get_last_msgid() const { return last_msgid_; }

			void set_yours_last_read(int64_t _val) { yours_last_read_ = _val; }
			int64_t get_yours_last_read() const { return yours_last_read_; }

			void set_theirs_last_read(int64_t _val) { theirs_last_read_ = _val; }
			int64_t get_theirs_last_read() const { return theirs_last_read_; }

			void set_theirs_last_delivered(int64_t _val) { theirs_last_delivered_ = _val; }
			int64_t	get_theirs_last_delivered() const { return theirs_last_delivered_; }

			void set_visible(bool _val) { visible_ = _val; } 
			bool get_visible() const { return visible_; }
						
			const history_message& get_last_message() const;
			void set_last_message(const history_message& _message);

			const std::string& get_last_message_friendly() const;
			void set_last_message_friendly(const std::string& _friendly);

			void serialize(icollection* _collection, const time_t _offset, const time_t _last_successful_fetch, bool _serialize_message = true) const;
			void serialize(core::tools::binary_stream& _data) const;
			
			bool unserialize(core::tools::binary_stream& _data);
		};
		//////////////////////////////////////////////////////////////////////////



		class archive_state
		{
			std::unique_ptr<dlg_state>	state_;
			std::unique_ptr<storage>	storage_;

		public:

			archive_state(const std::wstring& _file_name);
			~archive_state();

			void merge_state(const dlg_state& _state);

			bool save();
			bool load();

			const dlg_state& get_state();
			void set_state(const dlg_state& _state);
            void clear_state();
		};
	}
}


#endif //__DLG_STATE_H_