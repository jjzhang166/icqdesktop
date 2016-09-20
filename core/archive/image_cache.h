#ifndef __IMAGE_CACHE_H_
#define __IMAGE_CACHE_H_

namespace core
{
    namespace archive
    {
        class archive_index;
        class contact_archive;
        class history_message;
        class storage;

        typedef std::vector<std::shared_ptr<history_message>> history_block;

        class image_data
        {
            int64_t msgid_;
            std::string url_;
            bool is_filesharing_;

        public:
            image_data();
            explicit image_data(int64_t _msgid);
            image_data(int64_t _msgid, std::string _url, bool _is_filesharing);

            int64_t get_msgid() const;
            void set_msgid(int64_t _value);

            std::string get_url() const;
            void set_url(const std::string& _value);

            bool get_is_filesharing() const;
            void set_is_filesharing(bool _value);

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);
        };

        typedef std::list<image_data> image_list;

        class image_cache
        {
            std::unique_ptr<storage> storage_;
            std::unique_ptr<storage> tmp_to_add_storage_;
            std::unique_ptr<storage> tmp_to_delete_storage_;
            typedef std::multimap<int64_t, image_data> images_map_t;
            images_map_t image_by_msgid_; // need to keep the order of messages

            typedef std::vector<image_data> image_vector_t;

            mutable std::mutex mutex_;
            mutable std::condition_variable data_ready_;
            bool building_in_progress_;

            bool tree_is_consistent_;

            std::atomic_flag cancel_build_;

        public:
            explicit image_cache(const std::wstring& _file_name);
            virtual ~image_cache();

            bool load_from_local(const contact_archive& _archive);

            void get_images(int64_t _from, int64_t _count, image_list& _images) const;

            bool update(const history_block& _block);

            bool synchronize(const archive_index& _index);

            bool build(const contact_archive& _archive);
            void cancel_build();

        private:
            bool serialize_block(storage& _storage, const image_vector_t& _images) const;
            bool unserialize_block(core::tools::binary_stream& _stream, images_map_t& _images) const;

            bool save_all() const;

            void extract_images(const history_message& _message, image_vector_t& _images) const;
            image_vector_t extract_images(const history_block& _block) const;
            void extract_images(int64_t _msgid, const std::string& _text, image_vector_t& _images) const;

            void add_images_to_tree(const image_vector_t& _images);
            void add_images_to_tree(const images_map_t& _images);

            void erase_deleted_from_tree(const archive_index& _index);

            bool read_file(storage& _storage, images_map_t& _images) const;
            bool append_to_file(storage& _storage, const image_vector_t& _images) const;

            bool update_tree_from_tmp_files();
            bool process_deleted(bool& _need_to_save_all);
            bool update_file_from_tmp_files();
            void delete_tmp_files();
        };
    }
}

#endif//__IMAGE_CACHE_H_
