#pragma once

#include "PreviewContentWidget.h"

namespace HistoryControl
{

	class ImagePreviewWidget : public PreviewContentWidget
	{
		Q_OBJECT

	public:
		ImagePreviewWidget(QWidget *parent, const bool isOutgoing, const QString &uri, const QString &text, const bool previewsEnabled, QString _aimId);

		virtual ~ImagePreviewWidget();

		virtual bool isBlockElement() const override;

		virtual bool canUnload() const override;

		virtual QString toLogString() const override;

		virtual QString toString() const override;

        virtual void copyFile() override;

        virtual void saveAs() override;

        virtual QString toLink() const override;

        bool isPreviewVisible() const;

        bool isImageBubbleVisible() const;

        virtual void onVisibilityChanged(const bool isVisible) override;

    protected:
        virtual void initialize() override;

        virtual bool isPlaceholderVisible() const override;

        virtual bool isPreloaderVisible() const override;

        virtual void leaveEvent(QEvent *e) override;

        virtual void mouseMoveEvent(QMouseEvent *e) override;

        virtual void mousePressEvent(QMouseEvent *e) override;

        virtual void mouseReleaseEvent(QMouseEvent *e) override;

        virtual bool drag() override;

    private Q_SLOTS:
        void onImageDownloadError(qint64 seq, QString rawUri);

        void onImageDownloaded(int64_t seq, QString, QPixmap image, QString);

	private:
        enum class State;

        static bool isLeftButtonClick(QMouseEvent *e);

        void connectCoreSignals();

        bool isOverPicture(const QPoint &p) const;

        void onFullImageDownloaded(QString uri, QPixmap image, QString local);

        void onLeftMouseClick(const QPoint &click);

        void onPreviewDownloaded(QString uri, QPixmap image, QString local);

        void openPreviewer(const QPoint &click);

        State State_;

        const QString Uri_;

        QString FullFileLocalPath_;

        bool LeftButtonPressed_;

        bool CopyFile_;

        QString SaveAs_;

        QPoint OpenPreviewer_;

        int64_t FullImageDownloadSeq_;

        int64_t PreviewImageDownloadSeq_;

    Q_SIGNALS:
        void stateChanged();
	};

}