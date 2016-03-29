#include "stdafx.h"
#include "SearchWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/CustomButton.h"
#include "../../utils/utils.h"

namespace Ui
{
	SearchWidget::SearchWidget(int _offset, bool _isWithButton, QWidget* _parent)
        : QWidget(_parent)
	{
        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("search_widget"));
        active_ = false;
        parent_search_vertical_layout_ = new QVBoxLayout(this);
        parent_search_vertical_layout_->setContentsMargins(0, 0, 0, 0);
        parent_search_vertical_layout_->setSpacing(0);
        horizontal_search_layout_ = new QHBoxLayout();
        horizontal_search_layout_->setContentsMargins(0, 0, 0, Utils::scale_value(0));
        parent_search_vertical_layout_->addLayout(horizontal_search_layout_);

        horizontal_search_layout_->addSpacing(Utils::scale_value(_offset - 8));

        search_icon_ = new Ui::CustomButton(this, ":/resources/contr_search_100.png");
        search_icon_->setOffsets(Utils::scale_value(8), Utils::scale_value(0));
        search_icon_->setActiveImage(":/resources/contr_search_100_active.png");
        search_icon_->setFixedWidth(Utils::scale_value(34));
        search_icon_->setFixedHeight(Utils::scale_value(53));
        search_icon_->setStyleSheet("border: none;");
        horizontal_search_layout_->addWidget(search_icon_);

        parent_widget_ = new QWidget(this);
        search_vertical_layout_ = new QVBoxLayout(parent_widget_);
        search_vertical_layout_->setContentsMargins(0, Utils::scale_value(5), 0, 0);
        search_edit_ = new LineEditEx(this);
        search_edit_->setProperty("SearchEdit", true);
        search_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Search"));
        search_edit_->setContentsMargins(Utils::scale_value(6), 0, Utils::scale_value(22), 0);
        search_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        search_edit_->setStyleSheet(QString("background-color: transparent"));
        Testing::setAccessibleName(search_edit_, "search_edit");
        search_vertical_layout_->addSpacing(Utils::scale_value(0));
        search_vertical_layout_->addWidget(search_edit_);
        horizontal_search_layout_->addWidget(parent_widget_);
        search_edit_->setContextMenuPolicy(Qt::NoContextMenu);

        search_edit_icon_ = new CustomButton(this, ":/resources/contr_compose_100.png");
        search_edit_icon_->setOffsets(Utils::scale_value(0), Utils::scale_value(0));
		search_edit_icon_->setOffsetsForActive(Utils::scale_value(2), Utils::scale_value(0));
        search_edit_icon_->setHoverImage(":/resources/contr_compose_100_hover.png");
        search_edit_icon_->setActiveImage(":/resources/contr_clear_100.png");
        search_edit_icon_->setFixedWidth(Utils::scale_value(50));
        search_edit_icon_->setFixedHeight(Utils::scale_value(53));
		search_edit_icon_->setCursor(Qt::PointingHandCursor);
		search_edit_icon_->setFocusPolicy(Qt::NoFocus);
        search_edit_icon_->setStyleSheet("background-color: transparent;");
        Testing::setAccessibleName(search_edit_icon_, "CreateGroupChat");
		horizontal_search_layout_->addWidget(search_edit_icon_);

        horizontal_line_widget_ = new QWidget(this);
        horizontal_line_widget_->setFixedHeight(Utils::scale_value(1));
        horizontal_line_widget_->setStyleSheet(QString("background-color: #dadada;"));
        parent_search_vertical_layout_->addSpacing(0);
        widget_2_ = new QWidget(this);
        horizontal_layout_2_ = new QHBoxLayout(widget_2_);
        horizontal_layout_2_->setContentsMargins(Utils::scale_value(_isWithButton ? 24 : 16), 0, Utils::scale_value(_isWithButton ? 24 : 60), Utils::scale_value(10));
        horizontal_layout_2_->addWidget(horizontal_line_widget_);
        parent_search_vertical_layout_->addWidget(widget_2_);

        QMetaObject::connectSlotsByName(this);
        connect(search_edit_, SIGNAL(textEdited(QString)), this, SLOT(searchChanged(QString)), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(clicked()), this, SLOT(searchStarted()), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(escapePressed()), this, SLOT(searchCompleted()), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(enter()), this, SLOT(editEnterPressed()), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(upArrow()), this, SLOT(editUpPressed()), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(downArrow()), this, SLOT(editDownPressed()), Qt::QueuedConnection);
        connect(search_edit_, SIGNAL(focusOut()), this, SLOT(focusedOut()), Qt::QueuedConnection);
        connect(search_edit_icon_, SIGNAL(clicked()), this, SLOT(clearPressed()), Qt::QueuedConnection);

		setActive(false);
	}

    void SearchWidget::retranslateUi(QWidget *search_widget)
	{
		search_widget->setWindowTitle("");
        search_icon_->setText(QString());
        search_edit_icon_->setText(QString());
    }

	void SearchWidget::SetShowButton(bool _isShow)
	{
		is_show_button_= _isShow;
		search_edit_icon_->setVisible(is_show_button_);
	}

    void SearchWidget::setFocus()
    {
        search_edit_->setFocus(Qt::MouseFocusReason);
        setActive(true);
    }

	SearchWidget::~SearchWidget()
	{
	}

	void SearchWidget::setActive(bool active)
	{
		active_ = active;
        search_icon_->setActive(active);
        search_edit_icon_->setActive(active);
        if (active)
        {
            horizontal_line_widget_->setStyleSheet(QString("background-color: #579e1c;"));
        }
        else
        {
            horizontal_line_widget_->setStyleSheet(QString("background-color: #dadada;"));
        }
	}

    void SearchWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
		painter.setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        painter.fillRect(rect(), QColor(255, 255, 255, 0.95 * 255));
		painter.drawLine(geometry().width()-1, geometry().y(), geometry().width()-1, geometry().height());
    }

	void SearchWidget::searchStarted()
	{
		setActive(true);
	}

	void SearchWidget::ClearInput()
	{
		search_edit_->clear();
        search_edit_->clearFocus();
	}

	void SearchWidget::searchCompleted()
	{
		ClearInput();
		setActive(false);
		emit searchEnd();
	}

	void SearchWidget::searchChanged(QString text)
	{
        setActive(true);

        if (!text.isEmpty())
        {
            emit searchBegin();
        }
        else
        {
            searchEnd();
        }

		emit search(text);
	}

	void SearchWidget::clearPressed()
	{
		if (active_)
        {
			searchCompleted();
        }
        else
        {
            emit nonActiveButtonPressed();
        }
    }

	void SearchWidget::editEnterPressed()
	{
		emit enterPressed();
	}

	void SearchWidget::editUpPressed()
	{
		emit upPressed();
	}

	void SearchWidget::editDownPressed()
	{
		emit downPressed();
	}

	void SearchWidget::focusedOut()
	{
        setActive(false);
	}
}