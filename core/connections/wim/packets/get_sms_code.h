#ifndef __GET_SMS_CODE_H_
#define __GET_SMS_CODE_H_

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
	namespace wim
	{
		class validate_phone : public wim_packet
		{
			std::string		phone_;
			std::string		trans_id_;
            std::string     locale_;
			bool			existing_;

			virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
			virtual int32_t on_response_error_code() override;
			virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

		public:

			validate_phone(
				const wim_packet_params& params,
				const std::string& phone,
                const std::string& locale);

			virtual ~validate_phone();

			const std::string get_phone() const {return phone_;}
			const bool get_existing() const {return existing_;}
			const std::string get_trans_id() const {return trans_id_;}
		};
	}
}


#endif //__GET_SMS_CODE_H_