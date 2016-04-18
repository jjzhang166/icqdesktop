#pragma once

#include "fetch_event.h"
#include "../../../archive/dlg_state.h"
#include "../wim_history.h"

namespace core
{
    namespace wim
    {
        class fetch_event_dlg_state : public fetch_event
        {
            std::string aimid_;
            archive::dlg_state state_;
            std::shared_ptr<archive::history_block> messages_;

        public:
            fetch_event_dlg_state();
            virtual ~fetch_event_dlg_state();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_aim_id() const { return aimid_; }
            const archive::dlg_state& get_dlg_state() { return state_; }
            std::shared_ptr<archive::history_block> get_messages() { return messages_; }
        };
    }
}