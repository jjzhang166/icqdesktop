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

    private Q_SLOTS:
        void avatarChanged(QString);

	public:

		ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _display_name, int _size, bool _autoUpdate = false);
        ~ContactAvatarWidget();

        void UpdateParams(const QString& _aimid, const QString& _display_name);
	};
}
