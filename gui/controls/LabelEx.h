#pragma once

namespace Ui
{
	class LabelEx : public QLabel
	{
		Q_OBJECT
Q_SIGNALS:
		void clicked();

	public:
		LabelEx(QWidget* parent);

	protected:
		virtual void mouseReleaseEvent(QMouseEvent*);
	};
}