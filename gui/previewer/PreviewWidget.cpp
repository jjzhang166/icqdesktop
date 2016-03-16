#include "stdafx.h"

#include "../utils/utils.h"
#include "../themes/ThemePixmap.h"
#include "../themes/ResourceIds.h"

#include "PreviewWidget.h"

namespace
{

}

namespace Previewer
{

	PreviewWidget::PreviewWidget(QPixmap& preview)
		: QWidget(nullptr, Qt::Window)
        , SourcePreview_(preview)
	{
		assert(!preview.isNull());

		setWindowModality(Qt::ApplicationModal);

		setAttribute(Qt::WA_TranslucentBackground);

		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

		setMouseTracking(true);

		grabKeyboard();
	}

	PreviewWidget::~PreviewWidget()
	{
	}

	void PreviewWidget::keyPressEvent(QKeyEvent *e)
	{
		if (e->key() == Qt::Key_Escape)
		{
			releaseKeyboardAndClose();
		}
	}

	void PreviewWidget::mouseMoveEvent(QMouseEvent *e)
	{
		const auto pos = e->pos();

		if (CloseButtonRect_.contains(pos))
		{
			setCursor(Qt::PointingHandCursor);
			return;
		}

		setCursor(Qt::ArrowCursor);
	}

	void PreviewWidget::mousePressEvent(QMouseEvent*)
	{
		releaseKeyboardAndClose();
	}

	void PreviewWidget::paintEvent(QPaintEvent*)
	{
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.setRenderHint(QPainter::TextAntialiasing);

		drawPreview(p);

		drawCloseButton(p);
	}

	void PreviewWidget::resizeEvent(QResizeEvent *event)
	{
		if (event)
		{
			const auto &size = event->size();
			QRect rect(0, 0, size.width(), size.height());

			calculateCloseButtonRect(rect);
			calculatePreviewRectAndScale(rect);
		}

		QWidget::resizeEvent(event);
	}

	void PreviewWidget::calculateCloseButtonRect(const QRect &widgetRect)
	{
		static const auto bottomPadding = Utils::scale_value(24);
		static const auto buttonSize = Utils::scale_value(56);

		QRect buttonRect(0, 0, buttonSize, buttonSize);
		buttonRect.moveCenter(widgetRect.center());
		buttonRect.moveBottom(widgetRect.height() - bottomPadding);

		CloseButtonRect_ = buttonRect;
	}

	void PreviewWidget::calculatePreviewRectAndScale(const QRect &widgetRect)
	{
		assert(!CloseButtonRect_.isEmpty());
		assert(!SourcePreview_.isNull());

		const auto marginsLtr = (widgetRect.height() - CloseButtonRect_.top());
		const auto marginB = (marginsLtr * 1.5);
		const QMargins margins(marginsLtr, marginsLtr, marginsLtr, marginB);
		const auto previewBounds = widgetRect.marginsRemoved(margins);

		if (previewBounds.isEmpty())
		{
			ScaledPreview_ = QPixmap();
			PreviewRect_ = QRect();
			return;
		}

		const auto shouldScalePreview = (previewBounds.width() < SourcePreview_.width()) ||
										(previewBounds.height() < SourcePreview_.height());
		if (shouldScalePreview)
		{
			ScaledPreview_ = SourcePreview_.scaled(previewBounds.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			ScaledPreview_ = SourcePreview_;
		}

		auto rect = ScaledPreview_.rect();
		rect.moveCenter(previewBounds.center());

		PreviewRect_ = rect;
	}

	void PreviewWidget::drawCloseButton(QPainter &p)
	{
		p.save();

		p.setBrush(Qt::black);
		p.drawRoundRect(CloseButtonRect_, 16, 16);

		static const auto &closeButton = Themes::GetPixmap(Themes::PixmapResourceId::PreviewerClose);

		auto closeIconRect = closeButton->GetRect();
		closeIconRect.moveCenter(CloseButtonRect_.center());

		closeButton->Draw(p, closeIconRect);

		p.restore();
	}

	void PreviewWidget::drawPreview(QPainter &p)
	{
		static const auto BG_COLOR = QColor::fromRgba(qRgba(0x00, 0x00, 0x00, 0xCC));
		static const auto BG_BRUSH = QBrush(BG_COLOR);

		p.save();

		p.setBrush(BG_BRUSH);
		p.drawRect(0, 0, width(), height());

		if (!ScaledPreview_.isNull())
		{
			p.drawPixmap(PreviewRect_, ScaledPreview_);
		}

		p.restore();
	}

	void PreviewWidget::releaseKeyboardAndClose()
	{
		releaseKeyboard();
		close();
        SourcePreview_ = QPixmap();
        ScaledPreview_ = QPixmap();
	}

}