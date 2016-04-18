#include "stdafx.h"
#include "LiveChatMembersControl.h"
#include "../../types/chat.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Ui
{
    LiveChatMembersControl::LiveChatMembersControl(QWidget* _parent, std::shared_ptr<Data::ChatInfo> _info, const int _maxCount)
        :   QWidget(_parent),
            maxCount_(_maxCount)
    {
        setFixedHeight(Utils::scale_value(controlHeight));
        
        int i = 0;
        for (const auto& chatMember : _info->Members_)
        {
            assert(!chatMember.getFriendly().isEmpty());
            members_.push_back(std::make_pair(chatMember.AimdId_, chatMember.getFriendly()));
            if ((++i) >= _maxCount)
                break;
        }

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(onAvatarLoaded(QString)), Qt::QueuedConnection);
    }

    LiveChatMembersControl::~LiveChatMembersControl()
    {
    }

    const int LiveChatMembersControl::sv(const int _v)
    {
        return Utils::scale_value(_v);
    }

    void LiveChatMembersControl::onAvatarLoaded(QString _aimid)
    {
        for (const auto& chat : members_)
        {
            if (chat.first == _aimid)
            {
                update();
                break;
            }
        }
    }

    void LiveChatMembersControl::adjustWidth()
    {
        int width = (sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth)) * members_.size() + (sv(avatarIntersection) + sv(frameWidth));
        
        setFixedWidth(width);
    }

    void LiveChatMembersControl::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);

        QPainter painter(this);

        painter.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform|QPainter::TextAntialiasing);

        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        int maxCount = (geometry().width() - sv(avatarWithFrameHeight)) / (sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth)) + 1;

        int xOffset = 0;
        
        for (const auto& chat : members_)
        {
            if (maxCount <= 0)
                break;

            painter.setBrush(Qt::white);
            painter.setPen(Qt::white);

            painter.drawEllipse(xOffset, 0, sv(avatarWithFrameHeight), sv(avatarWithFrameHeight));

            bool isDefault = false;
            const auto &avatar = Logic::GetAvatarStorage()->GetRounded(chat.first, chat.second, Utils::scale_bitmap(sv(avatarHeight)), QString(), true, isDefault);

            if (!avatar->isNull())
            {
                painter.drawPixmap(xOffset + sv(frameWidth), sv(frameWidth), sv(avatarHeight), sv(avatarHeight), *avatar);
            }

            xOffset += sv(avatarWithFrameHeight) - sv(avatarIntersection) - sv(frameWidth);

            --maxCount;
        }


        return QWidget::paintEvent(_e);
    }
}
