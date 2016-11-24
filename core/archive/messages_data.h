#ifndef __MESSAGES_DATA_H_
#define __MESSAGES_DATA_H_

#pragma once

#include "history_message.h"
#include "dlg_state.h"


namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    namespace archive
    {
        class storage;
        class message_header;
        class headers_block;

        typedef std::vector< std::shared_ptr<history_message> >		history_block;
        typedef std::list<message_header>							headers_list;
        typedef std::vector<std::pair<std::string, int64_t>> contact_and_msgs;
        typedef std::vector<std::pair<std::pair<std::string, std::shared_ptr<int64_t>>, std::shared_ptr<int64_t>>> contact_and_offsets;

        struct coded_term
        {
            std::string lower_term;
            std::vector<std::pair<std::string, int32_t>> symb_table;
            std::string symbs;
            std::vector<int32_t> coded_string;
            std::vector<int32_t> prefix;
            std::vector<int32_t> symb_indexes;
        };

        class messages_data
        {
            std::unique_ptr<storage>	storage_;

            history_block get_message_modifications(const message_header& _header) const;

        public:

            messages_data(const std::wstring& _file_name);
            virtual ~messages_data();

            bool update(const history_block& _data);
            bool get_messages(headers_list& _headers, history_block& _messages) const;

            static void search_in_archive(std::shared_ptr<contact_and_offsets> _contacts, std::shared_ptr<coded_term> _cterm
                , std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data
                , std::vector<std::shared_ptr<::core::archive::searched_msg>>& messages_ids
                , int64_t _min_id);
            
            static bool get_history_archive(const std::wstring& _file_name, core::tools::binary_stream& _buffer
                , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode);
        };

    }
}


#endif //__MESSAGES_DATA_H_