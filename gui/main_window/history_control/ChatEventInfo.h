#pragma once

namespace core
{
	class coll_helper;

	struct iarray;

	enum class chat_event_type;
}

namespace HistoryControl
{

	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

	class ChatEventInfo
	{
	public:
		static ChatEventInfoSptr Make(
			const core::coll_helper &info, 
			const bool isOutgoing,
			const QString &myAimid);

		const QString& formatEventText() const;

		QImage loadEventIcon(const int32_t sizePx) const;

	private:
		const core::chat_event_type Type_;

		const bool IsOutgoing_;

		const QString MyAimid_;

		mutable QString FormattedEventText_;

		QString SenderFriendly_;

		struct
		{
			QStringList MembersFriendly_;
		} Mchat_;

		struct
		{
			QString NewName_;
		} Chat_;

		ChatEventInfo(const core::chat_event_type type, const bool isOutgoing, const QString &myAimid);

		QString formatEventTextInternal() const;

		QString formatAddedToBuddyListText() const;

		QString formatBirthdayText() const;

		QString formatBuddyFound() const;

		QString formatBuddyReg() const;

		QString formatChatNameModifiedText() const;

		QString formatMchatAddMembersText() const;

		QString formatMchatDelMembersText() const;

		QString formatMchatInviteText() const;

		QString formatMchatKickedText() const;

		QString formatMchatLeaveText() const;

		QString formatMchatMembersList(const bool activeVoice) const;

		bool isMyAimid(const QString &aimId) const;

		bool hasMultipleMembers() const;

		void setNewName(const QString &newName);

		void setSenderFriendly(const QString &friendly);

		void setMchatMembers(const core::iarray &members);

	};

}