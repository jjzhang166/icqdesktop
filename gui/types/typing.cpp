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

        return Logic::getContactListModel()->getDisplayName(chatterAimId_);
    }
    else
    {
        return Logic::getContactListModel()->getDisplayName(aimId_);
    }
}
