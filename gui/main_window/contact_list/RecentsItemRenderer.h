#pragma once

#include "Common.h"

namespace ContactList
{

	enum class DeliveryState
	{
		Invalid,
		Min,

		NotDelivered = Min,
		Sending,
		DeliveredToServer,
		DeliveredToClient,

		Max = DeliveredToClient
	};

	struct RecentItemVisualData : VisualDataBase
	{
		RecentItemVisualData(
			const QString &aimId,
			const QPixmap &avatar,
			const QString &state,
			const QString &status,
			const bool isHovered,
			const bool isSelected,
            const bool isTyping,
			const QString &contactName,
			const DeliveryState deliveryState,
			const bool haveLastSeen,
			const QDateTime &lastSeen,
			const int unreadsCounter,
			const bool muted,
            const QString &senderNick);

		const DeliveryState DeliveryState_;

		const int UnreadsCounter_;

		const bool Muted_;
        
        const bool IsTyping_;

		bool HasAvatar() const;
        
        QString senderNick_;
	};

	void RenderRecentsItem(QPainter &painter, const RecentItemVisualData &item, bool fromAlert);

    void RenderRecentsDragOverlay(QPainter &painter);

    void RenderServiceItem(QPainter &painter, const QString& text, bool renderState);
}