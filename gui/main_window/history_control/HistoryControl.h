#pragma once

#include "../../types/message.h"

namespace Ui
{
	class history_control;
	class HistoryControlPage;

	class HistoryControl : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
		void quote(QString);

	public Q_SLOTS:
		void contactSelected(QString _aimId);

	private Q_SLOTS:
		void updatePages();
		void close_dialog(const QString& _aimId);

	public:
		HistoryControl(QWidget* parent);
		~HistoryControl();
        void cancelSelection();
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        const QString & getCurrent();
        
	private:
		QMap<QString, HistoryControlPage*> pages_;
		QMap<QString, QTime> times_;
		QString current_;
		QTimer* timer_;
        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QStackedWidget *stacked_widget_;
        QWidget* page_;
	};
}