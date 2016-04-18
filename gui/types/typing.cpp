#include "stdafx.h"
#include "typing.h"

#include "../main_window/contact_list/ContactListModel.h"

QString Logic::TypingFires::getChatterName() const
{
    if (!chatterAimId_.isEmpty())
    {
        if (!chatterName_.isEmpty())
        {
            return chatterName_;
        }

        return Logic::GetContactListModel()->getDisplayName(chatterAimId_);
    }
    else
    {
        return Logic::GetContactListModel()->getDisplayName(aimId_);
    }
}
