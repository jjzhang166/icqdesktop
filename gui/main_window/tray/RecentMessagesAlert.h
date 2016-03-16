#pragma once

#include "../../types/message.h"

class QPainter;

namespace Logic
{
	class RecentItemDelegate;
}

namespace Ui
{
	class RecentMessagesAlert : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void messageClicked(QString);

	public:
		RecentMessagesAlert(Logic::RecentItemDelegate* delegate);
		~RecentMessagesAlert();

		void addAlert(const Data::DlgState& state);
		void markShowed();

	protected:
		virtual void enterEvent(QEvent*);
		virtual void leaveEvent(QEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);

	private:
		void init();

	private Q_SLOTS:
		void closeAlert();
        void statsCloseAlert();
		void viewAll();
		void startAnimation();
        void messageAlertClicked(QString);

	private:
		Logic::RecentItemDelegate* Delegate_;
		QVBoxLayout* Layout_;
		unsigned AlertsCount_;
		QPushButton* CloseButton_;
		QWidget* ViewAllWidget_;
		QTimer* Timer_;
		QPropertyAnimation* Animation_;
		int Height_;
		bool CursorIn_;
		bool ViewAllWidgetVisible_;
	};
}