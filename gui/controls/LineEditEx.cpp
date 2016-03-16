#include "stdafx.h"
#include "LineEditEx.h"


namespace Ui
{
	LineEditEx::LineEditEx(QWidget* parent)
		: QLineEdit(parent)
	{

	}

	void LineEditEx::focusInEvent(QFocusEvent* event)
	{
		emit focusIn();
		QLineEdit::focusInEvent(event);
	}

	void LineEditEx::focusOutEvent(QFocusEvent* event)
	{
		emit focusOut();
		QLineEdit::focusOutEvent(event);
	}

	void LineEditEx::mousePressEvent(QMouseEvent* event)
	{
		emit clicked();
		QLineEdit::mousePressEvent(event);
	}

	void LineEditEx::keyPressEvent(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_Backspace && text().isEmpty())
			emit emptyTextBackspace();

		if (event->key() == Qt::Key_Escape)
        {
			emit escapePressed();
            return;
        }

		if (event->key() == Qt::Key_Up)
			emit upArrow();

		if (event->key() == Qt::Key_Down)
			emit downArrow();

		if (event->key() == Qt::Key_Return && event->modifiers() == Qt::NoModifier)
			emit enter();

		QLineEdit::keyPressEvent(event);
	}

}