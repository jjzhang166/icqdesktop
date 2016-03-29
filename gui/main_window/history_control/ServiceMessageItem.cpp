#include "stdafx.h"
#include "ServiceMessageItem.h"
#include "../../utils/utils.h"
#include "../../cache/themes/themes.h"

namespace Ui
{
	ServiceMessageItem::ServiceMessageItem(QWidget* parent, bool overlay)
		: HistoryControlPageItem(parent)
		, new_(false)
		, overlay_(overlay)
	{
        aimId_ = "";
		setStyleSheet(Utils::LoadStyle(":/main_window/history_control/history_control.qss", Utils::get_scale_coefficient(), true));
        if (this->objectName().isEmpty())
            this->setObjectName(QStringLiteral("service_message_item"));
        this->resize(92, 16);
        this->setProperty("ServiceMessage", QVariant(true));
        horizontal_layout_ = new QHBoxLayout(this);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        left_widget_ = new QWidget(this);
        left_widget_->setObjectName(QStringLiteral("left_widget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(left_widget_->sizePolicy().hasHeightForWidth());
        left_widget_->setSizePolicy(sizePolicy);
        left_widget_->setProperty("ServiceMessageCommonWidget", QVariant(true));
        horizontal_layout_4_ = new QHBoxLayout(left_widget_);
        horizontal_layout_4_->setSpacing(0);
        horizontal_layout_4_->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontal_layout_4_->setContentsMargins(0, 0, 0, 0);
        horizontal_layout_->addWidget(left_widget_);
        widget_ = new QWidget(this);
        widget_->setObjectName(QStringLiteral("widget"));
        sizePolicy.setHeightForWidth(widget_->sizePolicy().hasHeightForWidth());
        widget_->setSizePolicy(sizePolicy);
        widget_->setProperty("DateWidget", QVariant(true));
        horizontal_layout_2_ = new QHBoxLayout(widget_);
        horizontal_layout_2_->setSpacing(0);
        horizontal_layout_2_->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontal_layout_2_->setContentsMargins(0, 0, 0, 0);
        message_ = new QLabel(widget_);
        message_->setObjectName(QStringLiteral("message"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(message_->sizePolicy().hasHeightForWidth());
        message_->setSizePolicy(sizePolicy1);
        message_->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
        if (platform::is_apple())
        {
            message_->setContentsMargins(0, 1, 0, 0);
        }
        message_->setWordWrap(false);
        message_->setProperty("DateWidgetLabel", QVariant(true));
        horizontal_layout_2_->addWidget(message_);

        horizontal_layout_->addWidget(widget_);

        right_widget_ = new QWidget(this);
        right_widget_->setObjectName(QStringLiteral("right_widget"));
        sizePolicy.setHeightForWidth(right_widget_->sizePolicy().hasHeightForWidth());
        right_widget_->setSizePolicy(sizePolicy);
        right_widget_->setProperty("ServiceMessageCommonWidget", QVariant(true));
        horizontal_layout_3_ = new QHBoxLayout(right_widget_);
        horizontal_layout_3_->setSpacing(0);
        horizontal_layout_3_->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontal_layout_3_->setContentsMargins(0, 0, 0, 0);

        horizontal_layout_->addWidget(right_widget_);

        horizontal_layout_->setStretch(0, 1);
        horizontal_layout_->setStretch(2, 1);

        message_->setText(QString());

        QMetaObject::connectSlotsByName(this);
        
		Utils::grabTouchWidget(this);
		left_widget_->setAttribute(Qt::WA_TransparentForMouseEvents);
		widget_->setAttribute(Qt::WA_TransparentForMouseEvents);
		right_widget_->setAttribute(Qt::WA_TransparentForMouseEvents);
		setDate(QDate());
		if (overlay)
		{
			setProperty("ServiceMessageOverlay", true);
			setProperty("ServiceMessage", false);
			setStyle(QApplication::style());
			widget_->setProperty("ServiceMessageWidgetOverlay", true);
			widget_->setProperty("DateWidget", false);
			widget_->setStyle(QApplication::style());
			message_->setProperty("ServiceMessageLabelOverlay", true);
			message_->setProperty("DateWidgetLabel", false);
			message_->setStyle(QApplication::style());
		}
	}
    
    void ServiceMessageItem::updateStyle()
    {
        auto curTheme = theme();
        QColor backgroundColor = curTheme->date_.bg_color_;
        QColor textColor = curTheme->date_.text_color_;
        QColor newMessagesPlateTextColor = curTheme->new_messages_plate_.text_color_;
        QColor newMessagesPlateBgColor = curTheme->new_messages_plate_.bg_color_;
        
        QString bgColorString = Utils::rgbaStringFromColor(backgroundColor);
        QString textColorString = Utils::rgbaStringFromColor(textColor);
        QString newMessagesPlateBgColorString = Utils::rgbaStringFromColor(newMessagesPlateBgColor);
        QString newMessagesPlateTextColorString = Utils::rgbaStringFromColor(newMessagesPlateTextColor);
        
        QString widgetStyleSheet = QString("QWidget[DateWidget=\"true\"] { background: %1; } QWidget[ServiceMessageWidgetOverlay=\"true\"] { background: %2; } QWidget[NewMessagesPlateWidget=\"true\"] { background: %3 }").arg(bgColorString).arg(bgColorString).arg(newMessagesPlateBgColorString);
        QString messageStyleSheet = QString("QLabel[DateWidgetLabel=\"true\"] { color: %1; } QLabel[ServiceMessageLabelOverlay=\"true\"] { color: %2; } QLabel[NewMessagesPlateWidgetLabel=\"true\"] { color: %3; }").arg(textColorString).arg(textColorString).arg(newMessagesPlateTextColorString);
        QString leftRightStyleSheet = QString("QWidget[NewMessagesPlateCommonWidget=\"true\"] { background: %1 }").arg(newMessagesPlateBgColorString);
        
        widget_->setStyleSheet(widgetStyleSheet);
        message_->setStyleSheet(messageStyleSheet);
        left_widget_->setStyleSheet(leftRightStyleSheet);
        right_widget_->setStyleSheet(leftRightStyleSheet);
    }

	ServiceMessageItem::~ServiceMessageItem()
	{
	}

	QString ServiceMessageItem::formatRecentsText() const
	{
		return message_->text();
	}

	void ServiceMessageItem::setMessage(const QString& message)
	{
		message_->setText(message);
		message_->adjustSize();
		widget_->setMinimumWidth(message_->width());
	}

	void ServiceMessageItem::setDate(const QDate& date)
	{
		if (!overlay_)
		{
			widget_->setProperty("DateWidget", true);
			widget_->setProperty("NewMessagesPlateWidget", false);
			widget_->setStyle(QApplication::style());
			left_widget_->setProperty("ServiceMessageCommonWidget", true);
			left_widget_->setProperty("NewMessagesPlateCommonWidget", false);
			left_widget_->setStyle(QApplication::style());
			right_widget_->setProperty("ServiceMessageCommonWidget", true);
			right_widget_->setProperty("NewMessagesPlateCommonWidget", false);
			right_widget_->setStyle(QApplication::style());
			message_->setProperty("DateWidgetLabel", true);
			message_->setProperty("NewMessagesPlateWidgetLabel", false);
			message_->setStyle(QApplication::style());
		}
		int daysToNow = (int)date.daysTo(QDate::currentDate());
		QString message;
		if (daysToNow == 0)
			message = QT_TRANSLATE_NOOP("chat_page", "Today");
		else if (daysToNow == 1)
			message = QT_TRANSLATE_NOOP("chat_page", "Yesterday");
		else
			message = Utils::GetTranslator()->formatDate(date, daysToNow < 365);

		setMessage(message);
	}

	void ServiceMessageItem::setNew()
	{
		if (!overlay_)
		{
			widget_->setProperty("DateWidget", false);
			widget_->setProperty("NewMessagesPlateWidget", true);
			widget_->setStyle(QApplication::style());
			left_widget_->setProperty("ServiceMessageCommonWidget", false);
			left_widget_->setProperty("NewMessagesPlateCommonWidget", true);
			left_widget_->setStyle(QApplication::style());
			right_widget_->setProperty("ServiceMessageCommonWidget", false);
			right_widget_->setProperty("NewMessagesPlateCommonWidget", true);
			right_widget_->setStyle(QApplication::style());
			message_->setProperty("DateWidgetLabel", false);
			message_->setProperty("NewMessagesPlateWidgetLabel", true);
			message_->setStyle(QApplication::style());
		}
		new_ = true;
		setMessage(QT_TRANSLATE_NOOP("chat_page", "New messages"));
	}

	void ServiceMessageItem::setWidth(int width)
	{
		setMessage(message_->text());
		setFixedWidth(width);
	}

	bool ServiceMessageItem::isNew() const
	{
		return new_;
	}
}