#include "stdafx.h"

#include "MessageItem.h"

#include "MessagesModel.h"
#include "../history_control/FileSharingWidget.h"
#include "../../utils/utils.h"

#include "../contact_list/RecentsModel.h"
#include "../contact_list/ContactListModel.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../my_info.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/ContextMenu.h"

namespace
{
	const int avatar_width = 32;
	const int avatar_height = 32;
	const int avatar_left_margin = 24;
	const int avatar_right_margin = 6;
	const int avatar_widget_width = avatar_width + avatar_left_margin + avatar_right_margin;

	const int time_margin_left = 0;
	const int time_margin_bottom = 5;
	const int time_margin_right = 8;
	const int time_width = 30 + time_margin_right + time_margin_left;

	const int delivery_margin_left = 8;
	const int delivery_margin_right = 8;
	const int delivery_margin_bottom = 4;
	const int delivery_width = 16;
    const int delivery_height = 16;
	const int delivery_widget_width = delivery_width + delivery_margin_left + delivery_margin_right;
	const int status_widget_width = delivery_widget_width + time_width;

	const int sender_margin_bottom = 4;

	const int message_margin_left = 16;
	const int message_margin_top = 2;
	const int message_margin_bottom = 0;

	const int message_widget_right_margin = 80;
	const int message_widget_right_margin_outgoing = 24;

	const int message_widget_left_margin = 0;
	const int message_widget_left_margin_outgoing = 118 - avatar_widget_width;

	const int indent_before = 12;
	const int default_indent = 2;

	QMap<QString, QVariant> makeData(const QString& command)
	{
		QMap<QString, QVariant> result;
		result["command"] = command;
		return result;
	}
}

namespace Ui
{
    MessageItem::MessageItem()
        : HistoryControlPageItem(0)
        , Data_(new MessageData())
        , MessageBody_(nullptr)
        , MessageSender_(nullptr)
        , ContentWidget_(nullptr)
        , Menu_(0)
    {
    }

	MessageItem::MessageItem(QWidget* parent)
		: HistoryControlPageItem(parent)
		, MessageBody_(new TextEditEx(this, Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI), Utils::scale_value(15), QColor(0x28, 0x28, 0x28), false, false))
		, MessageSender_(new TextEmojiWidget(this, Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI), Utils::scale_value(12), QColor(0x57, 0x54, 0x4c)))
		, ContentWidget_(nullptr)
		, Data_(new MessageData())
		, Selected_(false)
        , Direction_(NONE)
		, Menu_(new ContextMenu(this))
    {
        QSizePolicy sizePolicyFixed(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicyFixed.setHorizontalStretch(0);
        sizePolicyFixed.setVerticalStretch(0);
        setSizePolicy(sizePolicyFixed);
        setMinimumSize(QSize(0, 0));
        setProperty("MessageWidget", QVariant(true));

        QGridLayout* gridLayout = new QGridLayout(this);
        gridLayout->setSpacing(0);
        gridLayout->setSizeConstraint(QLayout::SetFixedSize);
        gridLayout->setContentsMargins(0, 0, 0, 0);

        avatar_widget_ = new QWidget(this);
        avatar_widget_->setSizePolicy(sizePolicyFixed);
        avatar_widget_->setProperty("MessageAvatarWidget", QVariant(true));

        QVBoxLayout* verticalLayout = new QVBoxLayout(avatar_widget_);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);

        avatar_ = new QPushButton(avatar_widget_);
        avatar_->setObjectName(QStringLiteral("avatar"));
        avatar_->setEnabled(true);

        avatar_->setSizePolicy(sizePolicyFixed);
        avatar_->setProperty("MessageAvatar", QVariant(true));
		avatar_->setCursor(Qt::PointingHandCursor);

        verticalLayout->addWidget(avatar_, 0, Qt::AlignTop);
        verticalLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

        gridLayout->addWidget(avatar_widget_, 1, 0, 1, 1, Qt::AlignBottom);

        message_widget_ = new QWidget(this);
        message_widget_->setSizePolicy(sizePolicyFixed);
        message_widget_->setProperty("MessageRightWidget", QVariant(true));

        QGridLayout* gridLayout2 = new QGridLayout(message_widget_);
        gridLayout2->setSpacing(0);
        gridLayout2->setSizeConstraint(QLayout::SetFixedSize);
        gridLayout2->setContentsMargins(0, 0, 0, 0);

        sender_widget_ = new QWidget(message_widget_);
        sender_widget_->setSizePolicy(sizePolicyFixed);
        sender_widget_->setProperty("SenderWidget", QVariant(true));

        QVBoxLayout* verticalLayout2 = new QVBoxLayout(sender_widget_);
        verticalLayout2->setSpacing(0);
        verticalLayout2->setContentsMargins(0, 0, 0, 0);

        gridLayout2->addWidget(sender_widget_, 0, 0, 1, 1);

        text_widget_ = new QWidget(message_widget_);
        text_widget_->setSizePolicy(sizePolicyFixed);
        text_widget_->setProperty("MessageTextWidget", QVariant(true));

        QHBoxLayout* horizontalLayout = new QHBoxLayout(text_widget_);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setSizeConstraint(QLayout::SetFixedSize);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);

        status_widget_ = new QWidget(text_widget_);
        QSizePolicy sizePolicyFixedPrefered(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicyFixedPrefered.setHorizontalStretch(0);
        sizePolicyFixedPrefered.setVerticalStretch(0);

        status_widget_->setSizePolicy(sizePolicyFixedPrefered);
        status_widget_->setProperty("MessageStatusWidget", QVariant(true));

        QHBoxLayout* horizontalLayout2 = new QHBoxLayout(status_widget_);
        horizontalLayout2->setSpacing(0);
        horizontalLayout2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout2->addItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        delivery_widget_ = new QWidget(status_widget_);
        delivery_widget_->setSizePolicy(sizePolicyFixedPrefered);

        QVBoxLayout* verticalLayout3 = new QVBoxLayout(delivery_widget_);
        verticalLayout3->setSpacing(0);
        verticalLayout3->setContentsMargins(0, 0, 0, 0);
        verticalLayout3->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

        delivery_status_ = new PictureWidget(delivery_widget_, ":/themes/standard/100/cl/sending_mark.png");
        delivery_status_->setSizePolicy(sizePolicyFixed);

        verticalLayout3->addWidget(delivery_status_);

        horizontalLayout2->addWidget(delivery_widget_);

        time_widget_ = new QWidget(status_widget_);

        time_widget_->setObjectName(QStringLiteral("time_widget"));
        time_widget_->setSizePolicy(sizePolicyFixedPrefered);
        QVBoxLayout* verticalLayout4 = new QVBoxLayout(time_widget_);
        verticalLayout4->setSpacing(0);
        verticalLayout4->setContentsMargins(0, 0, 0, 0);
        verticalLayout4->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

        time_ = new QLabel(time_widget_);
        time_->setObjectName(QStringLiteral("time"));
        time_->setSizePolicy(sizePolicyFixed);
        time_->setAlignment(Qt::AlignBottom | Qt::AlignLeading | Qt::AlignLeft);
        time_->setProperty("MessageTime", QVariant(true));

        verticalLayout4->addWidget(time_);
        horizontalLayout2->addWidget(time_widget_);
        horizontalLayout->addWidget(status_widget_);
        gridLayout2->addWidget(text_widget_, 1, 0, 1, 1);
        gridLayout2->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 0, 1, 1);
        gridLayout->addWidget(message_widget_, 1, 1, 1, 1, Qt::AlignBottom | Qt::AlignLeft);

        message_indent_ = new QWidget(this);
        message_indent_->setSizePolicy(sizePolicyFixed);
        QHBoxLayout* horizontalLayout3 = new QHBoxLayout(message_indent_);
        horizontalLayout3->setSpacing(0);
        horizontalLayout3->setContentsMargins(0, 0, 0, 0);
        gridLayout->addWidget(message_indent_, 0, 0, 1, 1);

        this->setWindowTitle(QString());
        avatar_->setText(QString());
        time_->setText(QString());

        QMetaObject::connectSlotsByName(this);

		setAttribute(Qt::WA_AcceptTouchEvents);

		MessageSender_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		getSenderWidgetLayout()->addWidget(MessageSender_);
		Utils::setPropertyToWidget(MessageSender_, "MChatSenderLabel", true);
		MessageSender_->setStyle(QApplication::style());
		MessageSender_->hide();

		MessageBody_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		getTextWidgetLayout()->insertWidget(0, MessageBody_);
		Utils::setPropertyToWidget(MessageBody_, "MessageBody", true);
		MessageBody_->setStyle(QApplication::style());
		MessageBody_->setFrameStyle(QFrame::NoFrame);
		MessageBody_->setDocument(MessageBody_->document());
		MessageBody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		MessageBody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		MessageBody_->setOpenLinks(true);
		MessageBody_->setOpenExternalLinks(true);
		MessageBody_->setWordWrapMode(QTextOption::WordWrap);

		initSizes();

		delivery_status_->hide();
		avatar_widget_->setVisible(true);
		avatar_->setVisible(true);

		Utils::grabTouchWidget(this);
		Utils::grabTouchWidget(message_widget_);
		Utils::grabTouchWidget(text_widget_);
		setFocusPolicy(Qt::NoFocus);
		MessageBody_->setFocusPolicy(Qt::NoFocus);

		Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_copy_100.png")), QT_TRANSLATE_NOOP("chat_page", "Copy"), makeData("copy"));
		Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quote_100.png")), QT_TRANSLATE_NOOP("chat_page", "Quote"), makeData("quote"));

        connect(avatar_, SIGNAL(pressed()), this, SLOT(avatarClicked()));
		connect(Menu_, SIGNAL(triggered(QAction*)), this, SLOT(menu(QAction*)), Qt::QueuedConnection);
	}

	MessageItem::~MessageItem()
    {
	}

	QString MessageItem::formatRecentsText() const
	{
		if (ContentWidget_)
		{
			return ContentWidget_->toRecentsString();
		}

		return Data_->Text_;
	}

	void MessageItem::initSizes()
	{
		avatar_->setFixedSize(QSize(Utils::scale_value(avatar_width), Utils::scale_value(avatar_height)));
		avatar_widget_->setContentsMargins(QMargins(Utils::scale_value(avatar_left_margin), 0, Utils::scale_value(avatar_right_margin), 0));
		time_->setFixedWidth(Utils::scale_value(time_width));
		time_->setContentsMargins(QMargins(Utils::scale_value(time_margin_left), 0, Utils::scale_value(time_margin_right), Utils::scale_value(time_margin_bottom)));
		time_widget_->setFixedWidth(Utils::scale_value(time_width));
		delivery_status_->setFixedWidth(Utils::scale_value(delivery_width));
        delivery_status_->setFixedHeight(Utils::scale_value(delivery_height));
		delivery_widget_->setFixedWidth(Utils::scale_value(delivery_widget_width));
		delivery_widget_->setContentsMargins(QMargins(Utils::scale_value(delivery_margin_left), 0, Utils::scale_value(delivery_margin_right), Utils::scale_value(delivery_margin_bottom)));
		status_widget_->setFixedWidth(Utils::scale_value(status_widget_width));
		status_widget_->setContentsMargins(QMargins(0, 0, 0, 0));
		MessageSender_->setContentsMargins(QMargins(0, 0, 0, Utils::scale_value(sender_margin_bottom)));
		text_widget_->setContentsMargins(QMargins(Utils::scale_value(message_margin_left), Utils::scale_value(message_margin_top), 0, Utils::scale_value(message_margin_bottom)));
		message_indent_->setFixedHeight(Utils::scale_value(default_indent));
	}

	QHBoxLayout* MessageItem::getTextWidgetLayout()
	{
		return qobject_cast<QHBoxLayout*>(text_widget_->layout());
	}

	QVBoxLayout* MessageItem::getSenderWidgetLayout()
	{
		return qobject_cast<QVBoxLayout*>(sender_widget_->layout());
	}

	void MessageItem::setId(qint64 id, const QString& aimId)
	{
		Data_->Id_ = id;
		Data_->AimId_ = aimId;
	}

	void MessageItem::setNotificationKeys(const QStringList& keys)
	{
        if (!Data_->Outgoing_ || Data_->DeliveredToClient_)
            return;

		Data_->NotificationsKeys_.append(keys);
		Data_->NotificationsKeys_.removeDuplicates();

		connectDeliverySignals(true);
	}

	const QStringList& MessageItem::getNotificationKeys()
	{
		return Data_->NotificationsKeys_;
	}

	void MessageItem::loadAvatar(const QString& sender, const QString& senderName, int size)
	{
		Data_->Sender_ = sender;
		Data_->AvatarSize_ = size;

		bool isDefault = false;
		const auto avatar = Logic::GetAvatarStorage()->Get(Data_->Sender_, senderName, size, true, isDefault);
		if (isDefault)
			connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);

		const auto isAvatarWide = (avatar_->height() < avatar_->width());

        const auto avatarMinimumHeight = Utils::scale_bitmap(avatar_->minimumHeight());
		const auto newP = (isAvatarWide ? avatar->scaledToHeight(avatarMinimumHeight) : avatar->scaledToWidth(avatarMinimumHeight));

		const auto roundedP = Utils::RoundImage(newP, QString());
		avatar_->setIcon(roundedP);
		avatar_->setIconSize(QSize(Utils::scale_bitmap(Utils::scale_value(avatar_height)), Utils::scale_bitmap(Utils::scale_value(avatar_height))));
		setAvatarVisible(true);
	}

	void MessageItem::setAvatarVisible(bool visible)
	{
        if (!visible)
			avatar_->setIcon(QIcon());

		Data_->AvatarVisible_ = visible;

        avatar_widget_->setAttribute(Qt::WA_TransparentForMouseEvents, !visible);
	}

	void MessageItem::resizeEvent(QResizeEvent* e)
	{
		if (e->oldSize() != e->size())
			updateLayout();
	}

	void MessageItem::mouseReleaseEvent(QMouseEvent* e)
	{
		if (e->buttons() & Qt::RightButton || e->button() == Qt::RightButton)
		{
			Menu_->popup(mapToGlobal(e->pos()));
		}
        e->ignore();
	}

	void MessageItem::selectByPos(const QPoint& pos)
	{
		if (MessageBody_)
		{
			MessageBody_->selectByPos(pos);
		}
		else if (ContentWidget_)
		{
			QRect widgetRect = rect();
			widgetRect = QRect(mapToGlobal(widgetRect.topLeft()), mapToGlobal(widgetRect.bottomRight()));
			if (Direction_ == NONE)
			{
				if (abs(pos.y() - widgetRect.y()) < abs(pos.y() - (widgetRect.y() + widgetRect.height())))
					Direction_ = DOWN;
				else
					Direction_ = UP;
			}
			if (widgetRect.y() <= pos.y() && (widgetRect.height() + widgetRect.y()) >= pos.y())
			{
				if (!Selected_)
				{
					select();
				}
			}
			else if (Selected_)
			{
				if (abs(pos.y() - widgetRect.y()) < abs(pos.y() - (widgetRect.y() + widgetRect.height())))
				{
					if (Direction_ == DOWN)
						clearSelection();
				}
				else
				{
					if (Direction_ == UP)
						clearSelection();
				}
			}
			else
			{
				if (abs(pos.y() - widgetRect.y()) < abs(pos.y() - (widgetRect.y() + widgetRect.height())))
				{
					if (Direction_ == UP)
						select();
				}
				else
				{
					if (Direction_ == DOWN)
						select();
				}
			}
		}
		PrevPos_ = pos;
	}

	void MessageItem::select()
	{
		Selected_ = true;
		if (isMessageBubbleVisible())
		{
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidget", !Data_->Outgoing_ && !Selected_);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetSelected", !Data_->Outgoing_ && Selected_);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoing", Data_->Outgoing_ && !Selected_);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoingSelected", Data_->Outgoing_ && Selected_);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetInvisible", !isMessageBubbleVisible());
			Utils::applyWidgetPropChanges(text_widget_);
		}
		else
		{
			ContentWidget_->select(true);
			ContentWidget_->update();
		}
	}

	void MessageItem::clearSelection()
	{
		PrevPos_ = QPoint();
		if (MessageBody_)
			MessageBody_->clearSelection();
		else if (ContentWidget_)
		{
			Direction_ = NONE;
			Selected_ = false;

			if (isMessageBubbleVisible())
			{
				Utils::setPropertyToWidget(text_widget_, "MessageTextWidget", !Data_->Outgoing_ && !Selected_);
				Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetSelected", !Data_->Outgoing_ && Selected_);
				Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoing", Data_->Outgoing_ && !Selected_);
				Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoingSelected", Data_->Outgoing_ && Selected_);
				Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetInvisible", !isMessageBubbleVisible());
				Utils::applyWidgetPropChanges(text_widget_);
			}
			else
			{
				ContentWidget_->select(false);
				ContentWidget_->update();
			}
		}
	}

	QString MessageItem::selection(bool textonly/* = false*/)
	{
		if (MessageBody_ && !MessageBody_->isAllSelected())
			return MessageBody_->selection();

		QString displayName;
		if (Data_->Outgoing_)
			displayName = MyInfo()->friendlyName();
		else
			displayName = Data_->Chat_ ? MessageSender_->text() : Logic::GetContactListModel()->getDisplayName(Data_->AimId_);

        QString format = textonly ? "" : QString("%1 (%2):\n").arg(displayName, QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm"));
		if (MessageBody_)
		{
			format += MessageBody_->selection();
			format += "\n\n";
		}
		else if (Selected_)
		{
			if (!Data_->StickerText_.isEmpty())
				format += Data_->StickerText_;
			else
                format += ContentWidget_->toString();
            format += "\n\n";
		}
		else
		{
			return QString();
		}

		return format;
	}

    void MessageItem::avatarClicked()
    {
        emit Utils::InterConnector::instance().profileSettingsShow(Data_->Sender_);
    }

	void MessageItem::menu(QAction* _action)
	{
		auto params = _action->data().toMap();
		const QString command = params["command"].toString();

        QString displayName;
        if (Data_->Outgoing_)
            displayName = MyInfo()->friendlyName();
        else
            displayName = Data_->Chat_ ? MessageSender_->text() : Logic::GetContactListModel()->getDisplayName(Data_->AimId_);

        QString format = (command == "copy") ? "" : QString("%1 (%2):\n").arg(displayName, QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm"));
        if (MessageBody_)
        {
            format += MessageBody_->getPlainText();
        }
        else
        {
            if (!Data_->StickerText_.isEmpty())
                format += Data_->StickerText_;
            else
                format += ContentWidget_->toString();
        }

		if (command == "copy")
		{
			emit copy(format);
		}
		else if (command == "quote")
		{
            format += "\n\n";
			emit quote(format);
		}
	}

	void MessageItem::updateLayout()
	{
		int widgetHeight = 0;
		if (ContentWidget_)
		{
			widgetHeight = ContentWidget_->height();
		}
		else
		{
			auto documentHeight = MessageBody_->document()->size().height();
			documentHeight += Utils::scale_value(4);
			widgetHeight = std::max<int>(documentHeight, Utils::scale_value(avatar_height));
			MessageBody_->setFixedHeight(widgetHeight);
		}

		widgetHeight += Utils::scale_value(default_indent);
		text_widget_->setFixedHeight(widgetHeight);

		if (Data_->SenderVisible_)
			widgetHeight += MessageSender_->height();

		message_widget_->setFixedHeight(widgetHeight);
		avatar_widget_->setFixedHeight(widgetHeight);

		widgetHeight += Utils::scale_value(Data_->IndentBefore_ ? indent_before : Utils::scale_value(default_indent));

		setFixedHeight(widgetHeight);
	}

	void MessageItem::connectDeliverySignals(const bool isConnected)
	{
		if (isConnected)
		{
            auto success = connect(Logic::GetMessagesModel(), SIGNAL(deliveredToClient(qint64)), this, SLOT(deliveredToClient(qint64)), Qt::QueuedConnection);
            assert(success);

			success = connect(Logic::GetMessagesModel(), SIGNAL(deliveredToClient(QString)), this, SLOT(deliveredToClient(QString)), Qt::QueuedConnection);
			assert(success);

			success = connect(Logic::GetMessagesModel(), SIGNAL(deliveredToServer(QString)), this, SLOT(deliveredToServer(QString)), Qt::QueuedConnection);
			assert(success);
		}
		else
		{
            auto success = disconnect(Logic::GetMessagesModel(), SIGNAL(deliveredToClient(qint64)), this, SLOT(deliveredToClient(qint64)));
            assert(success);

			success = disconnect(Logic::GetMessagesModel(), SIGNAL(deliveredToClient(QString)), this, SLOT(deliveredToClient(QString)));
			assert(success);

			success = disconnect(Logic::GetMessagesModel(), SIGNAL(deliveredToServer(QString)), this, SLOT(deliveredToServer(QString)));
			assert(success);
		}
	}

	bool MessageItem::isMessageBubbleVisible() const
	{
		if (!ContentWidget_)
		{
			return true;
		}

		return ContentWidget_->isBlockElement();
	}

	void MessageItem::setMessage(const QString& message)
	{
        if (Data_->Text_ == message)
        {
            return;
        }

		Data_->Text_ = message;

        if (!parent())
        {
            return;
        }

		MessageBody_->document()->clear();
		MessageBody_->verticalScrollBar()->blockSignals(true);
		const auto uriCallback = [this](const QString &uri, const int startPos)
		{
            if (uri.isEmpty() && startPos < 0)
                assert(false);

			replace();
		};

        QTextCursor cursor = MessageBody_->textCursor();

		Logic::Text4Edit(message, *MessageBody_, Logic::Text2DocHtmlMode::Escape, true, true, uriCallback);
        MessageBody_->setPlainText(message);
		MessageBody_->verticalScrollBar()->blockSignals(false);
	}

	void MessageItem::setOutgoing(const bool isOutgoing, const bool sending, const bool isDeliveredToServer, const bool isDeliveredToClient, const bool isMChat)
	{
		Data_->Outgoing_ = isOutgoing;
		Data_->DeliveredToServer_ = isDeliveredToServer;
        Data_->DeliveredToClient_ = isDeliveredToClient;
		Data_->Chat_ = isMChat;
		Data_->Sending_ = sending;

		Utils::setPropertyToWidget(text_widget_, "MessageTextWidget", !isOutgoing && !Selected_);
		Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetSelected", !isOutgoing && Selected_);
		Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoing", isOutgoing && !Selected_);
		Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoingSelected", isOutgoing && Selected_);
		Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetInvisible", !isMessageBubbleVisible());
		Utils::applyWidgetPropChanges(text_widget_);

		delivery_status_->setVisible(isOutgoing);
		if (isOutgoing)
		{
			if (sending)
			{
                delivery_status_->setImage(":/themes/standard/100/cl/sending_mark.png");
			}
			else
			{
                if (isMChat)
                {
                    delivery_status_->setImage(":/themes/standard/100/cl/read_mark.png");
                }
                else
                {
                    if (isDeliveredToServer)
                    {
                        delivery_status_->setImage(":/themes/standard/100/cl/delivered_mark.png");
                    }
                    if (isDeliveredToClient)
                    {
                        delivery_status_->setImage(":/themes/standard/100/cl/read_mark.png");
                    }
                }
			}
			delivery_status_->setStyle(QApplication::style());
		}
	}

	void MessageItem::contentWidgetSizeChanged()
	{
		updateLayout();
	}

    void MessageItem::deliveredToClient(qint64 id)
    {
        if (Data_->Id_ != id)
            return;

        connectDeliverySignals(false);
        setOutgoing(Data_->Outgoing_, false, true, true, Data_->Chat_);
        setTime(Data_->Time_);
    }

	void MessageItem::deliveredToClient(QString key)
	{
		if (!Data_->NotificationsKeys_.contains(key))
		{
			return;
		}

		connectDeliverySignals(false);

		Data_->NotificationsKeys_.clear(); // don't need them anymore

		__TRACE(
			"delivery",
			"status event reached a widget\n" <<
			"	status=<client>\n" <<
			"	key=<" << key <<">");

		setOutgoing(Data_->Outgoing_, false, true, true, Data_->Chat_);
		setTime(Data_->Time_);
	}

	void MessageItem::deliveredToServer(QString key)
	{
		assert(!key.isEmpty());

		__TRACE(
			"delivery",
			"status event reached a widget\n" <<
			"	status=<server>\n" <<
			"	key=<" << key <<">");

		if (!Data_->NotificationsKeys_.contains(key))
		{
			return;
		}

		setOutgoing(Data_->Outgoing_, false, true, false, Data_->Chat_);
		setTime(Data_->Time_);
	}

	void MessageItem::avatarChanged(QString sender)
	{
		if (sender == Data_->Sender_)
		{
			disconnect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)));
			loadAvatar(Data_->Sender_, QString(), Data_->AvatarSize_);
		}
	}

	void MessageItem::setMchatSender(const QString& sender)
	{
		QString senderName = sender;
		Utils::removeLineBreaks(senderName);
		MessageSender_->setText(sender);
		Data_->SenderVisible_ = !senderName.isEmpty() && Data_->AvatarVisible_;
        Data_->SenderFriendly_ = sender;
		MessageSender_->setContentsMargins(QMargins(0, 0, 0, Utils::scale_value(Data_->SenderVisible_ ? sender_margin_bottom : 0)));
		MessageSender_->setVisible(Data_->SenderVisible_);
		MessageSender_->adjustSize();
	}

	void MessageItem::setTime(qint32 _time)
	{
		Data_->Time_ = _time;
		time_->setText(QDateTime::fromTime_t(_time).time().toString("HH:mm"));
		time_widget_->setVisible(!Data_->Sending_);
	}

	void MessageItem::setWidth(const int width)
	{
		Data_->Width_ = width;
		message_widget_->setContentsMargins(QMargins(Utils::scale_value(Data_->Outgoing_ ? message_widget_left_margin_outgoing : message_widget_left_margin), 0, Utils::scale_value(Data_->Outgoing_ ? message_widget_right_margin_outgoing : message_widget_right_margin), 0));
        setFixedWidth(width);
		setMessageWidth(width);
	}

	void MessageItem::setMessageWidth(const int width)
	{
		auto contentWidth = width - Utils::scale_value(status_widget_width + avatar_widget_width);
		contentWidth -= Utils::scale_value(message_margin_left);

		if (Data_->Outgoing_)
		{
			contentWidth -= Utils::scale_value(message_widget_left_margin_outgoing);
			contentWidth -= Utils::scale_value(message_widget_right_margin_outgoing);
		}
		else
		{
			contentWidth -= Utils::scale_value(message_widget_left_margin);
			contentWidth -= Utils::scale_value(message_widget_right_margin);
		}

		MessageSender_->setFixedWidth(contentWidth);
		if (ContentWidget_)
		{
			const auto leftMargin = (ContentWidget_->isBlockElement() ? Utils::scale_value(12) : 0);
			ContentWidget_->setWidth(contentWidth);
			ContentWidget_->setContentsMargins(leftMargin, 0, 0, 0);
		}
		else
		{
			MessageBody_->setFixedWidth(contentWidth);
            text_widget_->setFixedWidth(Utils::scale_value(contentWidth + message_margin_left + (Data_->Outgoing_ ? status_widget_width : time_width)));
            message_widget_->setFixedWidth(Utils::scale_value(contentWidth + message_margin_left + (Data_->Outgoing_ ? status_widget_width : time_width)));
		}

		updateLayout();
	}

	void MessageItem::setTopMargin(const bool value)
	{
        Data_->IndentBefore_ = value;
		message_indent_->setFixedHeight(Utils::scale_value(Data_->IndentBefore_ ? indent_before : default_indent));

        HistoryControlPageItem::setTopMargin(value);
	}

	void MessageItem::setContentWidget(HistoryControl::MessageContentWidget *widget)
	{
		assert(widget);
		assert(!ContentWidget_);

		ContentWidget_ = widget;
        if (!parent())
            return;

		ContentWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		Utils::grabTouchWidget(ContentWidget_);
		getTextWidgetLayout()->removeWidget(MessageBody_);
		delete MessageBody_;
		MessageBody_ = nullptr;

		getTextWidgetLayout()->insertWidget(0, ContentWidget_, 0, Qt::AlignLeft);

		ContentWidget_->setVisible(true);

		const auto success = QObject::connect(
			ContentWidget_, SIGNAL(sizeChanged()),
			this, SLOT(contentWidgetSizeChanged()), Qt::QueuedConnection);
		assert(success);
	}

	void MessageItem::setStickerText(const QString& text)
	{
		Data_->StickerText_ = text;
	}

	void MessageItem::setDate(const QDate& date)
	{
		Data_->Date_ = date;
	}

	void MessageItem::replace()
	{
		auto doc = MessageBody_->document();
		doc->isEmpty();
	}

	bool MessageItem::selected()
	{
		if (!MessageBody_)
		{
			return false;
		}

		return !MessageBody_->textCursor().selectedText().isEmpty();
	}

	QDate MessageItem::date() const
	{
		return Data_->Date_;
	}

	qint64 MessageItem::getId() const
	{
		return Data_->Id_;
	}

	bool MessageItem::isPersistent() const
	{
		return ContentWidget_ ? !ContentWidget_->canUnload() : false;
	}

	void MessageItem::prepareForInsertion()
	{
		if (!ContentWidget_)
		{
			return;
		}

		ContentWidget_->initialize();

		if (!ContentWidget_->isBlockElement())
		{
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidget", false);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoing", false);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetSelected", false);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoing", false);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetOutgoingSelected", false);
			Utils::setPropertyToWidget(text_widget_, "MessageTextWidgetInvisible", !isMessageBubbleVisible());
			Utils::applyWidgetPropChanges(text_widget_);
		}
	}

	QString MessageItem::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		if (ContentWidget_)
		{
			fmt << ContentWidget_->toLogString();
		}
		else
		{
			auto text = MessageBody_->getPlainText();
			text.replace("\n", "\\n");
			fmt << "	text=<" << text << ">";
		}

		return result;
	}

	bool MessageItem::updateData(std::shared_ptr<MessageData> data)
	{
		const auto needUpdateLayout = data->Text_ != Data_->Text_ || data->AvatarVisible_ != Data_->AvatarVisible_;
		Data_ = data;
		if (Data_->Outgoing_ && !Data_->DeliveredToClient_)
			connectDeliverySignals(true);
        if (MessageBody_)
            setMessage(Data_->Text_);

		setTime(Data_->Time_);
		setOutgoing(Data_->Outgoing_, Data_->Sending_, Data_->DeliveredToServer_, Data_->DeliveredToClient_, Data_->Chat_);
		setTopMargin(Data_->IndentBefore_);
		if (Data_->AvatarVisible_)
			loadAvatar(Data_->Sender_, QString(), Utils::scale_bitmap(Utils::scale_value(32)));
		else
			setAvatarVisible(Data_->AvatarVisible_);

        setMchatSender(Data_->SenderFriendly_);

        if (ContentWidget_)
        {
            Data_->StickerText_ = data->StickerText_;
            return false;
        }

		auto contentWidth = Data_->Width_ - Utils::scale_value(status_widget_width + avatar_widget_width);
		contentWidth -= Utils::scale_value(message_margin_left);

		if (Data_->Outgoing_)
		{
			contentWidth -= Utils::scale_value(message_widget_left_margin_outgoing);
			contentWidth -= Utils::scale_value(message_widget_right_margin_outgoing);
		}
		else
		{
			contentWidth -= Utils::scale_value(message_widget_left_margin);
			contentWidth -= Utils::scale_value(message_widget_right_margin);
		}

		if (needUpdateLayout)
			updateLayout();

		return needUpdateLayout;
	}

	std::shared_ptr<MessageData> MessageItem::getData()
	{
		return Data_;
	}
}
