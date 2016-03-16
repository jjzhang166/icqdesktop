#pragma once

#include "../../../corelib/collection_helper.h"

namespace core
{
	namespace wim
	{
		class search_contacts_response
		{
            struct chunk
            {
                std::string aimid_;
                std::string stamp_;
                std::string type_;
                int32_t score_;
                
                std::string first_name_;
                std::string last_name_;
                std::string nick_name_;
                std::string city_;
                std::string state_;
                std::string country_;
                std::string gender_;
                struct birthdate
                {
                    int32_t year_;
                    int32_t month_;
                    int32_t day_;
                    birthdate(): year_(-1), month_(-1), day_(-1) {}
                }
                birthdate_;
                std::string about_;
                chunk(): score_(-1) {}
            };
            std::vector<chunk> data_;

		public:
			search_contacts_response();
			
			int32_t unserialize(const rapidjson::Value& _node);
			void serialize(core::coll_helper _coll);
		};

	}
}