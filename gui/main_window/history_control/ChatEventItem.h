#pragma once

#include "HistoryControlPageItem.h"

namespace HistoryControl
{
	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;
}

namespace Ui
{
    class DpiAwareImage;
    class TextEditEx;

	class ChatEventItem : public HistoryControlPageItem
	{
		Q_OBJECT

	public:
        ChatEventItem(const HistoryControl::ChatEventInfoSptr& eventInfo);

		ChatEventItem(QWidget *parent, const HistoryControl::ChatEventInfoSptr& eventInfo);

        ~ChatEventItem();

		virtual QString formatRecentsText() const override;

	private:
		QRect BubbleRect_;

		std::unique_ptr<DpiAwareImage> Icon_;

        TextEditEx *TextWidget_;

		const HistoryControl::ChatEventInfoSptr EventInfo_;

        int32_t evaluateFullIconWidth();

        int32_t evaluateTextHeight(const int32_t textWidth);

        int32_t evaluateTextWidth(const int32_t widgetWidth);

        void updateTheme();

		virtual void paintEvent(QPaintEvent *event) override;

		virtual void resizeEvent(QResizeEvent *event) override;

	};

}