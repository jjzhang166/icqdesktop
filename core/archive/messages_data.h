#ifndef __MESSAGES_DATA_H_
#define __MESSAGES_DATA_H_

#pragma once

#include "history_message.h"


namespace core
{
    namespace archive
    {
        class storage;
        class message_header;
        class headers_block;

        typedef std::vector< std::shared_ptr<history_message> >		history_block;
        typedef std::list<message_header>							headers_list;

        class messages_data
        {
            std::unique_ptr<storage>	storage_;

            history_block get_message_modifications(const message_header& _header) const;

        public:

            messages_data(const std::wstring& _file_name);
            virtual ~messages_data();

            bool update(const history_block& _data);
            bool get_messages(headers_list& _headers, history_block& _messages) const;
        };

    }
}


#endif //__MESSAGES_DATA_H_