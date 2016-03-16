#include "stdafx.h"
#include "SemitransparentWindow.h"
#include "../gui_settings.h"
#include "../utils/InterConnector.h"
#include "../main_window/contact_list/Common.h"

namespace Ui
{
    SemitransparentWindow::SemitransparentWindow(qt_gui_settings* _qui_settings, QWidget* _parent)
        : QWidget(_parent)
#ifdef __linux__
        , Is_Show_(false)
#endif //__linux__
    {
#ifdef __linux__
        setAttribute(Qt::WA_TranslucentBackground);
#endif //__linux__
        if (platform::is_apple())
        {
            setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        }
        else
        {
            setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        }
        setPalette(QPalette(Qt::black));
        setShow(true);

        const auto rect = Utils::GetMainRect();
        auto width = ::ContactList::ItemLength(true, 1, ::ContactList::dip(0)).px();
        auto height = ::ContactList::ItemLength(false, 1, ::ContactList::dip(0)).px();
        
        int appendix = 0;
        if (platform::is_apple())
        {
            if (auto mainWindow = (QMainWindow *)Utils::InterConnector::instance().getMainWindow())
            {
                QStyleOptionTitleBar options;
                options.initFrom(mainWindow);
                appendix = mainWindow->style()->pixelMetric(QStyle::PM_TitleBarHeight, &options, mainWindow);
            }
        }
        resize(width, height + appendix);
        move(rect.x(), rect.y() - appendix);

        show();
    }

    SemitransparentWindow::~SemitransparentWindow()
    {
    }

#ifdef __linux
    void SemitransparentWindow::paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(rect(), QBrush(QColor(0x0, 0x0, 0x0, Is_Show_ ? (int32_t)(0.6 * 255) : 0)));
    }

#endif //__linux__

    void SemitransparentWindow::setShow(bool _is_show)
    {
#ifdef __linux__
        Is_Show_ = _is_show;
        update();
#else
        setWindowOpacity(_is_show ? 0.4 : 0);
#endif //__linux__
    }
    
    void SemitransparentWindow::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        emit clicked();
    }
    
}
