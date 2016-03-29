#pragma once

#include "fetch_event.h"

namespace core
{
	namespace wim
	{
		class fetch_event_typing: public fetch_event
		{
        private:
            bool isTyping_;
            std::string aimId_;
            std::string chatterAimId_;
            std::string chatterName_;
            
		public:
			fetch_event_typing();
			~fetch_event_typing();

            inline bool is_typing() const { return isTyping_; }
            inline const std::string &aim_id() const { return aimId_; }
            inline const std::string &chatter_aim_id() const { return chatterAimId_; }
            inline const std::string &chatter_name() const { return chatterName_; }
            
			virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
			virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
		};

	}
}