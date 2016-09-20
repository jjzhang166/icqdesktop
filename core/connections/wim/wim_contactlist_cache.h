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
            std::string state_;
            std::string usertype_;
            std::string status_msg_;
            std::string other_number_;
            std::set<std::string> capabilities_;
            std::string ab_contact_name_;
            std::string friendly_;
            int32_t lastseen_;
            bool is_chat_;
            bool muted_;
            bool is_live_chat_;
            bool official_;
            std::string icon_id_;
            std::string big_icon_id_;
            std::string large_icon_id_;

            struct search_cache
            {
                std::string aimid_;
                std::string friendly_;
                std::string ab_;

                bool is_empty() const { return aimid_.empty(); }
                void clear() { aimid_.clear(); friendly_.clear(); ab_.clear(); }

            } search_cache_;
            
            cl_presence()
                : is_chat_(false), muted_(false), lastseen_(-1), is_live_chat_(false), official_(false)
            {
            }

            void serialize(icollection* _coll);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void unserialize(const rapidjson::Value& _node);
        };

        struct cl_buddy
        {
            uint32_t id_;
            std::string aimid_;
            std::shared_ptr<cl_presence> presence_;

            cl_buddy() : id_(0), presence_(new cl_presence()) {}
        };


        struct cl_group
        {
            uint32_t id_;
            std::string name_;

            std::list<std::shared_ptr<cl_buddy>> buddies_;

            bool added_;
            bool removed_;


            cl_group() : id_(0), added_(false), removed_(false) {}
        };

        typedef std::unordered_set<std::string> ignorelist_cache;

        class contactlist
        {
            bool changed_;

            std::map<std::string, std::shared_ptr<cl_buddy>> search_cache_;
            std::vector<std::string> last_search_patterns_;

            std::list<std::shared_ptr<cl_group>> groups_;
            std::map< std::string, std::shared_ptr<cl_buddy> > contacts_index_;

            ignorelist_cache ignorelist_;

            contactlist(const contactlist&) {};

        public:



            contactlist() : changed_(false) {}

            void update_cl(const contactlist& _cl);
            void update_ignorelist(const ignorelist_cache& _ignorelist);

            void set_changed(bool _is_changed) {changed_ = _is_changed;}
            bool is_changed() const {return changed_;}
            bool exist(const std::string& contact) { return contacts_index_.find(contact) != contacts_index_.end(); }

            std::string get_contact_friendly_name(const std::string& contact_login);

            int32_t unserialize(const rapidjson::Value& _node);
            int32_t unserialize_from_diff(const rapidjson::Value& _node);

            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll, const std::string& type);
            void serialize_search(icollection* _coll);
            void serialize_ignorelist(icollection* _coll);
            void serialize_contact(const std::string& _aimid, icollection* _coll);
            bool search(std::vector<std::string> search_patterns);
            void update_presence(const std::string& _aimid, std::shared_ptr<cl_presence> _presence);
            void merge_from_diff(const std::string& _type, std::shared_ptr<contactlist> _diff, std::shared_ptr<std::list<std::string>> removedContacts);
            

            int32_t get_contacts_count() const;
            int32_t get_phone_contacts_count() const;
            int32_t get_groupchat_contacts_count() const;

            void add_to_ignorelist(const std::string& _aimid);
            void remove_from_ignorelist(const std::string& _aimid);

            std::shared_ptr<cl_group> get_first_group() const;
            bool is_ignored(const std::string& _aimid);
        };
    }
}



#endif //__WIM_CONTACTLIST_CACHE_H_