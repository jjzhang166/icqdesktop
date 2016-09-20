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
			const QString &contactName,
			const bool haveLastSeen,
			const QDateTime &lastSeen,
			const int unreadsCounter,
			const bool muted,
            const QString &senderNick,
            const bool isOfficial,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const bool isTyping,
            const DeliveryState deliveryState);

		const DeliveryState DeliveryState_;

		const bool Muted_;

        const bool IsTyping_;

        QString senderNick_;
	};

	void RenderRecentsItem(QPainter &painter, const RecentItemVisualData &item, const ViewParams& viewParams_);

    void RenderRecentsDragOverlay(QPainter &painter, const ViewParams& viewParams_);

    void RenderUnknownsHeader(QPainter &painter, const QString& title, const int count, const ViewParams& viewParams_);

    void RenderServiceItem(QPainter &painter, const QString& text, bool renderState, bool drawLine, const ViewParams& viewParams_);

    void RenderDeleteAllItem(QPainter &painter, const QString& title, bool isMouseOver, const ViewParams& viewParams_);

	int RenderContactMessage(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx, const ViewParams& viewParams_, ContactListParams& _recentParams);

	int RenderNotifications(QPainter &painter, const int unreads, bool muted, const ViewParams& viewParams_, ContactListParams& _recentParams, bool _isUnknownHeader);

    void RenderLastReadAvatar(QPainter &painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams);

    int RenderAddContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams);

    int RenderRemoveContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams);

    QRect AddContactFrame();

    QRect RemoveContactFrame();

    QRect DeleteAllFrame();
}
