#pragma once

namespace Ui
{
	class ContactAvatarWidget : public QPushButton
	{
		Q_OBJECT

	protected:

		const int	size_;
		
		QString		aimid_;
		QString		displayName_;

		virtual void paintEvent(QPaintEvent* _e) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void enterEvent(QEvent*) override;
        virtual void leaveEvent(QEvent*) override;

    private:
        struct InfoForSetAvatar
        {
            QString currentDirectory;
            QImage image;
            QRectF croppingRect;
            QPixmap croppedImage;
        };

    Q_SIGNALS:
        void clicked();
        void mouseEntered();
        void mouseLeft();
        void afterAvatarChanged();

    private Q_SLOTS:
        void avatarChanged(QString);
        void frameChanged(int _frame);

        void selectFileForAvatar();
        void cropAvatar();
        void openAvatarPreview();

        void avatarEnter();
        void avatarLeave();
        void setAvatar(qint64 _seq, int _error);

	public:

		ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _displayName, int _size, bool _autoUpdate);
        ~ContactAvatarWidget();

        void UpdateParams(const QString& _aimid, const QString& _displayName);
        void SetIsInMyProfile(bool _isInMyProfile);
        void SetVisibleShadow(bool _isVisibleShadow);
        void SetVisibleSpinner(bool _isVisibleSpinner);

    private:
        QString GetState();
        void postSetAvatarToCore(const QPixmap& _avatar);
        void ResetInfoForSetAvatar();

    private:
        bool isInMyProfile_;
        bool isVisibleShadow_;
	    bool isVisibleSpinner_;
        bool connected_;
        QMovie* spinnerMovie_;
        InfoForSetAvatar infoForSetAvatar_;
        qint64 seq_;
    };
}
