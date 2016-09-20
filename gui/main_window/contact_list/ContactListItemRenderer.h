#pragma once

#include "Common.h"
#include "../../types/contact.h"

namespace ContactList
{
	void RenderServiceContact(QPainter &painter, const bool _isHovered, const bool _isActive, QString _name, Data::ContactType _type, int leftMargin, const ViewParams& viewParams_);

	void RenderContactItem(QPainter &painter, VisualDataBase item, ViewParams _viewParams);

	void RenderGroupItem(QPainter &painter, const QString &groupName, const ViewParams& viewParams_);

    void RenderContactsDragOverlay(QPainter& painter);

    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView, int width);
}
