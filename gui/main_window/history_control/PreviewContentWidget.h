#pragma once

#include "MessageContentWidget.h"

namespace Ui
{
    class TextEditEx;
}

namespace HistoryControl
{

    class PreviewContentWidget : public MessageContentWidget
    {
        friend class ImagePreviewWidget;

        Q_OBJECT

    public:
        PreviewContentWidget(QWidget *parent, const bool isOutgoing, const QString &text, const bool previewsEnabled, QString _aimId);

        virtual QPoint deliveryStatusOffsetHint(const int32_t statusLineWidth) const override final;

        virtual bool hasTextBubble() const override;

        virtual void initialize() override;

        virtual void render(QPainter &p) override final;

        virtual void select(const bool value) override;

        virtual bool selectByPos(const QPoint &pos) override;

        virtual QSize sizeHint() const override;

        virtual bool haveContentMenu(QPoint) const override;
        
        bool isTextPresented();

    protected:
        const bool PreviewsEnabled_;

        QRect getPreviewScaledRect() const;

        const QString& getText() const;

        virtual bool isPlaceholderVisible() const;

        virtual bool isPreloaderVisible() const = 0;

        virtual void renderPreview(QPainter &p);

        void invalidateSizes();

        void renderTextBubble(QPainter &p);

        QPainterPath evaluateClippingPath() const;

        virtual void resizeEvent(QResizeEvent *event) override;

        void setPreview(const QPixmap &preview);

        void setPreviewGenuineSize(const QSize &size);

        void setTextVisible(const bool isVisible);

    private Q_SLOTS:
        void onPreviewSizeLimited(QPixmap preview);

    private:
        QPixmap Preview_;

        QSizeF PreviewGenuineSize_;

        // you should not access the variable directly,
        // use getPreviewScaledSize() instead
        mutable QSizeF PreviewScaledSize_;

        QPainterPath ClippingPath_;

        const QString Text_;

        // you should not access the variable directly,
        // use getTextSize() instead
        mutable QSize TextSize_;

        bool IsTextVisible_;

        Ui::TextEditEx *TextControl_;

        mutable QPainterPath TextBubble_;

        QSize LastSize_;

        void applyClippingPath(QPainter &p);

        void createTextControl();

        QSizeF evaluatePreviewScaledSize(const int boundWidth) const;

        const QSizeF& getPreviewScaledSizeF() const;

        const QPainterPath& getTextBubble() const;

        QSize getTextSize() const;

        QSize getTextBubbleSize() const;

        void limitPreviewSize();

        void prepareTextGeometry();

        void renderNoPreview(QPainter &p);

        void renderPreloader(QPainter &p);

        void renderPreloaderBubble(QPainter &p);

        QRect updateWidgetSize();

    };

}
