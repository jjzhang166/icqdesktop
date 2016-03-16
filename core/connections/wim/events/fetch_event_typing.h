#pragma once

#include "fetch_event.h"

namespace core
{
	namespace wim
	{
		class fetch_event_typing: public fetch_event
		{
        private:
            std::string aimId_;
            std::vector< std::string > chattersAimIds_;
            
		public:
			fetch_event_typing();
			~fetch_event_typing();

            inline const std::string &aim_id() const { return aimId_; }
            inline const std::vector< std::string > &chatters_aim_ids() const { return chattersAimIds_; }
            
			virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
			virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
		};

	}
}