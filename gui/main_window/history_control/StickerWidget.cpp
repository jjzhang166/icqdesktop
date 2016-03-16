#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../utils/utils.h"

#include "../../cache/stickers/stickers.h"
#include "../../core_dispatcher.h"

#include "StickerInfo.h"
#include "StickerWidget.h"

using namespace Ui::stickers;

namespace HistoryControl
{

	namespace
	{
		qint32 getStickerMaxHeight();

		core::sticker_size getStickerSize();
	}

    StickerWidget::StickerWidget(QString _aimId)
        : MessageContentWidget(0, false, _aimId)
    {
    }

	StickerWidget::StickerWidget(QWidget *parent, const StickerInfoSptr &info, const bool isOutgoing, QString _aimId)
		: MessageContentWidget(parent, isOutgoing, _aimId)
		, Info_(info)
	{
		assert(Info_);

        assert(!Placeholder_);
        Placeholder_ = Themes::GetPixmap(Themes::PixmapResourceId::StickerHistoryPlaceholder);
	}

    QPoint StickerWidget::deliveryStatusOffsetHint(const int32_t) const
    {
        if (Sticker_.isNull())
        {
            return QPoint(
                Placeholder_->GetWidth(),
                0
            );
        }

        return QPoint(
            Sticker_.width(),
            0
        );
    }

	bool StickerWidget::canUnload() const
	{
		return true;
	}

	void StickerWidget::initialize()
	{
		connectStickerSignal(true);

		requestSticker();

		setFixedSize(Placeholder_->GetSize());
	}

	bool StickerWidget::isBlockElement() const
	{
		return false;
	}

	QString StickerWidget::toLogString() const
	{
		QString result;
        result.reserve(1024);

        QTextStream fmt(&result);

        fmt << "sticker/" << Info_->SetId_ << "/" << Info_->StickerId_;

        return result;
	}

	QString StickerWidget::toString() const
	{
		return QT_TRANSLATE_NOOP("contact_list", "Sticker");
	}

    QString StickerWidget::toLink() const
    {
        assert(false);
        return QT_TRANSLATE_NOOP("contact_list", "Sticker");
    }

    void StickerWidget::copyFile()
    {
        assert(false);
    }

    void StickerWidget::saveAs()
    {
        assert(false);
    }

    bool StickerWidget::haveContentMenu(QPoint) const
    {
        return false;
    }

	void StickerWidget::onSticker(qint32 setId, qint32 stickerId)
	{
		assert(setId > 0);
		assert(stickerId > 0);

		const auto isMySticker = ((Info_->SetId_ == setId) && (Info_->StickerId_ == stickerId));
		if (!isMySticker)
		{
			return;
		}

		if (!Sticker_.isNull())
		{
			return;
		}

		loadSticker();
	}

	void StickerWidget::onStickers()
	{
		if (Sticker_.isNull())
		{
			requestSticker();
		}
	}

	void StickerWidget::render(QPainter &p)
	{
		p.save();

		if (Sticker_.isNull())
		{
            assert(Placeholder_);
            Placeholder_->Draw(p, 0, 0);
		}
		else
		{
            updateStickerSize();

			p.drawImage(
                QRect(
                    0, 0,
                    LastSize_.width(), LastSize_.height()
                ),
                Sticker_
            );
		}

        if (isSelected())
        {
		    renderSelected(p);
        }

        p.restore();
	}

	void StickerWidget::connectStickerSignal(const bool isConnected)
	{
		connectCoreSignal(
			SIGNAL(on_sticker(qint32, qint32)),
			SLOT(onSticker(qint32, qint32)),
			isConnected
		);

		connectCoreSignal(
			SIGNAL(on_stickers()),
			SLOT(onStickers()),
			isConnected
		);
	}

	void StickerWidget::loadSticker()
	{
		assert(Sticker_.isNull());

		const auto sticker = get_sticker(Info_->SetId_, Info_->StickerId_);
		if (!sticker)
		{
			return;
		}

		Sticker_ = sticker->get_image(getStickerSize());

		if (Sticker_.isNull())
		{
			return;
		}

		connectStickerSignal(false);

        updateGeometry();
		update();
	}

	void StickerWidget::renderSelected(QPainter &p)
	{
        assert(isSelected());

        const QBrush brush(Utils::getSelectionColor());
		p.fillRect(rect(), brush);
	}

	void StickerWidget::requestSticker()
	{
		assert(Sticker_.isNull());

		Ui::GetDispatcher()->getSticker(Info_->SetId_, Info_->StickerId_, getStickerSize());
	}

	void StickerWidget::updateStickerSize()
	{
        auto stickerSize = Sticker_.size();

        const auto scaleDown = (stickerSize.height() > getStickerMaxHeight());

		if (!scaleDown)
		{
            if (LastSize_ != stickerSize)
            {
                setFixedSize(
                    Utils::scale_bitmap(stickerSize)
                );
            }

            LastSize_ = stickerSize;

			return;
		}

        const auto aspectRatio = ((double)stickerSize.width() / (double)stickerSize.height());
		const auto fixedStickerSize =
			Utils::scale_bitmap(
				QSize(
					getStickerMaxHeight() * aspectRatio,
					getStickerMaxHeight()
				)
			);

        if (LastSize_ != fixedStickerSize)
        {
		    setFixedSize(fixedStickerSize);
        }

        LastSize_ = fixedStickerSize;
	}

	namespace
	{
		qint32 getStickerMaxHeight()
		{
			const auto scalePercents = (int)Utils::scale_value(100);

#ifdef __APPLE__
            if (Utils::is_mac_retina())
            {
                return 65;
            }
#endif

			switch (scalePercents)
			{
				case 100: return Utils::scale_value(150);
				case 125: return Utils::scale_value(225 * 0.8);
				case 150: return Utils::scale_value(225);
				case 200: return Utils::scale_value(300);
			}

			assert(!"unexpected scale value");
			return Utils::scale_value(150);
		}

		core::sticker_size getStickerSize()
		{
			const auto scalePercents = (int)Utils::scale_value(100);

#ifdef __APPLE__
            if (Utils::is_mac_retina())
            {
                return core::sticker_size::medium;
            }
#endif

			switch (scalePercents)
			{
				case 100: return core::sticker_size::small;
				case 125: return core::sticker_size::medium;
				case 150: return core::sticker_size::medium;
				case 200: return core::sticker_size::large;
			}

			assert(!"unexpected scale");
			return core::sticker_size::small;
		}
	}

}