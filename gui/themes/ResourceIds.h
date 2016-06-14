#pragma once

namespace Themes
{
	enum class PixmapResourceId
	{
		Invalid,
		Min,

		ContactListAddContact,
		ContactListAddContactHovered,

		FileSharingDownload,
		FileSharingPlainCancel,
        FileSharingMediaCancel,
        FileSharingMediaPlay,
        FileSharingMediaPlayActive,
        FileSharingMediaPlayHover,

		FileSharingFileTypeIconUnknown,
        FileSharingFileTypeIconExe,
        FileSharingFileTypeIconExcel,
        FileSharingFileTypeIconHtml,
        FileSharingFileTypeIconImage,
        FileSharingFileTypeIconPdf,
        FileSharingFileTypeIconPpt,
        FileSharingFileTypeIconPresentation,
        FileSharingFileTypeIconSound,
        FileSharingFileTypeIconTxt,
        FileSharingFileTypeIconVideo,
        FileSharingFileTypeIconWord,
        FileSharingFileTypeIconZip,

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