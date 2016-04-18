#include "stdafx.h"
#include "SemitransparentWindow.h"
#include "../gui_settings.h"
#include "../utils/InterConnector.h"
#include "../main_window/contact_list/Common.h"

namespace Ui
{
    SemitransparentWindow::SemitransparentWindow(QWidget* _parent)
        : QWidget(_parent)
    {
        const auto rect = Utils::GetMainRect();
        auto width = rect.width();
        auto height = rect.height();

        setFixedHeight(height);
        setFixedWidth(width);

        move(0, 0);
        show();
    }

    SemitransparentWindow::~SemitransparentWindow()
    {
    }

    void SemitransparentWindow::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        return QWidget::paintEvent(_e);
    }

    void SemitransparentWindow::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        emit clicked();
    }
    
}
