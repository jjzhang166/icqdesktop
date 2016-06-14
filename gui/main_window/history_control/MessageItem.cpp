#include "stdafx.h"

#include "MessageStatusWidget.h"
#include "MessageItemLayout.h"
#include "MessageItem.h"
#include "MessagesModel.h"
#include "MessageStyle.h"

#include "ContentWidgets/FileSharingWidget.h"
#include "ContentWidgets/StickerWidget.h"
#include "ContentWidgets/ImagePreviewWidget.h"

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

namespace
{

    int32_t getMessageBottomPadding();

    int32_t getMessageTopPadding();

    int32_t getSenderHeight();

    int32_t getSenderBottomMargin();

	QMap<QString, QVariant> makeData(const QString& command);
}

namespace Ui
{

    MessageData::MessageData()
        : AvatarVisible_(false)
        , SenderVisible_(false)
        , IndentBefore_(false)
        , deliveredToServer_(false)
        , Id_(-1)
        , AvatarSize_(-1)
        , Time_(0)
        , Chat_(false)
    {
        Outgoing_.Outgoing_ = false;
        Outgoing_.Set_ = false;
    }

    bool MessageData::isOutgoing() const
    {
        assert(Outgoing_.Set_);

        return Outgoing_.Outgoing_;
    }

    MessageItemsAvatars::MessageItemsAvatars()
    {
        QObject::connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, [this](QString aimId)
        {
            if (MessageItemsAvatars::instance().data_.find(aimId) != MessageItemsAvatars::instance().data_.end())
            {
                bool isDefault = false;
                auto &info = MessageItemsAvatars::instance().data_[aimId];
                info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(aimId, info.friendlyName_, info.size_, QString(), true, isDefault, false).get();
                if (info.callback_)
                {
                    info.callback_();
                }
            }
        });
    }

    MessageItemsAvatars &MessageItemsAvatars::instance()
    {
        static MessageItemsAvatars instance_;
        return instance_;
    }

    MessageItemsAvatars::~MessageItemsAvatars()
    {
        //
    }

    QPixmap &MessageItemsAvatars::get(const QString &aimId, const QString &friendlyName, int size, const std::function<void()> &callback)
    {
        if (MessageItemsAvatars::instance().data_.find(aimId) == MessageItemsAvatars::instance().data_.end())
        {
            bool isDefault = false;
            MessageItemsAvatars::Info info;
            info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(aimId, friendlyName, size, QString(), true, isDefault, false).get();
            info.aimId_ = aimId;
            info.friendlyName_ = friendlyName;
            info.size_ = size;
            info.callback_ = callback;
            MessageItemsAvatars::instance().data_[aimId] = info;
        }
        return MessageItemsAvatars::instance().data_[aimId].avatar_;
    }

    void MessageItemsAvatars::reset(const QString &aimId)
    {
        if (MessageItemsAvatars::instance().data_.find(aimId) != MessageItemsAvatars::instance().data_.end())
        {
            MessageItemsAvatars::instance().data_.erase(aimId);
        }
    }

    MessageItem::MessageItem()
        : MessageItemBase(0)
        , Data_(new MessageData())
        , MessageBody_(nullptr)
        , Sender_(nullptr)
        , ContentWidget_(nullptr)
        , Direction_(NONE)
        , Menu_(0)
        , ContentMenu_(0)
        , ClickedOnAvatar_(false)
        , Layout_(nullptr)
        , StatusWidget_(nullptr)
        , isConnectedToDeliveryEvent_(false)
        , isConnectedReadEvent_(false)
        , start_select_y_(0)
        , is_selection_(false)
    {
    }

	MessageItem::MessageItem(QWidget* parent)
		: MessageItemBase(parent)
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
        , isConnectedToDeliveryEvent_(false)
        , isConnectedReadEvent_(false)
        , start_select_y_(0)
        , is_selection_(false)
    {
        QMetaObject::connectSlotsByName(this);

		setAttribute(Qt::WA_AcceptTouchEvents);

        setMouseTracking(true);

		Utils::grabTouchWidget(this);
		setFocusPolicy(Qt::NoFocus);

        assert(Layout_);
        setLayout(Layout_);

        lastRead_ = false;
	}

	MessageItem::~MessageItem()
    {
        if (Data_)
        {
            MessageItemsAvatars::reset(Data_->Sender_);
        }
	}

    void MessageItem::setContact(const QString& _aimId)
    {
        HistoryControlPageItem::setContact(_aimId);
        if (StatusWidget_)
        {
            StatusWidget_->setContact(_aimId);
        }
    }

    void MessageItem::setSender(const QString& _sender)
    {
        Data_->Sender_ = _sender;
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
		Data_->NotificationsKeys_.append(keys);
		Data_->NotificationsKeys_.removeDuplicates();
	}

	const QStringList& MessageItem::getNotificationKeys()
	{
		return Data_->NotificationsKeys_;
	}

    void MessageItem::loadAvatar(const int size)
    {
        assert(size > 0);

        if (Data_->AvatarSize_ != size)
        {
            MessageItemsAvatars::reset(Data_->Sender_);
        }

        Data_->AvatarSize_ = size;

        setAvatarVisible(true);

        update();
    }

    QSize MessageItem::sizeHint() const
    {
        auto height = evaluateTopContentMargin();

        height += evaluateDesiredContentHeight();

        if (lastRead_)
        {
            height += MessageStyle::getLastReadAvatarSize() + 2 * MessageStyle::getLastReadAvatarMargin();
        }

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
        if (isOverAvatar(e->pos()) && Data_->AvatarSize_ > 0)
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

        if (isLeftButton && isOverAvatar(e->pos()) && Data_->AvatarSize_ > 0)
        {
            ClickedOnAvatar_ = true;
        }

        QWidget::mousePressEvent(e);
    }

    void MessageItem::trackContentMenu(const QPoint& _pos)
    {
        if (ContentMenu_)
        {
            delete ContentMenu_;
            ContentMenu_ = nullptr;
        }

        ContentMenu_ = new ContextMenu(this);

        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_link_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData("copy_link"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_attach_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy file"), makeData("copy_file"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_download_100.png")), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData("save_as"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quote_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData("delete"));

        connect(ContentMenu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        const auto isChatAdmin = Logic::GetContactListModel()->isYouAdmin(Data_->AimId_);
        if (isOutgoing || isChatAdmin)
        {
            ContentMenu_->addActionWithIcon(QIcon(
                Utils::parse_image_name(":/resources/dialog_closechat_all_100.png")),
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData("delete_all"));
        }

        ContentMenu_->popup(_pos);
    }

    void MessageItem::trackMenu(const QPoint& _pos)
    {
        if (Menu_)
            delete Menu_;

        Menu_ = new ContextMenu(this);
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_copy_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy"), makeData("copy"));
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_quote_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));

        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData("delete"));

        connect(Menu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        const auto isChatAdmin = Logic::GetContactListModel()->isYouAdmin(Data_->AimId_);
        if (isOutgoing || isChatAdmin)
        {
            Menu_->addActionWithIcon(QIcon(
                Utils::parse_image_name(":/resources/dialog_closechat_all_100.png")),
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData("delete_all")
                );
        }

        Menu_->popup(_pos);
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
                trackMenu(globalPos);
            }
            else if (ContentWidget_)
            {
                if (ContentWidget_->haveContentMenu(globalPos))
                    trackContentMenu(globalPos);
                else
                    trackMenu(globalPos);
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
            e->accept();
            return;
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

        if (lastRead_)
        {
            drawLastReadAvatar(p, Data_->Sender_, Data_->SenderFriendly_, MessageStyle::getRightPadding(isOutgoing()));
        }
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
            QPalette palette;
            palette.setColor(QPalette::Text, textColor);
            MessageBody_->setPalette(palette);
        }
    }

    void MessageItem::resizeEvent(QResizeEvent *e)
    {
        HistoryControlPageItem::resizeEvent(e);
    }

    void MessageItem::createMessageBody()
    {
        if (MessageBody_)
        {
            return;
        }

        QPalette palette;
        MessageBody_ = new TextEditEx(
            this,
            Utils::FontsFamily::SEGOE_UI,
            Utils::scale_value(15),
            palette,
            false,
            false
        );
        updateMessageBodyColor();

        QString styleSheet = QString("background: transparent; selection-background-color: %1;").arg(Utils::getSelectionColor().name(QColor::HexArgb));

        MessageBody_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        MessageBody_->setFrameStyle(QFrame::NoFrame);
        MessageBody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->horizontalScrollBar()->setEnabled(false);
        MessageBody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->setOpenLinks(true);
        MessageBody_->setOpenExternalLinks(true);
        MessageBody_->setWordWrapMode(QTextOption::WordWrap);
        MessageBody_->setStyleSheet(styleSheet);
        MessageBody_->setFocusPolicy(Qt::NoFocus);
        MessageBody_->document()->setDocumentMargin(0);
        MessageBody_->setContextMenuPolicy(Qt::NoContextMenu);
        MessageBody_->setReadOnly(true);
        MessageBody_->setUndoRedoEnabled(false);
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
        if (Data_->Sender_.isEmpty() || Data_->AvatarSize_ <= 0)
        {
            return;
        }
        auto avatar = MessageItemsAvatars::get(Data_->Sender_, Data_->SenderFriendly_, Data_->AvatarSize_, [this](){ parentWidget()->update(); });
        if (avatar.isNull() || !Data_->AvatarVisible_)
        {
            return;
        }

        auto rect = getAvatarRect();
        if (rect.isValid())
            p.drawPixmap(getAvatarRect(), avatar);
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

        p.fillPath(Bubble_, MessageStyle::getBodyBrush(Data_->isOutgoing(), isSelected(), theme()->get_id()));
    }

    QRect MessageItem::evaluateAvatarRect() const
    {
        assert(!AvatarRect_.isValid());

        QRect result(
            MessageStyle::getLeftPadding(isOutgoing()),
            evaluateTopContentMargin(),
            MessageStyle::getAvatarSize(),
            MessageStyle::getAvatarSize()
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
        auto height = MessageStyle::getBubbleHeight();

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

                height = std::max<int32_t>(Utils::applyMultilineTextFix(textHeight, height), MessageStyle::getBubbleHeight());
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
        auto leftContentMargin = MessageStyle::getLeftPadding(isOutgoing());

        if (isAvatarVisible())
        {
            leftContentMargin += MessageStyle::getAvatarSize();
            leftContentMargin += MessageStyle::getAvatarRightMargin();
        }

        return leftContentMargin;
    }

    int32_t MessageItem::evaluateRightContentMargin() const
    {
        auto rightContentMargin = MessageStyle::getRightPadding(isOutgoing());

        return rightContentMargin;
    }

    int32_t MessageItem::evaluateTopContentMargin() const
    {
        auto topContentMargin = MessageStyle::getTopPadding(hasTopMargin());

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

    void MessageItem::initializeContentWidget()
    {
        if (!ContentWidget_)
        {
            return;
        }

        ContentWidget_->initialize();
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
        if (!Data_)
            return false;

        return Data_->isOutgoing();
    }

    bool MessageItem::isOverAvatar(const QPoint &p) const
    {
        return (
            isAvatarVisible() &&
            getAvatarRect().contains(p)
        );
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

        initializeContentWidget();
    }


    QString MessageItem::contentClass() const
    {
        if (MessageBody_)
        {
            static const auto N_OF_LEFTMOST_CHARS = 5;

            const auto &text = Data_->Text_;

            QString result;
            result.reserve(64);

            result.append("text(");
            result.append(text.left(N_OF_LEFTMOST_CHARS));

            if (text.length() > N_OF_LEFTMOST_CHARS)
            {
                result.append("...");
            }

            result.append(")");

            return result;
        }

        return ContentWidget_->metaObject()->className();
    }

    void MessageItem::updateBubbleGeometry(const QRect &bubbleGeometry)
    {
        assert(!bubbleGeometry.isEmpty());

        Bubble_ = Utils::renderMessageBubble(bubbleGeometry, MessageStyle::getBorderRadius(), Data_->isOutgoing());
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
            left += MessageStyle::getBubbleHorPadding();
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
            width -= MessageStatusWidget::getMaxWidth();
            width -= MessageStyle::getTimeStatusMargin();
            width -= MessageStyle::getTimeStatusMargin();
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
        messageBodyWidth -= (MessageStyle::getBubbleHorPadding() * 2);
        messageBodyWidth -= MessageStatusWidget::getMaxWidth();

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
            bubbleRect.left() + MessageStyle::getBubbleHorPadding(),
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
            MessageStyle::getLeftPadding(isOutgoing()),
            MessageStyle::getTopPadding(hasTopMargin())
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
                MessageStatusWidget::getMaxWidth() +
                MessageStyle::getTimeStatusMargin()
            );

            if (ContentWidget_->hasTextBubble())
            {
                // right margin
                fullStatusLineWidth += MessageStyle::getTimeStatusMargin();
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
            statusX += MessageStyle::getTimeStatusMargin();
        }
        else
        {
            statusX = contentGeometry.right();

            statusX -= MessageStatusWidget::getMaxWidth();
            statusX -= MessageStyle::getTimeStatusMargin();
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

        statusY -= MessageStyle::getTimeStatusMargin();
        statusY -= statusWidgetSize.height();

        const QRect statusGeometry(
            statusX,
            statusY,
            MessageStatusWidget::getMaxWidth(),
            statusWidgetSize.height()
        );

        StatusWidget_->setGeometry(statusGeometry);
    }

	void MessageItem::selectByPos(const QPoint& pos, bool doNotSelectImage)
	{
        if (!is_selection_)
        {
            is_selection_ = true;
            start_select_y_ = pos.y();
        }

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
		if (isCursorOverWidget && !selected && !doNotSelectImage)
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

			if (needToClear && (start_select_y_ < widgetRect.top() || start_select_y_ > widgetRect.bottom()))
            {
				clearSelection();
                is_selection_ = true;
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
        is_selection_ = false;
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

        auto format = (
            (command == "copy") ?
                QString() :
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
        else if ((command == "delete_all") || (command == "delete"))
        {
            const auto isPendingMessage = (Data_->Id_ <= -1);
            if (isPendingMessage)
            {
                return;
            }

            const auto is_shared = (command == "delete_all");
            const auto ids = Logic::GetMessagesModel()->getBubbleMessageIds(Data_->AimId_, Data_->Id_);
            GetDispatcher()->delete_messages(ids, Data_->AimId_, is_shared);
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

        setUpdatesEnabled(false);

        createMessageBody();

		MessageBody_->verticalScrollBar()->blockSignals(true);

        MessageBody_->document()->clear();

		Logic::Text4Edit(message, *MessageBody_, Logic::Text2DocHtmlMode::Escape, true, true);

        MessageBody_->verticalScrollBar()->blockSignals(false);

        MessageBody_->setVisible(true);

        Layout_->setDirty();

        setUpdatesEnabled(true);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateGeometry();
        update();
	}

    bool MessageItem::setLastRead(const bool _isLastRead)
    {
        HistoryControlPageItem::setLastRead(_isLastRead);

        if (_isLastRead != lastRead_)
        {
            lastRead_ = _isLastRead;

            Layout_->setDirty();

            updateGeometry();

            return true;
        }

        return false;
    }

	void MessageItem::setOutgoing(const bool _isOutgoing, const bool _isDeliveredToServer, const bool _isMChat, const bool _isInit)
	{
        Data_->Outgoing_.Outgoing_ = _isOutgoing;
        Data_->Outgoing_.Set_ = true;
        Data_->Chat_ = _isMChat;

        if (_isOutgoing && (Data_->deliveredToServer_ != _isDeliveredToServer || _isInit))
        {
            Data_->deliveredToServer_ = _isDeliveredToServer;

            if (!_isDeliveredToServer)
            {
                QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
                opacityEffect->setOpacity(0.5);
                setGraphicsEffect(opacityEffect);

                if (!_isInit)
                    update();

                isConnectedToDeliveryEvent_ = connect(Logic::GetMessagesModel(), SIGNAL(deliveredToServer(QString)), this, SLOT(deliveredToServer(QString)), Qt::QueuedConnection);
            }
            else
            {
                auto effect = graphicsEffect();
                if (effect)
                {
                    setGraphicsEffect(nullptr);
                    if (!_isInit)
                        update();
                }

                if (isConnectedToDeliveryEvent_)
                {
                    disconnect(Logic::GetMessagesModel(), SIGNAL(deliveredToServer(QString)), this, SLOT(deliveredToServer(QString)));
                    isConnectedToDeliveryEvent_ = false;
                }
            }
        }
	}

    void MessageItem::readByClient(QString _aimid, qint64 _id)
    {
        if (_aimid != Data_->AimId_)
            return;

        if (Data_->Id_ != _id)
        {
            setLastRead(false);
        }

        return;
    }

	void MessageItem::deliveredToServer(QString key)
	{
		assert(!key.isEmpty());

		__TRACE(
			"delivery",
			"status event reached a widget\n" <<
			"	status=<server>\n" <<
			"	key=<" << key <<">");

		if (!Data_ || !Data_->NotificationsKeys_.contains(key))
		{
			return;
		}

        if (Data_->deliveredToServer_)
            return;
        
		setOutgoing(Data_->isOutgoing(), true, Data_->Chat_);
		setTime(Data_->Time_);
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
        const auto topMarginUpdated = (hasTopMargin() != value);
        if (topMarginUpdated)
        {
            AvatarRect_ = QRect();
        }

        Data_->IndentBefore_ = value;

        HistoryControlPageItem::setTopMargin(value);
	}

    themes::themePtr MessageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(Data_.get() ? Data_->AimId_ : aimId_);
    }

	void MessageItem::setContentWidget(HistoryControl::MessageContentWidget *widget)
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

    bool MessageItem::isRemovable() const
	{
		return (
            ContentWidget_ ?
                ContentWidget_->canUnload() :
                true
        );
	}

    bool MessageItem::isUpdateable() const
    {
        return (
            ContentWidget_ ?
                ContentWidget_->canReplace() :
                true
        );
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
            StatusWidget_->setMessageBubbleVisible(isMessageBubbleVisible());
        }
        update();
        return true;
    }

	void MessageItem::updateWith(MessageItem &messageItem)
	{
        auto &data = messageItem.Data_;
        assert(data);
        assert(Data_ != data);

		Data_ = std::move(data);
        aimId_ = Data_->AimId_;

        const auto updateNotificationKeys = (Data_->isOutgoing());
		if (updateNotificationKeys)
        {
            setNotificationKeys(Data_->NotificationsKeys_);
        }

        if (messageItem.MessageBody_)
        {
            assert(!Data_->Text_.isEmpty());
            assert(!messageItem.ContentWidget_);

            if (ContentWidget_)
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;
            }

            setMessage(Data_->Text_);
        }

        if (messageItem.ContentWidget_)
        {
            assert(Data_->Text_.isEmpty());
            assert(!messageItem.MessageBody_);

            if (MessageBody_)
            {
                MessageBody_->deleteLater();
                MessageBody_ = nullptr;
            }

            if (ContentWidget_ && ContentWidget_->canReplace())
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;

                setContentWidget(messageItem.ContentWidget_);
                messageItem.ContentWidget_ = nullptr;
            }
        }

		setTime(Data_->Time_);
		setOutgoing(Data_->isOutgoing(), Data_->deliveredToServer_, Data_->Chat_);
		setTopMargin(Data_->IndentBefore_);

		if (Data_->AvatarVisible_)
        {
            loadAvatar(Utils::scale_bitmap(MessageStyle::getAvatarSize()));
        }
		else
        {
			setAvatarVisible(false);
        }

        setMchatSenderAimId(Data_->Sender_);
        setMchatSender(Data_->SenderFriendly_);
	}

	std::shared_ptr<MessageData> MessageItem::getData()
	{
		return Data_;
	}
}

namespace
{

    int32_t getMessageTopPadding()
    {
        return Utils::scale_value(4);
    }

    int32_t getSenderHeight()
    {
        return Utils::scale_value(14);
    }

    int32_t getSenderBottomMargin()
    {
        return Utils::scale_value(4);
    }

    QMap<QString, QVariant> makeData(const QString& command)
    {
        QMap<QString, QVariant> result;
        result["command"] = command;
        return result;
    }
}
