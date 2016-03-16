#include "stdafx.h"
#include "NewMessagesPlate.h"
#include "../../utils/utils.h"
#include "../../../corelib/enumerations.h"
#include "../../core_dispatcher.h"
#include "../../theme_settings.h"

namespace Ui
{
	NewMessagesPlate::NewMessagesPlate(QWidget* parent)
		: QWidget(parent)
		, unreads_(0)
	{
        aimId_ = "";
		setStyleSheet(Utils::LoadStyle(":/main_window/history_control/history_control.qss", Utils::get_scale_coefficient(), true));
        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("new_messages_plate"));
        this->resize(796, 437);
        this->setProperty("NewMessages", QVariant(true));
        horizontal_layout_ = new QHBoxLayout(this);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        horizontal_spacer_ = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        
        horizontal_layout_->addItem(horizontal_spacer_);
        
        widget_ = new QWidget(this);
        widget_->setObjectName(QStringLiteral("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget_->sizePolicy().hasHeightForWidth());
        widget_->setSizePolicy(sizePolicy);
        widget_->setMouseTracking(true);
        widget_->setProperty("NewMessagesWidget", QVariant(true));
        vertical_layout_ = new QVBoxLayout(widget_);
        vertical_layout_->setSpacing(0);
        vertical_layout_->setObjectName(QStringLiteral("verticalLayout"));
        vertical_layout_->setContentsMargins(0, 0, 0, 0);
        vertical_spacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        
        vertical_layout_->addItem(vertical_spacer_);
        
        message_ = new QLabel(widget_);
        message_->setObjectName(QStringLiteral("message"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(message_->sizePolicy().hasHeightForWidth());
        message_->setSizePolicy(sizePolicy1);
        message_->setMouseTracking(true);
        message_->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        message_->setWordWrap(false);
        message_->setProperty("NewMessagesLabel", QVariant(true));
        
        vertical_layout_->addWidget(message_);
        
        vertical_spacer_2_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        
        vertical_layout_->addItem(vertical_spacer_2_);
        
        
        horizontal_layout_->addWidget(widget_);
        
        horizontal_spacer_2_ = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        
        horizontal_layout_->addItem(horizontal_spacer_2_);
        
        message_->setText(QString());
        
        QMetaObject::connectSlotsByName(this);
        
        
		setMouseTracking(true);
		widget_->installEventFilter(this);
		widget_->setCursor(Qt::PointingHandCursor);
	}

	NewMessagesPlate::~NewMessagesPlate()
	{
	}

	bool NewMessagesPlate::eventFilter(QObject* obj, QEvent* event)
	{
		if (qobject_cast<QWidget*>(obj) == widget_)
		{
			if (event->type() == QEvent::Enter)
			{
				widget_->setProperty("NewMessagesWidget", false);
				widget_->setProperty("NewMessagesWidgetHover", true);
				widget_->setProperty("NewMessagesWidgetPressed", false);
				widget_->setStyle(QApplication::style());
				message_->setProperty("NewMessagesLabel", false);
				message_->setProperty("NewMessagesLabelHover", true);
				message_->setStyle(QApplication::style());
				return true;
			}
			else if (event->type() == QEvent::Leave)
			{
				widget_->setProperty("NewMessagesWidget", true);
				widget_->setProperty("NewMessagesWidgetHover", false);
				widget_->setProperty("NewMessagesWidgetPressed", false);
				widget_->setStyle(QApplication::style());
				message_->setProperty("NewMessagesLabel", true);
				message_->setProperty("NewMessagesLabelHover", false);
				message_->setStyle(QApplication::style());
				return true;
			}
			else if (event->type() == QEvent::MouseButtonPress)
			{
				widget_->setProperty("NewMessagesWidget", false);
				widget_->setProperty("NewMessagesWidgetHover", false);
				widget_->setProperty("NewMessagesWidgetPressed", true);
				widget_->setStyle(QApplication::style());
				message_->setProperty("NewMessagesLabel", false);
				message_->setProperty("NewMessagesLabelHover", true);
				message_->setStyle(QApplication::style());
				return true;
			}
			if (event->type() == QEvent::MouseButtonRelease)
			{
				widget_->setProperty("NewMessagesWidget", false);
				widget_->setProperty("NewMessagesWidgetHover", true);
				widget_->setProperty("NewMessagesWidgetPressed", false);
				widget_->setStyle(QApplication::style());
				message_->setProperty("NewMessagesLabel", false);
				message_->setProperty("NewMessagesLabelHover", true);
				message_->setStyle(QApplication::style());
				emit downPressed();
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_new_messages_botton);
				return true;
			}
		}

		return QWidget::eventFilter(obj, event);
	}
    
    void NewMessagesPlate::updateStyle()
    {
        auto curTheme = get_qt_theme_settings()->themeForContact(aimId_);
        QColor backgroundColor = curTheme->new_messages_bubble_.bg_color_;
        QColor backgroundHoverColor = curTheme->new_messages_bubble_.bg_hover_color_;
        QColor backgroundPressedColor = curTheme->new_messages_bubble_.bg_pressed_color_;
        QColor textColor = curTheme->new_messages_bubble_.text_color_;

        QString bgColorString = Utils::rgbaStringFromColor(backgroundColor);
        QString bgHoverColorString = Utils::rgbaStringFromColor(backgroundHoverColor);
        QString bgPressedColorString = Utils::rgbaStringFromColor(backgroundPressedColor);
        QString textColorString = Utils::rgbaStringFromColor(textColor);
        
        QString widgetStyleSheet = QString("QWidget[NewMessagesWidget=\"true\"] { background: %1; } QWidget[NewMessagesWidgetHover=\"true\"] { background: %2; } QWidget[NewMessagesWidgetPressed=\"true\"] { background: %3; }").arg(bgColorString).arg(bgHoverColorString).arg(bgPressedColorString);
        
        QString messageStyleSheet = QString("QLabel[NewMessagesLabel=\"true\"] { color: %1; } QLabel[NewMessagesLabelHover=\"true\"] { color: %2; }").arg(textColorString).arg(textColorString);
        
        widget_->setStyleSheet(widgetStyleSheet);
        message_->setStyleSheet(messageStyleSheet);
    }
	
	void NewMessagesPlate::setUnreadCount(int _count)
	{
		unreads_ = _count;
		QString message = QString("%1 " + Utils::GetTranslator()->getNumberString(_count, QT_TRANSLATE_NOOP3("chat_page", "new message", "1"), QT_TRANSLATE_NOOP3("chat_page", "new messages", "2"),
			QT_TRANSLATE_NOOP3("chat_page", "new messages", "5"), QT_TRANSLATE_NOOP3("chat_page", "new messages", "21"))).arg(_count);
		message_->setText(message);
		setMinimumHeight(message_->height());
	}
    
    void NewMessagesPlate::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }

	void NewMessagesPlate::addUnread(int _count)
	{
		unreads_ += _count;
		setUnreadCount(unreads_);
	}
	
	void NewMessagesPlate::setWidth(int _width)
	{
		resize(_width, height());
	}
}