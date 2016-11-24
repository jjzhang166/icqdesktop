#pragma once

#include "../../../corelib/collection_helper.h"

namespace core
{
    namespace wim
    {
        class chat_params final
        {
        private:
            boost::optional<std::string> name_;
            boost::optional<std::string> avatar_;
            boost::optional<std::string> about_;
            boost::optional<bool> public_;
            boost::optional<bool> approved_;
            boost::optional<bool> joiningByLink_;
            boost::optional<bool> readOnly_;
            boost::optional<bool> ageGate_;
            
        public:
            chat_params();
            ~chat_params();

            void set_name(const std::string& _name);
            void set_avatar(const std::string& _avatar);
            void set_about(const std::string& _about);
            void set_public(bool _public);
            void set_join(bool _approved);
            void set_joiningByLink(bool _joiningByLink);
            void set_readOnly(bool _readOnly);
            void set_ageGate(bool _ageGate);

            inline const boost::optional<std::string> &get_name() const { return name_; }
            inline const boost::optional<std::string> &get_avatar() const { return avatar_; }
            inline const boost::optional<std::string> &get_about() const { return about_; }
            inline const boost::optional<bool> &get_public() const { return public_; }
            inline const boost::optional<bool> &get_join() const { return approved_; }
            inline const boost::optional<bool> &get_joiningByLink() const { return joiningByLink_; }
            inline const boost::optional<bool> &get_readOnly() const { return readOnly_; }
            inline const boost::optional<bool> &get_ageGate() const { return ageGate_; }
            
            static chat_params *create(core::coll_helper _params);
        };

    }
}
