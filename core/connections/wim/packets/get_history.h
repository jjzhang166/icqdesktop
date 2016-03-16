#ifndef __ROBUSTO_GET_HISTORY
#define __ROBUSTO_GET_HISTORY

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
	namespace archive
	{
		class history_message;
		class dlg_state;

		typedef std::vector< std::shared_ptr<history_message> > history_block;
	}

	namespace wim
	{
		struct get_history_params 
		{
			std::string		aimid_;
			int64_t			till_msg_id_;
			int64_t			from_msg_id_;
			int32_t			count_;

			get_history_params() : 
				from_msg_id_(-1),
				till_msg_id_(-1),
				count_(0)
			{
			}
		};


		class get_history : public robusto_packet
		{
			int64_t		older_msgid_;

			get_history_params		hist_params_;

			virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
			virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
	
			std::shared_ptr<archive::history_block>		messages_;
			std::shared_ptr<archive::dlg_state>			dlg_state_;

		public:
			
			get_history(
				const wim_packet_params& _params, 
				const get_history_params& _hist_params);

			virtual ~get_history();

			std::shared_ptr<archive::history_block> get_messages() { return messages_; }
			std::shared_ptr<archive::dlg_state> get_dlg_state() { return dlg_state_; }
			const get_history_params& get_hist_params() const { return hist_params_; }
			int64_t get_older_msgid() const { return older_msgid_; }
		};

	}

}


#endif// __ROBUSTO_GET_HISTORY