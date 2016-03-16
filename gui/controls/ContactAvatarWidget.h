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

	public:

		ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _display_name, int _size);
        ~ContactAvatarWidget();
	};
}
