#ifndef __WIM_CONTACTLIST_CACHE_H_
#define __WIM_CONTACTLIST_CACHE_H_

#pragma once



namespace core
{
    class async_executer;
    struct icollection;

    namespace wim
    {
        class im;

        class contactlist;

        struct cl_presence
        {
            std::string			state_;
            std::string			usertype_;
            std::string			status_msg_;
            std::string			other_number_;
            std::set<std::string>	capabilities_;
            std::string			ab_contact_name_;
            std::string			friendly_;
            int32_t				lastseen_;
            bool				is_chat_;
            bool				muted_;
            bool                official_;

            cl_presence()
                : is_chat_(false), muted_(false), official_(false), lastseen_(-1)
            {
            }

            void serialize(icollection* _coll);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void unserialize(const rapidjson::Value& _node);
        };

        struct cl_buddy
        {
            uint32_t						id_;
            std::string						aimid_;
            std::shared_ptr<cl_presence>	presence_;

            cl_buddy() : id_(0), presence_(new cl_presence()) {}
        };


        struct cl_group
        {
            uint32_t								id_;
            std::string								name_;

            std::list<std::shared_ptr<cl_buddy>>	buddies_;

            bool									added_;
            bool									removed_;


            cl_group() : id_(0), added_(false), removed_(false) {}
        };

        class contactlist
        {
            bool	changed_;

            std::map< std::string, std::shared_ptr<cl_buddy> >	search_cache_;
            std::vector<std::string> last_search_patterns_;

        public:

            std::list<std::shared_ptr<cl_group>>	groups_;
            std::map< std::string, std::shared_ptr<cl_buddy> >	contacts_index_;

            contactlist() : changed_(false) {}

            void set_changed(bool _is_changed) {changed_ = _is_changed;}
            bool is_changed() const {return changed_;}
            bool exist(const std::string& contact) { return contacts_index_.find(contact) != contacts_index_.end(); }

            std::string get_contact_friendly_name(const std::string& contact_login);

            int32_t unserialize(const rapidjson::Value& _node);
            int32_t unserialize_from_diff(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll, const std::string& type);
            void serialize_search(icollection* _coll);
            bool search(std::vector<std::string> search_patterns);
            void update_presence(const std::string& _aimid, std::shared_ptr<cl_presence> _presence);
            void merge_from_diff(const std::string& _type, std::shared_ptr<contactlist> _diff, std::shared_ptr<std::list<std::string>> removedContacts);

            int GetPhoneContactCounts() const;
            int GetGroupChatContactCounts() const;
        };
    }
}



#endif //__WIM_CONTACTLIST_CACHE_H_