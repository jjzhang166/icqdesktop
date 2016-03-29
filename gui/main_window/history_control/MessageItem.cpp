#include "stdafx.h"

#include "MessageStatusWidget.h"
#include "MessageItemLayout.h"
#include "MessageItem.h"
#include "MessagesModel.h"
#include "MessageStyle.h"

#include "../history_control/FileSharingWidget.h"
#include "../history_control/StickerWidget.h"

#include "../../utils/utils.h"
#include "../../utils/PainterPath.h"
#include "../../utils/Text.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"

#include "../contact_list/RecentsModel.h"
#include "../contact_list/ContactListModel.h"

#include "../../core_dispatcher.h"

#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/ContextMenu.h"

#include "../../cache/avatars/AvatarStorage.h"

#include "../../my_info.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"
#include "ImagePreviewWidget.h"

namespace
{
    int32_t getAvatarRightMargin();

    int32_t getAvatarSize();

    int32_t getBubbleHorPadding();

    int32_t getLeftPadding(const bool isOutgoing);

    int32_t getMessageBottomPadding();

    int32_t getMessageTopPadding();

    int32_t getRightPadding(const bool isOutgoing);

    int32_t getSenderHeight();

    int32_t getSenderBottomMargin();

    int32_t getStatusIconBottomPadding(const Themes::PixmapResourceId resId);

    const QFont& getTimeFont();

    int32_t getTimeStatusMargin();

    int32_t getTopPadding(const bool hasTopMargin);

	QMap<QString, QVariant> makeData(const QString& command);
}

namespace Ui
{
    MessageItem::MessageItem()
        : HistoryControlPageItem(0)
        , Data_(new MessageData())
        , MessageBody_(nullptr)
        , Sender_(nullptr)
        , ContentWidget_(nullptr)
        , Menu_(0)
        , ContentMenu_(0)
        , ClickedOnAvatar_(false)
        , Layout_(nullptr)
        , StatusWidget_(nullptr)
    {
    }

	MessageItem::MessageItem(QWidget* parent)
		: HistoryControlPageItem(parent)
		, MessageBody_(nullptr)
		, Sender_(nullptr)
		, ContentWidget_(nullptr)
		, Data_(new MessageData())
        , Direction_(NONE)
		, Menu_(0)
        , ContentMenu_(0)
        , ClickedOnAvatar_(false)
        , Layout_(new MessageItemLayout(this))
        , StatusWidget_(new MessageStatusWidget(this))
    {
        QMetaObject::connectSlotsByName(this);

		setAttribute(Qt::WA_AcceptTouchEvents);

        setMouseTracking(true);

		Utils::grabTouchWidget(this);
		setFocusPolicy(Qt::NoFocus);

        assert(Layout_);
        setLayout(Layout_);

        initMenu();
	}

	MessageItem::~MessageItem()
    {
	}
    
    void MessageItem::setContact(const QString& _aimId)
    {
        HistoryControlPageItem::setContact(_aimId);
        if (StatusWidget_)
        {
            StatusWidget_->setContact(_aimId);
        }
    }

	QString MessageItem::formatRecentsText() const
	{
		if (ContentWidget_)
		{
			return ContentWidget_->toRecentsString();
		}

		return Data_->Text_;
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

	void MessageItem::loadAvatar(const QString &sender, const QString &senderName, const int size)
	{
        assert(!sender.isEmpty());
        assert(size > 0);

		Data_->Sender_ = sender;
		Data_->AvatarSize_ = size;

		auto isDefault = false;
		const auto avatar = Logic::GetAvatarStorage()->Get(Data_->Sender_, senderName, size, true, Out isDefault);
		if (isDefault)
        {
			connect(
                Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)),
                this, SLOT(avatarChanged(QString)),
                Qt::QueuedConnection
            );
        }

		const auto isAvatarWide = (avatar->height() < avatar->width());

        const auto scaledAvatarHeight = Utils::scale_bitmap(getAvatarSize());
		const auto newP = (
            isAvatarWide ?
                avatar->scaledToHeight(scaledAvatarHeight) :
                avatar->scaledToWidth(scaledAvatarHeight)
        );

		Avatar_ = Utils::RoundImage(newP, QString());

		setAvatarVisible(true);

        update();
	}

    QSize MessageItem::sizeHint() const
    {
        auto height = evaluateTopContentMargin();

        height += evaluateDesiredContentHeight();

        const QSize result(
            0,
            height
        );

        return result;
    }

	void MessageItem::setAvatarVisible(const bool visible)
	{
        const auto visibilityChanged = (Data_->AvatarVisible_ != visible);

        Data_->AvatarVisible_ = visible;

        if (visibilityChanged)
        {
            update();
        }
	}

    void MessageItem::leaveEvent(QEvent *e)
    {
        ClickedOnAvatar_ = false;

        QWidget::leaveEvent(e);
    }

    void MessageItem::mouseMoveEvent(QMouseEvent *e)
    {
        if (isOverAvatar(e->pos()))
        {
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }

        QWidget::mouseMoveEvent(e);
    }

    void MessageItem::mousePressEvent(QMouseEvent *e)
    {
        const auto isLeftButtonFlagSet = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (e->button() == Qt::LeftButton)
        );

        if (isLeftButton && isOverAvatar(e->pos()))
        {
            ClickedOnAvatar_ = true;
        }

        QWidget::mousePressEvent(e);
    }

    void MessageItem::mouseReleaseEvent(QMouseEvent *e)
    {
        const auto isRightButtonFlagSet = ((e->buttons() & Qt::RightButton) != 0);
        const auto isRightButton = (
            isRightButtonFlagSet ||
            (e->button() == Qt::RightButton)
        );

        if (isRightButton)
        {
            QPoint globalPos = mapToGlobal(e->pos());
            if (MessageBody_)
            {
                Menu_->popup(globalPos);
            }
            else if (ContentWidget_)
            {
                if (ContentWidget_->haveContentMenu(globalPos))
                    ContentMenu_->popup(globalPos);
                else
                    Menu_->popup(globalPos);
            }
        }

        const auto isLeftButtonFlagSet = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (e->button() == Qt::LeftButton)
        );

        if (isLeftButton &&
            ClickedOnAvatar_ &&
            isOverAvatar(e->pos()))
        {
            avatarClicked();
        }

        QWidget::mouseReleaseEvent(e);
    }

    void MessageItem::paintEvent(QPaintEvent*)
    {
        assert(parent());

        QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::TextAntialiasing);

        drawAvatar(p);

        drawMessageBubble(p);
    }
    
    void MessageItem::updateMessageBodyColor()
    {
        if (MessageBody_)
        {
            QColor textColor = QColor(0x28, 0x28, 0x28);
            if (theme())
            {
                if (isOutgoing())
                {
                    textColor = theme()->outgoing_bubble_.text_color_;
                }
                else
                {
                    textColor = theme()->incoming_bubble_.text_color_;
                }
            }
            QPalette palette;;
            palette.setColor(QPalette::Text, textColor);
            MessageBody_->setPalette(palette);
        }
    }

    void MessageItem::resizeEvent(QResizeEvent *e)
    {
        HistoryControlPageItem::resizeEvent(e);
    }

    void MessageItem::createMenu()
    {
        Menu_ = new ContextMenu(this);
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_copy_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy"), makeData("copy"));
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quote_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));
        //Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete"), makeData("delete"));
        
        connect(Menu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);
    }
    
    void MessageItem::createContentMenu()
    {
        ContentMenu_ = new ContextMenu(this);
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_download_100.png")), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData("save_as"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_attach_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy file"), makeData("copy_file"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_link_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData("copy_link"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quote_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));
        //ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete"), makeData("delete"));
        
        connect(ContentMenu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);
    }

    void MessageItem::initMenu()
    {
        createMenu();
        createContentMenu();
    }

    void MessageItem::createMessageBody()
    {
        if (MessageBody_)
        {
            return;
        }

        QPalette palette;;
        MessageBody_ = new TextEditEx(
            this,
            Utils::FontsFamily::SEGOE_UI,
            Utils::scale_value(15),
            palette,
            false,
            false
        );
        updateMessageBodyColor();
        
        QString styleSheet = QString("background: transparent; selection-background-color: %1;").arg(Utils::getSelectionColor().name());

        MessageBody_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        MessageBody_->setFrameStyle(QFrame::NoFrame);
        MessageBody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->setOpenLinks(true);
        MessageBody_->setOpenExternalLinks(true);
        MessageBody_->setWordWrapMode(QTextOption::WordWrap);
        MessageBody_->setStyleSheet(styleSheet);
        MessageBody_->setFocusPolicy(Qt::NoFocus);
        MessageBody_->document()->setDocumentMargin(0);
    }
    
    void MessageItem::updateSenderControlColor()
    {
        QColor color = theme() ? theme()->contact_name_.text_color_ : QColor(0x57, 0x54, 0x4c);
        Sender_->setColor(color);
    }

    void MessageItem::createSenderControl()
    {
        if (Sender_)
        {
            return;
        }
        
        QColor color;
        Sender_ = new TextEmojiWidget(
            this,
            Utils::FontsFamily::SEGOE_UI,
            Utils::scale_value(12),
            color
        );
        updateSenderControlColor();
    }

    void MessageItem::drawAvatar(QPainter &p)
    {
        if (Avatar_.isNull() || !Data_->AvatarVisible_)
        {
            return;
        }

        p.drawPixmap(getAvatarRect(), Avatar_);
    }

    void MessageItem::drawMessageBubble(QPainter &p)
    {
        if (ContentWidget_ && !ContentWidget_->isBlockElement())
        {
            return;
        }

        if (Bubble_.isEmpty())
        {
            return;
        }

        assert(!Bubble_.isEmpty());
        p.fillPath(Bubble_, MessageStyle::getBodyBrush(Data_->Outgoing_, isSelected(), theme()->get_id()));

    }

    QRect MessageItem::evaluateAvatarRect() const
    {
        assert(!AvatarRect_.isValid());

        QRect result(
            getLeftPadding(isOutgoing()),
            getTopPadding(hasTopMargin()),
            getAvatarSize(),
            getAvatarSize()
        );

        return result;
    }

    QRect MessageItem::evaluateContentHorGeometry(const int32_t contentWidth) const
    {
        assert(contentWidth > 0);

        const QRect result(
            evaluateLeftContentMargin(),
            -1,
            contentWidth,
            0
        );

        return result;
    }

    QRect MessageItem::evaluateContentVertGeometry() const
    {
        const auto contentHeight = evaluateDesiredContentHeight();

        const QRect result(
            -1,
            evaluateTopContentMargin(),
            0,
            contentHeight
        );

        return result;
    }

    int32_t MessageItem::evaluateContentWidth(const int32_t widgetWidth) const
    {
        assert(widgetWidth > 0);

        auto contentWidth = widgetWidth;

        contentWidth -= evaluateLeftContentMargin();
        contentWidth -= evaluateRightContentMargin();

        return contentWidth;
    }

    int32_t MessageItem::evaluateDesiredContentHeight() const
    {
        auto height = Utils::scale_value(32);

        if (MessageBody_)
        {
            assert(!ContentWidget_);

            const auto textHeight = MessageBody_->getTextSize().height();

            if (textHeight > 0)
            {
                height = std::max<int32_t>(
                    height,
                    textHeight
                );

                height += getMessageTopPadding();

                height = Utils::applyMultilineTextFix(textHeight, height);
            }
        }

        if (ContentWidget_)
        {
            assert(!MessageBody_);

            height = std::max(height, ContentWidget_->height());
        }

        return height;
    }

    int32_t MessageItem::evaluateLeftContentMargin() const
    {
        auto leftContentMargin = getLeftPadding(isOutgoing());

        if (isAvatarVisible())
        {
            leftContentMargin += getAvatarSize();
            leftContentMargin += getAvatarRightMargin();
        }

        return leftContentMargin;
    }

    int32_t MessageItem::evaluateRightContentMargin() const
    {
        auto rightContentMargin = getRightPadding(isOutgoing());

        return rightContentMargin;
    }

    int32_t MessageItem::evaluateTopContentMargin() const
    {
        auto topContentMargin = getTopPadding(hasTopMargin());

        if (Data_->SenderVisible_)
        {
            topContentMargin += getSenderHeight();
            topContentMargin += getSenderBottomMargin();
        }

        return topContentMargin;
    }

    const QRect& MessageItem::getAvatarRect() const
    {
        assert(isAvatarVisible());

        if (!AvatarRect_.isValid())
        {
            AvatarRect_ = evaluateAvatarRect();
            assert(AvatarRect_.isValid());
        }

        return AvatarRect_;
    }

    Themes::PixmapResourceId MessageItem::getDeliveryStatusResId() const
    {
        assert(isOutgoing());

        if (Data_->Sending_)
        {
            return Themes::PixmapResourceId::ContactListSendingMark;
        }

        if (Data_->Chat_)
        {
            return Themes::PixmapResourceId::ContactListReadMark;
        }

        if (Data_->DeliveredToClient_)
        {
            return Themes::PixmapResourceId::ContactListReadMark;
        }

        if (Data_->DeliveredToServer_)
        {
            return Themes::PixmapResourceId::ContactListDeliveredMark;
        }

        assert(!"unexpected outcome");
        return Themes::PixmapResourceId::Invalid;
    }

    bool MessageItem::isAvatarVisible() const
    {
        return !isOutgoing();
    }

    bool MessageItem::isBlockItem() const
    {
        assert((bool)MessageBody_ ^ (bool)ContentWidget_);

        return (MessageBody_ || ContentWidget_->isBlockElement());
    }

    bool MessageItem::isOutgoing() const
    {
        return Data_->Outgoing_;
    }

    bool MessageItem::isOverAvatar(const QPoint &p) const
    {
        return (
            isAvatarVisible() &&
            getAvatarRect().contains(p)
        );
    }

    bool MessageItem::isSending() const
    {
        return Data_->Sending_;
    }

    void MessageItem::manualUpdateGeometry(const int32_t widgetWidth)
    {
        assert(widgetWidth > 0);

        const auto contentWidth = evaluateContentWidth(widgetWidth);

        const auto enoughSpace = (contentWidth > 0);
        if (!enoughSpace)
        {
            return;
        }

        const auto contentHorGeometry = evaluateContentHorGeometry(contentWidth);

        updateMessageBodyHorGeometry(contentHorGeometry);

        updateContentWidgetHorGeometry(contentHorGeometry);

        const auto contentVertGeometry = evaluateContentVertGeometry();
        const QRect contentGeometry(
            contentHorGeometry.left(),
            contentVertGeometry.top(),
            contentHorGeometry.width(),
            contentVertGeometry.height()
        );
        assert(!contentGeometry.isEmpty());

        updateMessageBodyFullGeometry(contentGeometry);

        updateBubbleGeometry(contentGeometry);

        updateSenderGeometry();

        updateStatusGeometry(contentGeometry);
    }

    void MessageItem::updateBubbleGeometry(const QRect &bubbleGeometry)
    {
        assert(!bubbleGeometry.isEmpty());

        Bubble_ = Utils::renderMessageBubble(bubbleGeometry, Utils::scale_value(8), Data_->Outgoing_);
        assert(!Bubble_.isEmpty());
    }

    void MessageItem::updateContentWidgetHorGeometry(const QRect &bubbleHorGeometry)
    {
        assert(bubbleHorGeometry.width() > 0);

        if (!ContentWidget_)
        {
            return;
        }

        auto left = bubbleHorGeometry.left();

        if (ContentWidget_->isBlockElement())
        {
            left += getBubbleHorPadding();
        }

        ContentWidget_->move(
            left,
            evaluateTopContentMargin()
        );

        if (ContentWidget_->isBlockElement())
        {
            return;
        }

        auto width = bubbleHorGeometry.width();
        
        int maxWidgetWidth = ContentWidget_->maxWidgetWidth();

        if (!ContentWidget_->hasTextBubble() || maxWidgetWidth != -1)
        {
            width -= MessageStatusWidget::getMaxWidth(isOutgoing());
            width -= getTimeStatusMargin();
            width -= getTimeStatusMargin();
        }

        assert(width > 0);

        if (maxWidgetWidth == -1)
        {
            ContentWidget_->setFixedWidth(width);
            return;
        }
        
        if (width > maxWidgetWidth)
            width = maxWidgetWidth;
        
        ContentWidget_->setFixedWidth(width);
    }

    void MessageItem::updateMessageBodyHorGeometry(const QRect &bubbleHorGeometry)
    {
        assert(bubbleHorGeometry.width() > 0);

        if (!MessageBody_)
        {
            return;
        }

        auto messageBodyWidth = bubbleHorGeometry.width();
        messageBodyWidth -= (getBubbleHorPadding() * 2);
        messageBodyWidth -= MessageStatusWidget::getMaxWidth(isOutgoing());
        assert(messageBodyWidth > 0);

        const auto widthChanged = (messageBodyWidth != MessageBody_->getTextSize().width());
        if (!widthChanged)
        {
            return;
        }

        MessageBody_->setFixedWidth(messageBodyWidth);
        MessageBody_->document()->setTextWidth(messageBodyWidth);
    }

    void MessageItem::updateMessageBodyFullGeometry(const QRect &bubbleRect)
    {
        if (!MessageBody_)
        {
            return;
        }

        const QRect messageBodyGeometry(
            bubbleRect.left() + getBubbleHorPadding(),
            bubbleRect.top() + getMessageTopPadding(),
            MessageBody_->getTextSize().width(),
            MessageBody_->getTextSize().height()
        );
        assert(!messageBodyGeometry.isEmpty());

        MessageBody_->setGeometry(messageBodyGeometry);
    }

    void MessageItem::updateSenderGeometry()
    {
        if (!Sender_)
        {
            return;
        }

        if (!Data_->SenderVisible_)
        {
            Sender_->setVisible(false);
            return;
        }

        Sender_->move(
            getLeftPadding(isOutgoing())
            + getAvatarSize()
            + getAvatarRightMargin(),
            getTopPadding(hasTopMargin())
        );

        Sender_->setVisible(true);
    }

    void MessageItem::updateStatusGeometry(const QRect &contentGeometry)
    {
        const auto statusWidgetSize = StatusWidget_->sizeHint();

        QPoint posHint;

        if (ContentWidget_)
        {
            // width with left margin
            auto fullStatusLineWidth = (
                MessageStatusWidget::getMaxWidth(isOutgoing()) +
                getTimeStatusMargin()
            );

            if (ContentWidget_->hasTextBubble())
            {
                // right margin
                fullStatusLineWidth += getTimeStatusMargin();
            }

            posHint = ContentWidget_->deliveryStatusOffsetHint(
                fullStatusLineWidth
            );
        }

        assert(posHint.x() >= 0);
        assert(posHint.y() >= 0);

        auto statusX = 0;

        if (posHint.x() > 0)
        {
            statusX = posHint.x();
            statusX += evaluateLeftContentMargin();
            statusX += getTimeStatusMargin();
        }
        else
        {
            statusX = contentGeometry.right();

            statusX -= MessageStatusWidget::getMaxWidth(isOutgoing());
            statusX -= getTimeStatusMargin();
        }

        auto statusY = 0;

        if (posHint.y() > 0)
        {
            statusY = posHint.y();
            statusY += evaluateTopContentMargin();
        }
        else
        {
            statusY = contentGeometry.bottom();
        }

        statusY -= getTimeStatusMargin();
        statusY -= statusWidgetSize.height();

        const QRect statusGeometry(
            statusX,
            statusY,
            MessageStatusWidget::getMaxWidth(isOutgoing()),
            statusWidgetSize.height()
        );

        StatusWidget_->setGeometry(statusGeometry);
    }

	void MessageItem::selectByPos(const QPoint& pos)
	{
		if (MessageBody_)
		{
			MessageBody_->selectByPos(pos);
            return;
		}

        assert(ContentWidget_);

        QRect widgetRect;
        bool selected = false;
        if (ContentWidget_->hasTextBubble())
        {
            selected = ContentWidget_->selectByPos(pos);
            widgetRect = QRect(mapToGlobal(ContentWidget_->rect().topLeft()), mapToGlobal(ContentWidget_->rect().bottomRight()));
        }
        else
        {
            widgetRect = QRect(mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
        }

        const auto isCursorOverWidget = (
            (widgetRect.top() <= pos.y()) &&
            (widgetRect.bottom() >= pos.y())
        );
		if (isCursorOverWidget && !selected)
		{
			if (!isSelected())
			{
				select();
			}

            return;
		}

        const auto distanceToWidgetTop = std::abs(pos.y() - widgetRect.top());
        const auto distanceToWidgetBottom = std::abs(pos.y() - widgetRect.bottom());
        const auto isCursorCloserToTop = (distanceToWidgetTop < distanceToWidgetBottom);

        if (Direction_ == NONE)
        {
            Direction_ = (isCursorCloserToTop ? DOWN : UP);
        }

        if (selected)
            return;

        const auto isDirectionDown = (Direction_ == DOWN);
        const auto isDirectionUp = (Direction_ == UP);

		if (isSelected())
		{
            const auto needToClear = (
                (isCursorCloserToTop && isDirectionDown) ||
                (!isCursorCloserToTop && isDirectionUp)
            );

			if (needToClear)
            {
				clearSelection();
            }

            return;
		}

        const auto needToSelect = (
            (isCursorCloserToTop && isDirectionUp) ||
            (!isCursorCloserToTop && isDirectionDown)
        );

		if (needToSelect)
        {
            select();
        }
	}

	void MessageItem::select()
	{
		HistoryControlPageItem::select();

		if (ContentWidget_)
        {
			ContentWidget_->select(true);
        }
	}

	void MessageItem::clearSelection()
	{
        HistoryControlPageItem::clearSelection();

		if (MessageBody_)
        {
			MessageBody_->clearSelection();
        }

		if (ContentWidget_)
		{
			Direction_ = NONE;

            if (ContentWidget_->hasTextBubble())
            {
                ContentWidget_->clearSelection();
            }

			if (isMessageBubbleVisible())
			{
			}
			else
			{
				ContentWidget_->select(false);
			}
		}
	}

	QString MessageItem::selection(bool textonly/* = false*/)
	{
        QString selectedText;
        if (ContentWidget_)
            selectedText = ContentWidget_->selectedText();

        if (!MessageBody_ && !isSelected() && selectedText.isEmpty())
        {
            return QString();
        }

		if (textonly && MessageBody_ && !MessageBody_->isAllSelected())
        {
			return MessageBody_->selection();
        }

		QString displayName;

		if (isOutgoing())
        {
			displayName = MyInfo()->friendlyName();
        }
		else
        {
			displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::GetContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        QString format = (
            textonly ?
                "" :
                QString("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm")
                )
        );

        if (MessageBody_)
		{
            if (!MessageBody_->selection().isEmpty())
            {
			    format += MessageBody_->selection();
			    format += "\n\n";
                return format;
            }
            return QString();
		}

        assert(isSelected() || !selectedText.isEmpty());

		if (!Data_->StickerText_.isEmpty())
        {
			format += Data_->StickerText_;
        }
		else
        {
            format += ContentWidget_->toString();
        }

        if (!textonly)
            format += "\n\n";

        return format;
	}

    void MessageItem::avatarClicked()
    {
        emit Utils::InterConnector::instance().profileSettingsShow(Data_->Sender_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_avatar);
    }

	void MessageItem::menu(QAction* _action)
	{
		const auto params = _action->data().toMap();
		const auto command = params["command"].toString();

        QString displayName;

        if (isOutgoing())
        {
            displayName = MyInfo()->friendlyName();
        }
        else
        {
            displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::GetContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        QString format = (
            (command == "copy") ?
                "" :
                QString("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm")
                )
        );

        if (MessageBody_)
        {
            format += MessageBody_->getPlainText();
        }
        else
        {
            if (!Data_->StickerText_.isEmpty())
            {
                format += Data_->StickerText_;
            }
            else
            {
                format += ContentWidget_->toString();
            }
        }

		if (command == "copy")
		{
			emit copy(format);
		}
		else if (command == "quote")
		{
            format += "\n";
			emit quote(format);
		}
        else if (command == "copy_link")
        {
            emit copy(ContentWidget_->toLink());
        }
        else if (command == "copy_file")
        {
            ContentWidget_->copyFile();
        }
        else if (command == "save_as")
        {
            ContentWidget_->saveAs();
        }
        else if (command == "delete")
        {
            GetDispatcher()->delete_message(Data_->Id_);
        }
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
        if (auto item = qobject_cast<HistoryControl::ImagePreviewWidget*>(ContentWidget_))
        {
            if (!item->isImageBubbleVisible() || item->isTextPresented())
            {
                return true;
            }
        }

		return ContentWidget_->isBlockElement();
	}

	void MessageItem::setMessage(const QString& message)
	{
        assert(!ContentWidget_);

		Data_->Text_ = message;

        if (!parent())
        {
            return;
        }

        createMessageBody();

		MessageBody_->document()->clear();
		MessageBody_->verticalScrollBar()->blockSignals(true);
		const auto uriCallback =
            [this](const QString &uri, const int startPos)
		    {
                if (uri.isEmpty() && startPos < 0)
                {
                    assert(false);
                }

			    replace();
		    };

        auto cursor = MessageBody_->textCursor();

		Logic::Text4Edit(message, *MessageBody_, Logic::Text2DocHtmlMode::Escape, true, true, uriCallback);
		MessageBody_->verticalScrollBar()->blockSignals(false);

        Layout_->setDirty();

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateGeometry();
        update();
	}

	void MessageItem::setOutgoing(const bool isOutgoing, const bool sending, const bool isDeliveredToServer, const bool isDeliveredToClient, const bool isMChat)
	{
        Data_->Outgoing_ = isOutgoing;
        Data_->DeliveredToServer_ = isDeliveredToServer;
        Data_->DeliveredToClient_ = isDeliveredToClient;
        Data_->Chat_ = isMChat;
        Data_->Sending_ = sending;

        if (isOutgoing)
        {
            const auto isDeliveredToMchat = (isMChat && isDeliveredToServer);

            if (isDeliveredToClient || isDeliveredToMchat)
            {
                StatusWidget_->setDeliveredToClient();
            }

            if (isDeliveredToServer)
            {
                StatusWidget_->setDeliveredToServer();
            }
        }

        StatusWidget_->show();

        Layout_->setDirty();

        updateGeometry();

        update();
	}

	void MessageItem::deliveredToClient(qint64 id)
    {
        if (Data_->Id_ != id)
            return;

        connectDeliverySignals(false);
        setOutgoing(Data_->Outgoing_, false, true, true, Data_->Chat_);
        setTime(Data_->Time_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_sent_read);
    }

	void MessageItem::deliveredToClient(QString key)
	{
		if (!Data_->NotificationsKeys_.contains(key))
		{
			return;
		}

		connectDeliverySignals(false);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_sent_read);

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

        if (Data_->DeliveredToServer_)
            return;

        if (Data_->Chat_)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_sent_groupchat);
        else
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_sent);
		setOutgoing(Data_->Outgoing_, false, true, false, Data_->Chat_);
		setTime(Data_->Time_);
	}

	void MessageItem::avatarChanged(QString sender)
	{
		if (sender != Data_->Sender_)
		{
            return;
        }

		disconnect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)));
		loadAvatar(Data_->Sender_, QString(), Data_->AvatarSize_);
	}

    void MessageItem::setMchatSenderAimId(const QString& senderAimId)
    {
        MessageSenderAimId_ = senderAimId;
    }

	void MessageItem::setMchatSender(const QString& sender)
	{
        createSenderControl();

		Sender_->setText(sender);
        Data_->SenderFriendly_ = sender;

        auto senderName = sender;
        Utils::removeLineBreaks(InOut senderName);

		Data_->SenderVisible_ = (!senderName.isEmpty() && Data_->AvatarVisible_);

        Sender_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

        updateGeometry();
	}

	void MessageItem::setTime(const int32_t time)
	{
		Data_->Time_ = time;

        StatusWidget_->setTime(time);

        StatusWidget_->show();
	}

	void MessageItem::setTopMargin(const bool value)
	{
        Data_->IndentBefore_ = value;

        HistoryControlPageItem::setTopMargin(value);
	}

	void MessageItem::setContentWidget(HistoryControl::MessageContentWidget *widget, bool)
	{
		assert(widget);
		assert(!ContentWidget_);
        assert(!MessageBody_);

		ContentWidget_ = widget;
        connect(ContentWidget_, SIGNAL(stateChanged()), this, SLOT(updateData()), Qt::UniqueConnection);
        if (StatusWidget_)
        {
            StatusWidget_->setMessageBubbleVisible(isMessageBubbleVisible());
        }

        if (!parent())
        {
            return;
        }

        auto success = QObject::connect(
            ContentWidget_,
            &HistoryControl::MessageContentWidget::forcedLayoutUpdatedSignal,
            this,
            [this]
            {
                Layout_->setDirty();
            },
            Qt::DirectConnection
        );
        assert(success);

        ContentWidget_->setParent(this);
        ContentWidget_->show();

        StatusWidget_->raise();

		Utils::grabTouchWidget(ContentWidget_);

        updateGeometry();

        update();
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

	QString MessageItem::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		if (ContentWidget_)
		{
			fmt << ContentWidget_->toLogString();
		}

		if (MessageBody_)
		{
			auto text = MessageBody_->getPlainText();
			text.replace("\n", "\\n");
			fmt << "	text=<" << text << ">";
		}

		return result;
	}

    bool MessageItem::updateData()
    {
        updateMessageBodyColor();
        updateSenderControlColor();
        if (StatusWidget_)
        {
            StatusWidget_->setMessageBubbleVisible(isMessageBubbleVisible() );
        }
        update();
        return true;
    }
    
	bool MessageItem::updateData(const std::shared_ptr<MessageData> &data)
	{
		const auto needUpdateLayout = (
            (data->Text_ != Data_->Text_) ||
            (data->AvatarVisible_ != Data_->AvatarVisible_)
        );

		Data_ = data;
        aimId_ = data->AimId_;

		if (Data_->Outgoing_ && !Data_->DeliveredToClient_)
        {
            setNotificationKeys(Data_->NotificationsKeys_);
        }

        if (MessageBody_)
        {
            setMessage(Data_->Text_);
        }

		setTime(Data_->Time_);
		setOutgoing(Data_->Outgoing_, Data_->Sending_, Data_->DeliveredToServer_, Data_->DeliveredToClient_, Data_->Chat_);
		setTopMargin(Data_->IndentBefore_);

		if (Data_->AvatarVisible_)
        {
            loadAvatar(Data_->Sender_, QString(), Utils::scale_bitmap(getAvatarSize()));
        }
		else
        {
			setAvatarVisible(Data_->AvatarVisible_);
        }

        setMchatSenderAimId(Data_->Sender_);
        setMchatSender(Data_->SenderFriendly_);

        if (ContentWidget_)
        {
            Data_->StickerText_ = data->StickerText_;
            return false;
        }

		return needUpdateLayout;
	}

	std::shared_ptr<MessageData> MessageItem::getData()
	{
		return Data_;
	}
}

namespace
{

    int32_t getAvatarRightMargin()
    {
        return Utils::scale_value(6);
    }

    int32_t getAvatarSize()
    {
        return Utils::scale_value(32);
    }

    int32_t getBubbleHorPadding()
    {
        return Utils::scale_value(16);
    }

    int32_t getLeftPadding(const bool isOutgoing)
    {
        return Utils::scale_value(
            isOutgoing ? 118 : 24
        );
    }

    int32_t getMessageTopPadding()
    {
        return Utils::scale_value(5);
    }

    int32_t getRightPadding(const bool isOutgoing)
    {
        return Utils::scale_value(
            isOutgoing ? 24 : 80
        );
    }

    int32_t getSenderHeight()
    {
        return Utils::scale_value(12);
    }

    int32_t getSenderBottomMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getStatusIconBottomPadding(const Themes::PixmapResourceId resId)
    {
        (void)resId;

        return Utils::scale_value(3);
    }

    const QFont& getTimeFont()
    {
        static QFont font(
            Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI)
        );

        font.setPixelSize(
            Utils::scale_value(12)
        );

        return font;
    }

    int32_t getTimeStatusMargin()
    {
        return Utils::scale_value(8);
    }

    int32_t getTopPadding(const bool hasTopMargin)
    {
        return Utils::scale_value(
            hasTopMargin ? 12 : 2
        );
    }

    QMap<QString, QVariant> makeData(const QString& command)
    {
        QMap<QString, QVariant> result;
        result["command"] = command;
        return result;
    }
}
