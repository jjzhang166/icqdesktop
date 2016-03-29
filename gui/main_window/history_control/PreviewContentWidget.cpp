#include "stdafx.h"

#include "../../utils/log/log.h"
#include "../../utils/PainterPath.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../utils/Text.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"

#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "../../controls/TextEditEx.h"

#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "ResizePixmapTask.h"

#include "PreviewContentWidget.h"
#include "../../theme_settings.h"

namespace HistoryControl
{
    namespace
    {
        int32_t getBubbleHorPadding();

        int32_t getBubbleVertPadding();

        int32_t getClippingPathBorderRadius();

        const QSizeF& getMaxPreviewSize();

        const QSizeF& getMinPreviewSize();

        QSize getPlaceholderSize();

        const QFont& getPreviewTextFont();

        int32_t getTextBottomMargin();

        int32_t getTextBubbleBorderRadius();
    }

    PreviewContentWidget::PreviewContentWidget(QWidget *parent, const bool isOutgoing, const QString &text, const bool previewsEnabled, QString _aimId)
        : MessageContentWidget(parent, isOutgoing, _aimId)
        , Text_(text)
        , TextControl_(nullptr)
        , IsTextVisible_(false)
        , PreviewsEnabled_(previewsEnabled)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        setMinimumSize(getMinPreviewSize().toSize());

        updateGeometry();
    }

    QPoint PreviewContentWidget::deliveryStatusOffsetHint(const int32_t statusLineWidth) const
    {
        if (TextControl_ && TextControl_->isVisible())
        {
            const auto textSize = getTextBubbleSize();
            return QPoint(
                textSize.width() - statusLineWidth,
                textSize.height()
            );
        }

        if (PreviewsEnabled_ && !isBlockElement())
        {
            return QPoint(
                getPreviewScaledRect().width(),
                0
            );
        }

        return MessageContentWidget::deliveryStatusOffsetHint(statusLineWidth);
    }

    bool PreviewContentWidget::hasTextBubble() const
    {
        return TextControl_;
    }

    void PreviewContentWidget::initialize()
    {
        MessageContentWidget::initialize();
    }

    void PreviewContentWidget::select(const bool value)
    {
        MessageContentWidget::select(value);

        if (TextControl_)
        {
            TextControl_->clearSelection();
        }
    }

    bool PreviewContentWidget::selectByPos(const QPoint &pos)
    {
        assert(TextControl_);

        TextControl_->selectByPos(pos);

        return true;
    }

    QSize PreviewContentWidget::sizeHint() const
    {
        if (PreviewGenuineSize_.isEmpty())
        {
            return getMinPreviewSize().toSize();
        }

        const auto &previewScaledSize = getPreviewScaledSizeF();

        const auto bubbleSize = getTextBubbleSize();

        const auto width = previewScaledSize.width();
        const auto height = (
            bubbleSize.height() +
            getTextBottomMargin() +
            previewScaledSize.height()
        );

        QSize widgetSize(
            std::max(width, getMinPreviewSize().width()),
            std::max(height, getMinPreviewSize().height())
        );

        return widgetSize;
    }

    bool PreviewContentWidget::haveContentMenu(QPoint p) const
    {
        if (!TextControl_)
        {
            return true;
        }

        const auto local = mapFromGlobal(p);
        if (local.y() < getTextBubbleSize().height())
        {
            return false;
        }

        return true;
    }

    const QString& PreviewContentWidget::getText() const
    {
        return Text_;
    }

    bool PreviewContentWidget::isPlaceholderVisible() const
    {
        return true;
    }

    void PreviewContentWidget::renderPreview(QPainter &p)
    {
        if (Preview_.isNull())
        {
            renderNoPreview(p);

            return;
        }

        const auto imageRect = updateWidgetSize();

        p.save();

        p.drawPixmap(imageRect, Preview_);

        if (isSelected())
        {
            const QBrush brush(Utils::getSelectionColor());
            p.fillRect(imageRect, brush);
        }

        p.restore();
    }

    void PreviewContentWidget::renderTextBubble(QPainter &p)
    {
        const auto &bubblePath = getTextBubble();
        if (bubblePath.isEmpty())
        {
            return;
        }

        p.save();
        
        int theme_id = Ui::get_qt_theme_settings()->themeIdForContact(aimId_);
        
        QBrush b;
        p.fillPath(
            bubblePath,
            Ui::MessageStyle::getBodyBrush(isOutgoing(), isSelected(), theme_id)
        );

        p.restore();
    }

    QPainterPath PreviewContentWidget::evaluateClippingPath() const
    {
        const auto borderRadius = getClippingPathBorderRadius();

        const auto pathSize = getPreviewScaledSizeF().toSize();
        assert(pathSize.isValid());

        auto path = Utils::renderMessageBubble(pathSize, borderRadius, isOutgoing());

        if (TextControl_)
        {
            path.translate(
                0,
                getTextBubbleSize().height() + getTextBottomMargin()
            );
        }

        return path;
    }

    void PreviewContentWidget::render(QPainter &p)
    {
        p.save();

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);

        applyClippingPath(p);

        renderPreview(p);

        p.restore();

        renderTextBubble(p);
    }

    void PreviewContentWidget::resizeEvent(QResizeEvent *e)
    {
        MessageContentWidget::resizeEvent(e);

        invalidateSizes();
    }

    void PreviewContentWidget::setPreview(const QPixmap &preview)
    {
        assert(!preview.isNull());

        __TRACE(
            "preview",
            "setting preview\n" <<
            toLogString() << "\n"
            "----------------------------\n"
            "    preview_size=<" << preview.size() << ">"
        );

        if (PreviewGenuineSize_.isEmpty())
        {
            setPreviewGenuineSize(preview.size());
        }

        Preview_ = preview;

        limitPreviewSize();

        invalidateSizes();

        update();
    }

    void PreviewContentWidget::setPreviewGenuineSize(const QSize &size)
    {
        assert(!size.isEmpty());
        assert(PreviewGenuineSize_.isEmpty());
        assert(Preview_.isNull());

        PreviewGenuineSize_ = size;

        updateGeometry();
    }

    void PreviewContentWidget::setTextVisible(const bool isVisible)
    {
        IsTextVisible_ = isVisible;

        TextSize_ = QSize();

        if (IsTextVisible_)
        {
            if (TextControl_)
            {
                return;
            }

            createTextControl();
        }
        else
        {
            if(!TextControl_)
            {
                return;
            }

            TextControl_->hide();
            delete TextControl_;
            TextControl_ = nullptr;
        }

        emit forcedLayoutUpdatedSignal();

        updateGeometry();

        update();
    }

    void PreviewContentWidget::onPreviewSizeLimited(QPixmap preview)
    {
        assert(preview);

        Preview_ = preview;
    }

    void PreviewContentWidget::applyClippingPath(QPainter &p)
    {
        const auto minExtent = (getClippingPathBorderRadius() * 2);

        const auto isPreviewTooSmall = (
            (PreviewGenuineSize_.width() <= minExtent) ||
            (PreviewGenuineSize_.height() <= minExtent)
        );

        if (isPreviewTooSmall)
        {
            return;
        }

        if (ClippingPath_.isEmpty())
        {
            ClippingPath_ = evaluateClippingPath();
            assert(!ClippingPath_.isEmpty());
        }

        p.setClipPath(ClippingPath_);
    }

    void PreviewContentWidget::createTextControl()
    {
        assert(IsTextVisible_);
        assert(!Text_.isEmpty());
        assert(!TextControl_);

        TextControl_ = new Ui::TextEditEx(
            this,
            Utils::FontsFamily::SEGOE_UI,
            Utils::scale_value(15),
            QColor(0x28, 0x28, 0x28),
            false,
            false
        );

        TextControl_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        TextControl_->setStyle(QApplication::style());
        TextControl_->setFrameStyle(QFrame::NoFrame);
        TextControl_->setDocument(TextControl_->document());
        TextControl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextControl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextControl_->setOpenLinks(true);
        TextControl_->setOpenExternalLinks(true);
        TextControl_->setWordWrapMode(QTextOption::WordWrap);
        TextControl_->setStyleSheet("background: transparent");
        TextControl_->document()->setDocumentMargin(0);

        TextControl_->verticalScrollBar()->blockSignals(true);
        Logic::Text4Edit(Text_, *TextControl_, Logic::Text2DocHtmlMode::Escape, true, true);
        TextControl_->verticalScrollBar()->blockSignals(false);

        TextControl_->show();
    }

    QSizeF PreviewContentWidget::evaluatePreviewScaledSize(const int boundWidth) const
    {
        const auto &previewSize = (
            PreviewGenuineSize_.isEmpty() ?
                getPlaceholderSize() :
                PreviewGenuineSize_
        );

        QSizeF fixedSize(boundWidth, 0);

        // prevent small images from being stretched up
        fixedSize.setWidth(
            std::min(fixedSize.width(), previewSize.width())
        );

        const auto verticalAspectRatio = ((double)previewSize.height() / (double)previewSize.width());
        fixedSize.setHeight(fixedSize.width() * verticalAspectRatio);

        const auto shouldScaleDown =
            (fixedSize.width() > getMaxPreviewSize().width()) ||
            (fixedSize.height() > getMaxPreviewSize().height());
        if (shouldScaleDown)
        {
            fixedSize = fixedSize.scaled(getMaxPreviewSize(), Qt::KeepAspectRatio);
        }

        const auto shouldScaleUp =
            (fixedSize.width() < getMinPreviewSize().width()) &&
            (fixedSize.height() < getMinPreviewSize().height());
        if (shouldScaleUp)
        {
            fixedSize = fixedSize.scaled(getMinPreviewSize(), Qt::KeepAspectRatio);
        }

        return fixedSize;
    }

    QRect PreviewContentWidget::getPreviewScaledRect() const
    {
        QPoint topLeft(0, 0);

        if (TextControl_)
        {
            topLeft.ry() +=
                getTextBubbleSize().height() +
                getTextBottomMargin();
        }

        QRect result(
            QPoint(0, 0),
            getPreviewScaledSizeF().toSize()
        );

        result.moveTopLeft(topLeft);

        return result;
    }

    const QSizeF& PreviewContentWidget::getPreviewScaledSizeF() const
    {
        if (!PreviewScaledSize_.isValid())
        {
            PreviewScaledSize_ = evaluatePreviewScaledSize(width());
            assert(PreviewScaledSize_.isValid());
        }

        return PreviewScaledSize_;
    }

    const QPainterPath& PreviewContentWidget::getTextBubble() const
    {
        const auto textBubbleEmpty = TextBubble_.isEmpty();
        if (!textBubbleEmpty)
        {
            return TextBubble_;
        }

        const auto &textSize = getTextSize();
        if (textSize.isEmpty())
        {
            return TextBubble_;
        }

        const auto textBubbleSize = getTextBubbleSize();

        TextBubble_ = Utils::renderMessageBubble(
            textBubbleSize,
            getTextBubbleBorderRadius(),
            isOutgoing()
        );

        assert(!TextBubble_.isEmpty());

        return TextBubble_;
    }

    QSize PreviewContentWidget::getTextSize() const
    {
        if (!TextControl_)
        {
            TextSize_ = QSize(0, 0);
        }

        if (TextSize_.isValid())
        {
            return TextSize_;
        }

        return TextControl_->getTextSize();
    }

    QSize PreviewContentWidget::getTextBubbleSize() const
    {
        auto bubbleSize = getTextSize();

        const auto textHeight = bubbleSize.height();

        bubbleSize.setHeight(
            std::max(
                Utils::scale_value(32),
                textHeight
            )
        );

        bubbleSize.setHeight(
            Utils::applyMultilineTextFix(textHeight, bubbleSize.height())
        );

        bubbleSize.rwidth() += getBubbleHorPadding();
        bubbleSize.rwidth() += getBubbleHorPadding();
        bubbleSize.rwidth() += Ui::MessageStatusWidget::getMaxWidth(isOutgoing());
        bubbleSize.rheight() += getBubbleVertPadding();

        return bubbleSize;
    }

    void PreviewContentWidget::limitPreviewSize()
    {
        assert(!Preview_.isNull());
        assert(!PreviewGenuineSize_.isEmpty());

        const auto previewSize = PreviewGenuineSize_;
        const auto shouldScalePreviewDown =
            (previewSize.width() > getMaxPreviewSize().width()) ||
            (previewSize.height() > getMaxPreviewSize().height());
        if (!shouldScalePreviewDown)
        {
            return;
        }

        Utils::check_pixel_ratio(Preview_);

        const auto scaledSize = Utils::scale_bitmap(getMaxPreviewSize().toSize());

        auto task = new ResizePixmapTask(Preview_, scaledSize);

        const auto succeed = QObject::connect(
            task, &ResizePixmapTask::resizedSignal,
            this, &PreviewContentWidget::onPreviewSizeLimited
        );
        assert(succeed);

        QThreadPool::globalInstance()->start(task);
    }

    void PreviewContentWidget::invalidateSizes()
    {
        // invalidate size-dependent children

        ClippingPath_ = QPainterPath();

        PreviewScaledSize_ = QSize();
        assert(!PreviewScaledSize_.isValid());

        TextSize_ = QSize();
        assert(!TextSize_.isValid());

        TextBubble_ = QPainterPath();
        assert(TextBubble_.isEmpty());
    }

    void PreviewContentWidget::prepareTextGeometry()
    {
        if (!TextControl_)
        {
            return;
        }

        auto documentWidth = (
            width() -
            getBubbleHorPadding() -
            getBubbleHorPadding() -
            Ui::MessageStatusWidget::getMaxWidth(isOutgoing())
        );

        const auto textWidthChanged = (TextControl_->width() != documentWidth);
        if (textWidthChanged)
        {
            TextControl_->document()->setTextWidth(documentWidth);
            TextControl_->setFixedWidth(documentWidth);
        }

        TextControl_->move(
            getBubbleHorPadding(),
            getBubbleVertPadding()
        );
    }

    void PreviewContentWidget::renderNoPreview(QPainter &p)
    {
        const auto nothingToRender = (!isPlaceholderVisible() && !TextControl_);
        if (nothingToRender)
        {
            return;
        }

        if (!isPlaceholderVisible())
        {
            prepareTextGeometry();

            const QSize newSize(
                width(),
                getTextBubbleSize().height()
            );

            if (newSize != LastSize_)
            {
                setFixedSize(newSize);

                LastSize_ = newSize;
            }

            return;
        }

        updateWidgetSize();

        renderPreloader(p);
    }

    void PreviewContentWidget::renderPreloader(QPainter &p)
    {
        renderPreloaderBubble(p);

        if (!isPreloaderVisible())
        {
            return;
        }

        p.save();

    }

    void PreviewContentWidget::renderPreloaderBubble(QPainter &p)
    {
        p.save();

        auto bodyBrush = (
            isOutgoing() ?
                Ui::MessageStyle::getOutgoingBodyColorA(0.7) :
                Ui::MessageStyle::getIncomingBodyColorA(0.7)
        );

        p.setBrush(bodyBrush);

        const auto borderRadius = Utils::scale_value(8);
        p.drawRoundedRect(getPreviewScaledRect(), borderRadius, borderRadius);

        p.restore();
    }

    QRect PreviewContentWidget::updateWidgetSize()
    {
        auto imageRect = getPreviewScaledRect();

        imageRect = QRect(
            imageRect.left(),
            imageRect.top(),
            std::max(imageRect.width(), 2),
            std::max(imageRect.height(), 2)
        );

        auto widgetHeight = imageRect.height();

        if (TextControl_)
        {
            widgetHeight += getTextBubbleSize().height();
            widgetHeight += getTextBottomMargin();
        }

        const QSize widgetSize(
            width(),
            widgetHeight
        );

        if (LastSize_ != widgetSize)
        {
            prepareTextGeometry();
            MessageContentWidget::setFixedSize(widgetSize);
        }

        LastSize_ = widgetSize;

        return imageRect;
    }
    
    bool PreviewContentWidget::isTextPresented()
    {
        return TextControl_;
    }

    namespace
    {
        int32_t getBubbleHorPadding()
        {
            return Utils::scale_value(16);
        }

        int32_t getBubbleVertPadding()
        {
            return Utils::scale_value(5);
        }

        int32_t getClippingPathBorderRadius()
        {
            return Utils::scale_value(8);
        }

        const QSizeF& getMaxPreviewSize()
        {
            static const QSizeF size(
                Utils::scale_value(640),
                Utils::scale_value(320)
            );

            return size;
        }

        const QSizeF& getMinPreviewSize()
        {
            static const QSizeF size(
                Utils::scale_value(48),
                Utils::scale_value(48)
            );

            return size;
        }

        QSize getPlaceholderSize()
        {
            return QSize(Utils::scale_value(320), Utils::scale_value(240));
        }

        const QFont& getPreviewTextFont()
        {
            static QFont font(
                Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI)
            );

            return font;
        }

        int32_t getTextBottomMargin()
        {
            return Utils::scale_value(2);
        }

        int32_t getTextBubbleBorderRadius()
        {
            return Utils::scale_value(8);
        }
    }
}
