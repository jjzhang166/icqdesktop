#ifndef _WEB_FILE_INFO_H_
#define _WEB_FILE_INFO_H_

#pragma once

namespace core
{
	namespace tools
	{
		class binary_stream;
	}

	struct icollection;

	namespace wim
	{
		class web_file_info
		{
			std::wstring	file_name_;
			int64_t			file_size_;
			std::string		file_name_short_;
			std::string		file_url_;
			bool			is_previewable_;
            bool            played_;
			std::string		file_dlink_;
			std::string		mime_;
			std::string		md5_;
			std::string		file_preview_;
			std::string		file_preview_2k_;
			std::string		file_id_;

			int64_t			bytes_transfer_;

		public:

			web_file_info();

			void set_file_size(int64_t _size);
			int64_t get_file_size() const;

			void set_file_name(const std::wstring& _name);
			std::wstring get_file_name() const;

			void set_bytes_transfer(int64_t _bytes);
			int64_t get_bytes_transfer() const;

			void set_file_name_short(const std::string& _file_name_short);
			std::string get_file_name_short() const;

			void set_file_url(const std::string& _url);
			std::string get_file_url() const;

			bool is_previewable() const;
			void set_is_previewable(bool _is_previewable);

            bool is_played() const;
            void set_played(bool _played);

			void set_file_dlink(const std::string& _dlink);
			std::string get_file_dlink() const;

			void set_mime(const std::string& _mime);
			std::string get_mime() const;

			void set_md5(const std::string& _md5);
			std::string get_md5() const;

			void set_file_preview(const std::string& _val);
			const std::string& get_file_preview() const;

			void set_file_preview_2k(const std::string& _val);
			std::string get_file_preview_2k() const;

			std::string get_file_id() const;
			void set_file_id(const std::string& _file_id);

			void serialize(icollection* _collection) const;
			void serialize(core::tools::binary_stream& _bs) const;
			bool unserialize(const core::tools::binary_stream& _bs);
		};
	}
};



#endif //_WEB_FILE_INFO_H_
