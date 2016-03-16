#ifndef __ROBUSTO_GEN_TOKEN
#define __ROBUSTO_GEN_TOKEN

#pragma once

#include "../robusto_packet.h"

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
		class gen_robusto_token : public robusto_packet
		{
			virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
			virtual int32_t on_response_error_code() override;
			virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

			std::string		token_;

		public:
			
			gen_robusto_token(const wim_packet_params& _params);
			virtual ~gen_robusto_token();

			const std::string get_token() const { return token_; }
		};

	}

}


#endif// __ROBUSTO_GEN_TOKEN