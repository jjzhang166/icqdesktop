#pragma once

namespace Themes
{
	enum class PixmapResourceId
	{
		Invalid,
		Min,

		ContactListSendingMark,
		ContactListReadMark,
		ContactListDeliveredMark,
		ContactListAddContact,
		ContactListAddContactHovered,

		FileSharingDownload,
		FileSharingPlainCancel,
        FileSharingMediaCancel,
		FileSharingFileTypeIconUnknown,

		PreviewerClose,
		ContentMuteNotify,
		ContentMuteNotifyNew,

        VoipEventMissedIcon,
        VoipEventIncomingCallIcon,
        VoipEventOutgoingCallIcon,
        VoipEventCallEndedIcon,

        StickerHistoryPlaceholder,
        StickerPickerPlaceholder,

		Max
	};
}