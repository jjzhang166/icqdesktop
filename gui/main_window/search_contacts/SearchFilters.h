#pragma once

#include "search_params.h"

namespace Ui
{
    class LineEditEx;

    class SearchFilters : public QWidget
    {
        Q_OBJECT

            LineEditEx*	keyword_;
        QPushButton*	search_button_;

Q_SIGNALS:

        void onSearch(search_params _filters);

        private Q_SLOTS:

            void onSearchButtonClicked();

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        void on_focus();
        void on_search_results();

        SearchFilters(QWidget* _parent);
        virtual ~SearchFilters();
    };


}

