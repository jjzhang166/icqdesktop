#include "stdafx.h"
#include "ContactAvatarWidget.h"
#include "../utils/utils.h"
#include "../cache/avatars/AvatarStorage.h"

namespace Ui
{
	ContactAvatarWidget::ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _display_name, int _size, bool _autoUpdate)
		:	QPushButton(_parent), aimid_(_aimid), display_name_(_display_name), size_(_size)
	{
		setFixedHeight(_size);
		setFixedWidth(_size);

        if (_autoUpdate)
            connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);
	}

    ContactAvatarWidget::~ContactAvatarWidget()
    {
    }
    
	void ContactAvatarWidget::paintEvent(QPaintEvent* _e)
	{
		bool isDefault = false;
		const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimid_, display_name_, Utils::scale_bitmap(size_), QString(), true, isDefault);

		if (avatar->isNull())
			return;

		QPainter p(this);
		p.drawPixmap(0, 0, size_, size_, *avatar);

		return QWidget::paintEvent(_e);
	}

    void ContactAvatarWidget::UpdateParams(const QString& _aimid, const QString& _display_name)
    {
        aimid_ = _aimid;
        display_name_ = _display_name;
    }

    void ContactAvatarWidget::avatarChanged(QString _aimId)
    {
        if (_aimId == aimid_)
            update();
    }
}
