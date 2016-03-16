#pragma once

#include "MessageContentWidget.h"

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace HistoryControl
{

	typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;

	class StickerWidget : public MessageContentWidget
	{
		Q_OBJECT

	public:
        StickerWidget(QString _aimId);

		StickerWidget(QWidget *parent, const StickerInfoSptr &info, const bool isOutgoing, QString _aimId);

        virtual QPoint deliveryStatusOffsetHint(const int32_t statusLineWidth) const override;

		virtual bool canUnload() const override;

		virtual void initialize() override;

		virtual bool isBlockElement() const override;

		virtual QString toLogString() const override;

		virtual QString toString() const override;

        virtual void copyFile() override;

        virtual void saveAs() override;

        virtual bool haveContentMenu(QPoint) const override;

        virtual QString toLink() const override;

	private Q_SLOTS:
		void onSticker(qint32 setId, qint32 stickerId);

		void onStickers();

	private:
		const StickerInfoSptr Info_;

		QImage Sticker_;

        QSize LastSize_;

        Themes::IThemePixmapSptr Placeholder_;

		virtual void render(QPainter &p) override;

		void connectStickerSignal(const bool isConnected);

		void loadSticker();

		void renderSelected(QPainter &p);

		void requestSticker();

		void updateStickerSize();

	};

}