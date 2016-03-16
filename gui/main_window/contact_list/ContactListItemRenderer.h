#pragma once

#include "Common.h"

namespace ContactList
{
	struct ContactListVisualData : VisualDataBase
	{
		ContactListVisualData(
			const QString &aimId,
			const QPixmap &avatar,
			const QString &state,
			const QString &status,
			const bool isHovered,
			const bool isSelected,
			const QString &contactName,
			const bool haveLastSeen,
			const QDateTime &lastSeen,
			bool isWithCheckBox,
            bool isChatMember);
	};

	void RenderServiceContact(QPainter &painter, const bool _isHovered, const bool _isActive, int _regim, QString _name, Data::ContactType _type);

	void RenderContactItem(QPainter &painter, const ContactListVisualData &item, int _regim, bool _shortView);

	void RenderGroupItem(QPainter &painter, const QString &groupName);

    void RenderContactsDragOverlay(QPainter& painter);

    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView);
}