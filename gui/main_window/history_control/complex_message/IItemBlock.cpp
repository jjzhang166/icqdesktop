#include "stdafx.h"

#include "IItemBlock.h"
#include "../../../types/message.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

IItemBlock::~IItemBlock()
{
}

Data::Quote IItemBlock::getQuote() const
{
    return Data::Quote();
}

UI_COMPLEX_MESSAGE_NS_END