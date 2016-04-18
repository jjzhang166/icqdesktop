#include "stdafx.h"
#include "ContextMenu.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace Ui
{
    int MenuStyle::pixelMetric(PixelMetric metric, const QStyleOption * option, const QWidget * widget) const
    {
        int s = QProxyStyle::pixelMetric(metric, option, widget);
        if (metric == QStyle::PM_SmallIconSize) {
            s = Utils::scale_value(20);
        }
        return s;
    }

    ContextMenu::ContextMenu(QWidget* parent)
        : QMenu(parent)
        , InvertRight_(false)
        , Indent_(0)
    {
        setWindowFlags(windowFlags() | Qt::NoDropShadowWindowHint);
        setStyle(new MenuStyle());
		setStyleSheet(QString("QMenu {\
                       background-color: #f2f2f2;\
                       border: 1px solid #cccccc;\
                       }\
                       QMenu::item {\
                       padding-left: %1px;\
                       background-color: transparent;\
                       color: #000000;\
                       padding-top: %4px;\
                       padding-bottom: %4px;\
                       padding-right: %2px;\
                       }\
                       QMenu::item:selected\
                       {\
                       background-color: #e2e2e2;\
                       padding-left: %1px;\
                       padding-top: %4px;\
                       padding-bottom: %4px;\
                       padding-right: %2px;\
                       }\
                       QMenu::icon\
                       {\
                       padding-left:%3px;\
                       }").arg(Utils::scale_value(40)).arg(Utils::scale_value(12)).arg(Utils::scale_value(22)).arg(Utils::scale_value(6)));

        QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16));
        setFont(font);
        if (platform::is_apple())
        {
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupWindow()), this, SLOT(close()));
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& icon, const QString& name, const QVariant& data)
    {
        QAction* action = addAction(icon, name);
        action->setData(data);
        return action;
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& icon, const QString& name, const QObject *receiver, const char* member)
    {
        return addAction(icon, name, receiver, member);
    }

    bool ContextMenu::hasAction(const QString& command)
    {
        assert(!command.isEmpty());

        for (const auto action : actions())
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find("command");
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == command)
            {
                return true;
            }
        }

        return false;
    }

    void ContextMenu::removeAction(const QString& command)
    {
        assert(!command.isEmpty());

        for (auto action : actions())
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find("command");
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == command)
            {
                QWidget::removeAction(action);

                return;
            }
        }
    }

    void ContextMenu::invertRight(bool invert)
    {
        InvertRight_ = invert;
    }

    void ContextMenu::setIndent(int indent)
    {
        Indent_ = indent;
    }

	void ContextMenu::popup(const QPoint &pos, QAction *at)
	{
		Pos_ = pos;
		QMenu::popup(pos, at);
	}

    void ContextMenu::clear()
    {
        QMenu::clear();
    }

    void ContextMenu::hideEvent(QHideEvent *e)
    {
        QMenu::hideEvent(e);
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
		if (InvertRight_ || Indent_ != 0)
		{
			QPoint p;
			if (pos().x() != Pos_.x())
				p = Pos_;
			else
				p = pos();

			if (InvertRight_)
				p.setX(p.x() - width() - Indent_);
			else
				p.setX(p.x() + Indent_);
			move(p);
		}
    }
}
