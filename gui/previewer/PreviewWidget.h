#pragma once

namespace Previewer
{

	class PreviewWidget : public QWidget
	{
	public:
		PreviewWidget(QPixmap& preview);

		virtual ~PreviewWidget() override;

	protected:
		virtual void keyPressEvent(QKeyEvent*) override;

		virtual void mouseMoveEvent(QMouseEvent*) override;

		virtual void mousePressEvent(QMouseEvent*) override;

		virtual void paintEvent(QPaintEvent*) override;

		virtual void resizeEvent(QResizeEvent*) override;

	private:
		QPixmap& SourcePreview_;

		QPixmap ScaledPreview_;

		QRect PreviewRect_;

		QRect CloseButtonRect_;

		void calculateCloseButtonRect(const QRect &widgetRect);

		void calculatePreviewBounds(const QRect &widgetRect);

		void calculatePreviewRectAndScale(const QRect &widgetRect);

		void drawCloseButton(QPainter &p);

		void drawPreview(QPainter &p);

		void releaseKeyboardAndClose();

	};

}