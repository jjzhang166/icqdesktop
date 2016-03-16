#pragma once

namespace core
{
	class coll_helper;
}

namespace HistoryControl
{

	typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;

	class StickerInfo
	{
	public:
		static StickerInfoSptr Make(const core::coll_helper &coll);

		const quint32 SetId_;

		const quint32 StickerId_;

	private:
		StickerInfo(const quint32 setId, const quint32 stickerId);

	};

}