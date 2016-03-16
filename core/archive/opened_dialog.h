#pragma once

namespace core
{
	namespace archive
	{
		class opened_dialog
		{
		public:
			opened_dialog();
			void set_first_msg_id(int64_t _id);
			int64_t get_first_msg_id() const;

		private:
			int64_t first_msg_id_;
		};
	}
}