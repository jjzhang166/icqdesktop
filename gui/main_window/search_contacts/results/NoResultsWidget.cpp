#include "stdafx.h"
#include "NoResultsWidget.h"

namespace Ui
{
	NoResultsWidget::NoResultsWidget(QWidget* _parent)
		:	QWidget(_parent), 
			rootLayout_(new QVBoxLayout())
	{
		QLabel* text = new QLabel(this);
		text->setObjectName("no_result_label");
		text->setText(QT_TRANSLATE_NOOP("search_widget", "Nothing found"));
		text->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        rootLayout_->addWidget(text);

		setLayout(rootLayout_);
	}


	NoResultsWidget::~NoResultsWidget(void)
	{
	}

	void NoResultsWidget::paintEvent(QPaintEvent* _e)
	{
		QStyleOption opt;
		opt.init(this);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

		return QWidget::paintEvent(_e);
	}
}

