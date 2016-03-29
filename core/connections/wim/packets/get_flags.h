#ifndef __WIM_GET_FLAGS_H_
#define __WIM_GET_FLAGS_H_

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
		class get_flags : public wim_packet
		{
			virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
			virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            int flags_;

		public:

			get_flags(const wim_packet_params& _params);

            int flags() const;

			virtual ~get_flags();
		};

	}

}


#endif// __WIM_GET_FLAGS_H_
