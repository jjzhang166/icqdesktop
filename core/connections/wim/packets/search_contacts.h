#ifndef __WIM_SEARCH_CONTACTS_H_
#define __WIM_SEARCH_CONTACTS_H_

#pragma once

#include "../wim_packet.h"
#include "../../search_contacts_params.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace profile
    {
        class info;
        typedef std::vector<std::shared_ptr<info>> profiles_list;
    }

    namespace wim
    {
        class search_contacts : public wim_packet
        {
            enum search_type
            {
                unknown,
                presence_get,
                memberdir_get,
                memberdir_search
            };

            search_type					search_type_;

            const core::search_params	filters_;
            profile::profiles_list		search_result_;


            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;


        public:

            search_contacts(
                const wim_packet_params& _params,
                const core::search_params& _filters);

            virtual ~search_contacts();

            const profile::profiles_list& get_result() const;
        };

    }

}


#endif// __WIM_SEARCH_CONTACTS_H_