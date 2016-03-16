#pragma once


namespace Ui
{
	class LineEditEx : public QLineEdit
	{
		Q_OBJECT
Q_SIGNALS:
		void focusIn();
		void focusOut();
		void clicked();
		void emptyTextBackspace();
		void escapePressed();
		void upArrow();
		void downArrow();
		void enter();

	public:
		LineEditEx(QWidget* parent);

	protected:
		virtual void focusInEvent(QFocusEvent*);
		virtual void focusOutEvent(QFocusEvent*);
		virtual void mousePressEvent(QMouseEvent*);
		virtual void keyPressEvent(QKeyEvent*);
	};
}