#pragma once

#include "fetch_event.h"

namespace core
{
	namespace wim
	{
		class contactlist;

        typedef std::map<std::string, std::shared_ptr<contactlist>> diffs_map;

		class fetch_event_diff : public fetch_event
		{
			std::shared_ptr<diffs_map>	diff_;

		public:
						
			fetch_event_diff();
			virtual ~fetch_event_diff();

			std::shared_ptr<diffs_map> get_diff() { return diff_; }

			virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
			virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
		};

	}
}