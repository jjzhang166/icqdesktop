#include "stdafx.h"

#include "../../../corelib/collection_helper.h"

#include "StickerInfo.h"

namespace HistoryControl
{

	StickerInfoSptr StickerInfo::Make(const core::coll_helper &coll)
	{
		const auto setId = coll.get_value_as_uint("set_id");
		assert(setId > 0);

		const auto stickerId = coll.get_value_as_uint("sticker_id");
		assert(stickerId > 0);

		return StickerInfoSptr(
			new StickerInfo(setId, stickerId)
		);
	}

	StickerInfo::StickerInfo(const quint32 setId, const quint32 stickerId)
		: SetId_(setId)
		, StickerId_(stickerId)
	{
		assert(SetId_ > 0);
		assert(StickerId_ > 0);
	}

}