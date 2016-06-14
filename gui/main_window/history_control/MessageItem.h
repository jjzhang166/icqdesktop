#pragma once

#include "../../types/message.h"

#include "MessageItemBase.h"

namespace HistoryControl
{
	class MessageContentWidget;
}

namespace Themes
{
    enum class PixmapResourceId;
}

namespace Ui
{
	class message_item;
	class TextEditEx;
	class TextEmojiWidget;
	class PictureWidget;
	class ContextMenu;
    class MessageStatusWidget;

	class MessageData
	{
	public:
		MessageData();

        bool isOutgoing() const;

		bool AvatarVisible_;
		bool SenderVisible_;
		bool IndentBefore_;
		QDate Date_;

		bool deliveredToServer_;

		bool Chat_;
		qint64 Id_;
		QStringList NotificationsKeys_;
		QString AimId_;
		QString Sender_;
		QString Text_;
		QString StickerText_;
        QString SenderFriendly_;
		int AvatarSize_;
		qint32 Time_;

        struct IsOutgoingField
        {
            bool Outgoing_;
            bool Set_;
        } Outgoing_;
	};

	enum SelectDirection
	{
		NONE = 0,
		DOWN,
		UP,
	};

    class MessageItemLayout;

    class MessageItemsAvatars
    {
    private:
        struct Info
        {
            QString aimId_;
            QString friendlyName_;
            int size_;
            QPixmap avatar_;
            std::function<void()> callback_;
        };
        std::map<QString, MessageItemsAvatars::Info> data_;
        
    private:
        MessageItemsAvatars();
        static MessageItemsAvatars &instance();
        
    public:
        ~MessageItemsAvatars();
        
        static QPixmap &get(const QString &aimId, const QString &friendlyName, int size, const std::function<void()> &callback);
        static void reset(const QString &aimId);
    };
    
	class MessageItem : public MessageItemBase
	{
        friend class MessageItemLayout;

		Q_OBJECT

	Q_SIGNALS:
		void copy(QString);
		void quote(QString);

	public:
        MessageItem();
		MessageItem(QWidget* parent);
		~MessageItem();

		virtual QString formatRecentsText() const override;

        virtual void setContact(const QString& _aimId) override;
        virtual void setSender(const QString& _sender) override;

		void selectByPos(const QPoint& pos, bool doNotSelectImage = false);
		QString selection(bool textonly = false);

		void setId(qint64 id, const QString& aimId);
        qint64 getId() const override;

		void setNotificationKeys(const QStringList& keys);
		const QStringList& getNotificationKeys();
		void waitForAvatar(bool wait);
		void setAvatarVisible(const bool);
		void setMessage(const QString& message);
        void setMchatSenderAimId(const QString& senderAimId);
        inline const QString& getMchatSenderAimId() const { return MessageSenderAimId_; };
		void setMchatSender(const QString& sender);

		void setOutgoing(
            const bool _isOutgoing, 
            const bool _isDeliveredToServer, 
            const bool _isMChat,
            const bool _isInit = false);

        virtual bool setLastRead(const bool _isLastRead) override;

        virtual bool isOutgoing() const override;

		void setTime(const int32_t time);
		void setContentWidget(::HistoryControl::MessageContentWidget *widget);
		void setStickerText(const QString& text);
		virtual void setTopMargin(const bool value) override;
        virtual themes::themePtr theme() const override;
		void setDate(const QDate& date);
		bool selected();
		QDate date() const;
		bool isRemovable() const;
        bool isUpdateable() const;
		QString toLogString() const;
        virtual void select() override;
        virtual void clearSelection() override;
        void manualUpdateGeometry(const int32_t widgetWidth);
        void updateMenus();
        QString contentClass() const;

		void updateWith(MessageItem &messageItem);
		std::shared_ptr<MessageData> getData();

        void loadAvatar(const int size);

        virtual QSize sizeHint() const override;

    public Q_SLOTS:
        bool updateData();

	private Q_SLOTS:
		void deliveredToServer(QString);
        void readByClient(QString _aimid, qint64 _id);
        void avatarClicked();
		void menu(QAction*);

	protected:
        virtual void leaveEvent(QEvent*) override;

        virtual void mouseMoveEvent(QMouseEvent*) override;

        virtual void mousePressEvent(QMouseEvent*) override;

        virtual void mouseReleaseEvent(QMouseEvent*) override;

        virtual void paintEvent(QPaintEvent*) override;

        virtual void resizeEvent(QResizeEvent*) override;

	private:

        void createMessageBody();

        void updateMessageBodyColor();

        void updateSenderControlColor();

        void createSenderControl();

        void drawAvatar(QPainter &p);

        void drawMessageBubble(QPainter &p);

        QRect evaluateAvatarRect() const;

        QRect evaluateContentHorGeometry(const int32_t contentWidth) const;

        QRect evaluateContentVertGeometry() const;

        int32_t evaluateContentWidth(const int32_t widgetWidth) const;

        int32_t evaluateDesiredContentHeight() const;

        int32_t evaluateLeftContentMargin() const;

        int32_t evaluateRightContentMargin() const;

        int32_t evaluateTopContentMargin() const;

        const QRect& getAvatarRect() const;

        void initializeContentWidget();

        bool isAvatarVisible() const;

        bool isBlockItem() const;

        bool isOverAvatar(const QPoint &p) const;

        void updateBubbleGeometry(const QRect &bubbleGeometry);

        void updateContentWidgetHorGeometry(const QRect &bubbleHorGeometry);

        void updateMessageBodyHorGeometry(const QRect &bubbleHorGeometry);

        void updateMessageBodyFullGeometry(const QRect &bubbleRect);

        void updateSenderGeometry();

        void updateStatusGeometry(const QRect &contentGeometry);

		bool isMessageBubbleVisible() const;

        void trackContentMenu(const QPoint& _pos);
        void trackMenu(const QPoint& _pos);

		TextEditEx *MessageBody_;
		TextEmojiWidget *Sender_;
        QString MessageSenderAimId_;
		::HistoryControl::MessageContentWidget *ContentWidget_;

        bool lastRead_;

		SelectDirection Direction_;
		ContextMenu* Menu_;
        ContextMenu* ContentMenu_;

		std::shared_ptr<MessageData> Data_;

        QPainterPath Bubble_;

        mutable QRect AvatarRect_;

        bool ClickedOnAvatar_;

        bool isConnectedToDeliveryEvent_;
        bool isConnectedReadEvent_;

        MessageStatusWidget *StatusWidget_;

        MessageItemLayout *Layout_;
        int start_select_y_;
        bool is_selection_;
	};
}
