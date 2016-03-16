#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../archive/storage.h"

namespace core
{
	namespace wim
	{
		class my_info
		{
			std::string aimId_;
			std::string displayId_;
			std::string friendlyName_;
			std::string state_;
			std::string userType_;
			std::string phoneNumber_;
			uint32_t	flags_;

		public:
			my_info();

			int32_t unserialize(const rapidjson::Value& _node);
			void serialize(core::coll_helper _coll);

			void serialize(core::tools::binary_stream& _data) const;
			bool unserialize(core::tools::binary_stream& _data);
		};

		class my_info_cache
		{
			bool changed_;

			std::shared_ptr<my_info> info_;

		public:
			my_info_cache();

            bool is_changed() const;

			std::shared_ptr<my_info> get_info() const;
			void set_info(std::shared_ptr<my_info> _info);

			int save(const std::wstring& _filename);
			int load(const std::wstring& _filename);
		};
	}
}