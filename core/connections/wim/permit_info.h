#pragma once

namespace core
{
    namespace wim
    {
        class permit_info
        {
        private:

            std::vector<std::string> ignore_aimid_list_;

        public:

            permit_info();
            virtual ~permit_info();

            int32_t parse_response_data(const rapidjson::Value& _node_results);
            std::vector<std::string> get_ignore_list() const;
        };
    }
}