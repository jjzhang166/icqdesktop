#ifndef __ROBUSTO_SET_DLG_STATE
#define __ROBUSTO_SET_DLG_STATE

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
		struct set_dlg_state_params 
		{
			std::string		aimid_;
			std::string		last_delivered_wim_;
			std::string		last_read_wim_;
			bool			invisible_;
			int64_t			last_delivered_;
			int64_t			last_read_;

			set_dlg_state_params() : 
				last_delivered_(-1),
				last_read_(-1),
				invisible_(false)
			{
			}
		};


		class set_dlg_state: public robusto_packet
		{
			set_dlg_state_params		params_;

			virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

		public:
			
			set_dlg_state(
				const wim_packet_params& _params, 
				const set_dlg_state_params& _dlg_params);

			virtual ~set_dlg_state();
		};

	}

}


#endif// __ROBUSTO_SET_DLG_STATE