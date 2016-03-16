#ifndef get_themes_index_hpp
#define get_themes_index_hpp

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    namespace tools
    {
        class binary_stream;
    }
    
    namespace wim
    {
        class get_themes_index : public wim_packet
        {
            std::shared_ptr<core::tools::binary_stream>			response_;
            
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            
        public:
            
            virtual int32_t execute() override;
            
            get_themes_index(const wim_packet_params& _params);
            virtual ~get_themes_index();
            
            std::shared_ptr<core::tools::binary_stream> get_response();
            
        };
    }
}

#endif /* get_themes_index_hpp */
