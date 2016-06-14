#pragma once

namespace Ui
{
	class ContactAvatarWidget : public QPushButton
	{
		Q_OBJECT

	protected:

		const int	size_;
		
		QString		aimid_;
		QString		display_name_;

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
        void frameChanged(int frame);

        void selectFileForAvatar();
        void cropAvatar();
        void openAvatarPreview();

        void avatarEnter();
        void avatarLeave();
        void setAvatar(int _error);

	public:

		ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _display_name, int _size, bool _autoUpdate);
        ~ContactAvatarWidget();

        void UpdateParams(const QString& _aimid, const QString& _display_name);
        void SetIsInMyProfile(bool _is_in_my_profile);
        void SetVisibleShadow(bool _is_visible_shadow);
        void SetVisibleSpinner(bool _is_visible_spinner);

    private:
        QString GetState();
        void postSetAvatarToCore(const QPixmap& _avatar);
        void ResetInfoForSetAvatar();

    private:
        bool is_in_my_profile_;
        bool is_visible_shadow_;
	    bool is_visible_spinner_;
        bool connected_;
        QMovie* spinner_movie_;
        InfoForSetAvatar infoForSetAvatar_;
    };
}
