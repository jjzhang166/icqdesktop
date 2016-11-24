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
            const DeliveryState deliveryState,
            const QString term = "",
            const bool haveLastMsg = true,
            const qint64 _msg_id = -1);

		const DeliveryState DeliveryState_;

		const bool Muted_;

        const bool IsTyping_;

        QString senderNick_;

        bool HaveLastMsg_;

        qint64 msg_id_;
	};

	void RenderRecentsItem(QPainter &painter, const RecentItemVisualData &item, const ViewParams& viewParams);

    void RenderRecentsDragOverlay(QPainter &painter, const ViewParams& viewParams);

    void RenderUnknownsHeader(QPainter &painter, const QString& title, const int count, const ViewParams& viewParams);

    void RenderServiceItem(QPainter &painter, const QString& text, bool renderState, bool drawLine, const ViewParams& viewParams);

    void RenderDeleteAllItem(QPainter &painter, const QString& title, bool isMouseOver, const ViewParams& viewParams);

	int RenderContactMessage(QPainter &painter, const RecentItemVisualData &visData, const int rightBorderPx, const ViewParams& viewParams, ContactListParams& _recentParams);

	int RenderNotifications(QPainter &painter, const int unreads, bool muted, const ViewParams& viewParams, ContactListParams& _recentParams, bool _isUnknownHeader);

    void RenderLastReadAvatar(QPainter &painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams);

    int RenderAddContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams);

    int RenderRemoveContact(QPainter &painter, bool hasMouseOver, ContactListParams& _recentParams, const ViewParams& _viewParams);

    QRect AddContactFrame();

    QRect RemoveContactFrame();

    QRect DeleteAllFrame();
}
