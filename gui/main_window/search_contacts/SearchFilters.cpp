#include "stdafx.h"
#include "SearchFilters.h"
#include "ComboButton.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../controls/CustomButton.h"
#include "../../controls/LineEditEx.h"

namespace Ui
{
    SearchFilters::SearchFilters(QWidget* _parent)
        :	QWidget(_parent)
        , keyword_(new LineEditEx(this))
        , search_button_(new QPushButton(this))

    {

        QVBoxLayout* root_layout = new QVBoxLayout();
        root_layout->setContentsMargins(0, Utils::scale_value(16), 0, 0);
        root_layout->setSpacing(0);

        QHBoxLayout* keyword_layout = new QHBoxLayout();
        keyword_layout->setContentsMargins(0, 0, 0, 0);
        keyword_layout->setSpacing(0);

        keyword_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Phone or Name, Email, UIN"));
        keyword_->setAttribute(Qt::WA_MacShowFocusRect, false);
        keyword_->setObjectName("keyword");
        keyword_layout->addWidget(keyword_);

		Utils::ApplyStyle(search_button_, main_button_style);
		search_button_->setText(QT_TRANSLATE_NOOP("search_widget", "Search"));
        search_button_->setObjectName("search_button");
        search_button_->setCursor(QCursor(Qt::PointingHandCursor));

        keyword_layout->addWidget(search_button_);

        root_layout->addLayout(keyword_layout);

        /*	QHBoxLayout* filters_layout = new QHBoxLayout();
        filters_layout->setAlignment(Qt::AlignLeft);
        filters_layout->setContentsMargins(0, Utils::scale_value(16), 0, 0);

        ComboButton* button_gender = new ComboButton(this);
        button_gender->setText(TR_("SearchFilters", "Gender"));
        filters_layout->addWidget(button_gender);

        ComboButton* button_country = new ComboButton(this);
        button_country->setText(TR_("SearchFilters", "Country"));
        filters_layout->addWidget(button_country);

        ComboButton* button_age = new ComboButton(this);
        button_age->setText(TR_("SearchFilters", "Age"));
        filters_layout->addWidget(button_age);

        QCheckBox* online_check = new QCheckBox(TR_("SearchFilters", "Online"), this);
        online_check->setObjectName("online_check");
        filters_layout->addWidget(online_check);

        root_layout->addLayout(filters_layout);*/

        setLayout(root_layout);


        connect(search_button_, SIGNAL(clicked()), this, SLOT(onSearchButtonClicked()), Qt::QueuedConnection);
        connect(keyword_, SIGNAL(enter()), this, SLOT(onSearchButtonClicked()), Qt::QueuedConnection);
    }

    SearchFilters::~SearchFilters()
    {
    }

    void SearchFilters::on_search_results()
    {
        search_button_->setEnabled(true);
    }

    void SearchFilters::on_focus()
    {
        keyword_->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void SearchFilters::onSearchButtonClicked()
    {
        search_button_->setEnabled(false);

        search_params filters;
        filters.set_keyword(keyword_->text().trimmed());

        emit onSearch(filters);
    }

    void SearchFilters::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
