#pragma once


namespace Ui
{
	class new_messages_plate;

	class NewMessagesPlate : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void downPressed();

	public:
		NewMessagesPlate(QWidget* parent);
		~NewMessagesPlate();

		void setWidth(int width);
		void setUnreadCount(int count);
		void addUnread(int _count = 1);
        void updateStyle();
        void setContact(const QString& _aimId);

	protected:
		bool eventFilter(QObject* obj, QEvent* event);
        QString aimId_;

	private:
		int unreads_;
        QHBoxLayout *horizontal_layout_;
        QSpacerItem *horizontal_spacer_;
        QWidget *widget_;
        QVBoxLayout *vertical_layout_;
        QSpacerItem *vertical_spacer_;
        QLabel *message_;
        QSpacerItem *vertical_spacer_2_;
        QSpacerItem *horizontal_spacer_2_;
	};
}