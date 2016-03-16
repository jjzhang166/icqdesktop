#ifndef __WIM_HISTORY_H_
#define __WIM_HISTORY_H_

#pragma once


namespace core
{
	namespace archive
	{
		class history_message;
		typedef std::vector<std::shared_ptr<history_message>> history_block;
		typedef core::Str2StrMap persons_map;
	}

	namespace wim
	{
		bool parse_history_messages_json(
			const rapidjson::Value &_node,
			const int64_t _older_msg_id,
			const std::string &_sender_aimid,
			Out archive::history_block &_block);
	}
}


#endif //__WIM_HISTORY_H_