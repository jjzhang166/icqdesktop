#include "stdafx.h"
#include "SemitransparentWindow.h"
#include "../gui_settings.h"
#include "../utils/InterConnector.h"
#include "../main_window/contact_list/Common.h"

namespace
{
    int instancesCounter = 0;
    bool touchSwallowed = false;
}

namespace Ui
{
    SemitransparentWindow::SemitransparentWindow(QWidget* _parent)
        : QWidget(_parent), main_(instancesCounter == 0)
    {
        ++instancesCounter;

        const auto rect = _parent->rect();
        auto width = rect.width();
        auto height = rect.height();

        setFixedHeight(height);
        setFixedWidth(width);

        move(0, 0);
        show();
    }

    SemitransparentWindow::~SemitransparentWindow()
    {
        --instancesCounter;
        if (instancesCounter == 0)
            touchSwallowed = false;
    }

    void SemitransparentWindow::paintEvent(QPaintEvent* _e)
    {
        if (main_)
        {
            QStyleOption opt;
            opt.initFrom(this);
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        }
        return QWidget::paintEvent(_e);
    }

    void SemitransparentWindow::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        if (!touchSwallowed)
        {
            touchSwallowed = true;
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        }
    }

}
