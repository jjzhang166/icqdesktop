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

        const QString getFriendly() const;

		bool Friend_;
		bool NoAvatar_;
	};

	class ChatInfo
	{
	public:
		ChatInfo();

		QString AimId_;
		QString Name_;
        QString Location_;
		QString About_;
		QString YourRole_;
		QString Owner_;
		QString MembersVersion_;
		QString InfoVersion_;
        QString Stamp_;

		int32_t CreateTime_;
		int32_t MembersCount_;
		int32_t FriendsCount;
		int32_t BlockedCount_;

		bool YouBlocked_;
		bool Public_;
		bool Live_;
		bool Controlled_;

		QList<ChatMemberInfo> Members_;
	};

    void UnserializeChatInfo(core::coll_helper* helper, ChatInfo& info);
}

Q_DECLARE_METATYPE(Data::ChatMemberInfo*);