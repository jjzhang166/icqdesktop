#pragma once

namespace core
{
    namespace wim
    {
        typedef std::unordered_set<std::string> ignorelist_cache;

        class permit_info
        {
        private:

            ignorelist_cache ignore_aimid_list_;

        public:

            permit_info();
            virtual ~permit_info();

            int32_t parse_response_data(const rapidjson::Value& _node_results);
            const ignorelist_cache& get_ignore_list() const;
        };
    }
}