#pragma once

class QPixmap;

namespace core
{
	class coll_helper;
}

namespace Data
{
	class ChatMemberInfo
	{
	public:
		ChatMemberInfo()
			: Friend_(false)
			, NoAvatar_(false)
		{
		}

		QString AimdId_;
		QString Role_;
		QString FirstName_;
		QString LastName_;
		QString NickName_;

		bool Friend_;
		bool NoAvatar_;
	};

	class ChatInfo
	{
	public:
		ChatInfo()
		{
		}

		QString AimId_;
		QString Name_;
		QString About_;
		QString YourRole_;
		QString Owner_;
		QString MembersVersion_;
		QString InfoVersion_;

		qint32 CreateTime_;
		qint32 MembersCount_;
		qint32 FriendsCount;
		qint32 BlockedCount_;

		bool YouBlocked_;
		bool Public_;
		bool Live_;
		bool Controlled_;

		QList<ChatMemberInfo> Members_;
	};

    void UnserializeChatInfo(core::coll_helper* helper, ChatInfo& info);
}

Q_DECLARE_METATYPE(Data::ChatMemberInfo*);