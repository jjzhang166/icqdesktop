#ifndef __CONTACT_ARCHIVE_H_
#define __CONTACT_ARCHIVE_H_

#pragma once

namespace core
{
	namespace archive
	{
		class history_message;
		class archive_index;
		class messages_data;
		class message_header;
		class dlg_state;
		class archive_hole;
		class archive_state;

		typedef std::vector<std::shared_ptr<history_message>>		history_block;
		typedef std::list<int64_t>									msgids_list;
		typedef std::list<message_header>							headers_list;

		class contact_archive
		{
			const std::wstring					path_;

			std::unique_ptr<archive_index>		index_;
			std::unique_ptr<messages_data>		data_;
			std::unique_ptr<archive_state>		state_;

			bool								local_loaded_;

		public:

			void get_messages_index(int64_t _from, int64_t _count, headers_list& _headers) const;
			bool get_messages_buddies(std::shared_ptr<archive::msgids_list> _ids, std::shared_ptr<history_block> _messages) const;

			bool get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth);

			const dlg_state& get_dlg_state() const;
			void set_dlg_state(const dlg_state& _state);
            void clear_dlg_state();

			int insert_history_block(std::shared_ptr<archive::history_block> _data, /*out*/ headers_list& _inserted_messages);
			int load_from_local();

			bool need_optimize();
			void optimize();

			contact_archive(const std::wstring& _archive_path);
			virtual ~contact_archive();

		};

		std::wstring version_db_filename(const std::wstring &filename);

	}
}

#endif //__CONTACT_ARCHIVE_H_