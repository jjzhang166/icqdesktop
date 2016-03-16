#pragma once

#include "../../controls/DpiAwareImage.h"
#include "../../controls/TextEmojiWidget.h"

#include "HistoryControlPageItem.h"

namespace HistoryControl
{
	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;
}

namespace Ui
{
	class ChatEventItem : public HistoryControlPageItem
	{
		Q_OBJECT

	public:
        ChatEventItem(const HistoryControl::ChatEventInfoSptr& eventInfo);

		ChatEventItem(QWidget *parent, const HistoryControl::ChatEventInfoSptr& eventInfo);

		virtual QString formatRecentsText() const override;

        virtual QSize sizeHint() const override;

	private:
		QRect BubbleRect_;

		qint32 CachedTextWidth_;

		DpiAwareImage Icon_;

        TextEmojiWidget emojer_;

		const HistoryControl::ChatEventInfoSptr EventInfo_;

		virtual void paintEvent(QPaintEvent *event) override;

		virtual void resizeEvent(QResizeEvent *event) override;

	};

}