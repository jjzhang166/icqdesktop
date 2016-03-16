#pragma once

namespace core
{
	struct icollection;

	namespace wim
	{
		class favorite
		{
			std::string		aimid_;
            int64_t        time_;

		public:
			favorite();
			favorite(const std::string& _aimid, const int64_t _time);

			std::string get_aimid() const { return aimid_; }
            int64_t get_time() const { return time_; }
			
			int32_t unserialize(const rapidjson::Value& _node);
			void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
		};

		class favorites
		{
			bool	changed_;

			std::list<favorite>	contacts_;
            std::unordered_map<std::string, int64_t> index_;

		public:
            size_t size() const;

			bool is_changed() { return changed_; }
			void set_changed(bool _changed) { changed_ = _changed; }

			favorites();
			virtual ~favorites();

			void update(favorite& _contact);
			void remove(const std::string& _aimid);
            bool contains(const std::string& _aimid) const;
            int64_t get_time(const std::string& _aimid) const;

			int32_t unserialize(const rapidjson::Value& _node);
			void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
		};

	}
}