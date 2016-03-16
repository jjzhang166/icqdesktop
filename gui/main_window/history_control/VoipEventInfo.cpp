#include "stdafx.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "../../../corelib/enumerations.h"
#include "../../../corelib/collection_helper.h"
#include "../../collection_helper_ext.h"

#include "VoipEventInfo.h"


namespace HistoryControl
{

    VoipEventInfoSptr VoipEventInfo::Make(const coll_helper &info, const int32_t timestamp)
    {
        assert(timestamp > 0);

        VoipEventInfoSptr result(
            new VoipEventInfo(timestamp)
        );

        if (result->unserialize(info))
        {
            return result;
        }

        return nullptr;
    }

    VoipEventInfo::VoipEventInfo(const int32_t timestamp)
        : Type_(voip_event_type::invalid)
        , DurationSec_(-1)
        , IsIncomingCall_(false)
        , Timestamp_(timestamp)
    {
        assert(timestamp > 0);
    }

    void VoipEventInfo::serialize(Out coll_helper & _coll) const
    {
        if (_coll.empty())
            assert(false);
        assert(!"the method should not be used");
    }

    bool VoipEventInfo::unserialize(const coll_helper &_coll)
    {
        assert(Type_ == voip_event_type::invalid);

        Type_ = _coll.get<voip_event_type>("type");

        const auto isValidType = ((Type_ > voip_event_type::min) && (Type_ < voip_event_type::max));
        assert(isValidType);
        if (!isValidType)
        {
            return false;
        }

        ContactAimid_ = _coll.get<QString>("sender_aimid");

        assert(!ContactAimid_.isEmpty());
        if (ContactAimid_.isEmpty())
        {
            return false;
        }

        ContactFriendly_ = _coll.get<QString>("sender_friendly");

        assert(!ContactFriendly_.isEmpty());
        if (ContactFriendly_.isEmpty())
        {
            return false;
        }

        DurationSec_ = _coll.get<int32_t>("duration_sec", -1);
        assert(DurationSec_ >= -1);

        IsIncomingCall_ = _coll.get<bool>("is_incoming", false);

        return true;
    }

    QString VoipEventInfo::formatEventText() const
    {
        assert(Type_ > voip_event_type::min);
        assert(Type_ < voip_event_type::max);

        QString result;
        result.reserve(128);

        switch (Type_)
        {

            case core::voip_event_type::missed_call:
                if (IsIncomingCall_)
                {
                    result += QT_TRANSLATE_NOOP("chat_event", "Missed call");
                }
                else
                {
                    result += QT_TRANSLATE_NOOP("chat_event", "Outgoing call");
                }
                break;

            case core::voip_event_type::call_ended:
                if (IsIncomingCall_)
                {
                    result += QT_TRANSLATE_NOOP("chat_event", "Incoming call");
                }
                else
                {
                    result += QT_TRANSLATE_NOOP("chat_event", "Outgoing call");
                }
                break;

            case core::voip_event_type::accept:
                return result;

            default:
                assert(!"unexpected event type");
                break;
        }

        if (DurationSec_ < 0)
        {
            return result;
        }

        result += " - ";

        QTime duration(0, 0);
        duration = duration.addSecs(DurationSec_);

        if (duration.hour() > 0)
        {
            result +=
                QString("%1:%2:%3")
                    .arg(duration.hour(), 2, 10, QChar('0'))
                    .arg(duration.minute(), 2, 10, QChar('0'))
                    .arg(duration.second(), 2, 10, QChar('0'));
        }
        else
        {
            result +=
                QString("%1:%2")
                .arg(duration.minute(), 2, 10, QChar('0'))
                .arg(duration.second(), 2, 10, QChar('0'));
        }

        return result;
    }

    const QString& VoipEventInfo::getContactAimid() const
    {
        assert(!ContactAimid_.isEmpty());
        return ContactAimid_;
    }

    const QString& VoipEventInfo::getContactFriendly() const
    {
        assert(!ContactFriendly_.isEmpty());
        return ContactFriendly_;
    }

    Themes::IThemePixmapSptr VoipEventInfo::loadIcon(const bool isHovered) const
    {
        if (isHovered)
        {
            return Themes::GetPixmap(Themes::PixmapResourceId::VoipEventCallEndedIcon);
        }

        switch(Type_)
        {
            case voip_event_type::missed_call:
                if (IsIncomingCall_)
                {
                    return Themes::GetPixmap(Themes::PixmapResourceId::VoipEventMissedIcon);
                }

                return Themes::GetPixmap(Themes::PixmapResourceId::VoipEventOutgoingCallIcon);

            case voip_event_type::accept:
                return nullptr;

            case voip_event_type::call_ended:
                if (IsIncomingCall_)
                {
                    return Themes::GetPixmap(Themes::PixmapResourceId::VoipEventIncomingCallIcon);
                }

                return Themes::GetPixmap(Themes::PixmapResourceId::VoipEventOutgoingCallIcon);

        }

        assert(!"unexpected event type");
        return nullptr;
    }

    int32_t VoipEventInfo::getTimestamp() const
    {
        assert(Timestamp_ > 0);
        return Timestamp_;
    }

    bool VoipEventInfo::isClickable() const
    {
#ifdef STRIP_VOIP
        return false;
#endif //STRIP_VOIP
        return isVisible();
    }

    bool VoipEventInfo::isIncomingCall() const
    {
        return IsIncomingCall_;
    }

    bool VoipEventInfo::isVisible() const
    {
        return ((Type_ == voip_event_type::call_ended) ||
                (Type_ == voip_event_type::missed_call));
    }

}
