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

    protected:
        virtual void initialize() override;

        virtual bool isPlaceholderVisible() const override;

        virtual bool isPreloaderVisible() const override;

        virtual void leaveEvent(QEvent *e) override;

        virtual void mouseMoveEvent(QMouseEvent *e) override;

        virtual void mousePressEvent(QMouseEvent *e) override;

        virtual void mouseReleaseEvent(QMouseEvent *e) override;

    private Q_SLOTS:
        void imageDownloaded(qint64, QString, QPixmap, QString);

	private:
        enum class State;

        static bool isLeftButtonClick(QMouseEvent *e);

        void connectCoreSignals(const bool isConnected);

        bool isOverPicture(const QPoint &p) const;

        State State_;

        const QString Uri_;

        QString Local_;

        bool LeftButtonPressed_;

        bool CopyFile_;
        
    Q_SIGNALS:
        void stateChanged();
	};

}