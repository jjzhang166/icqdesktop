#pragma once

namespace Themes
{
    enum class PixmapResourceId;
}

namespace Ui
{

    class HistoryControlPageItem;

    class MessageStatusWidget : public QWidget
    {
    public:
        static int32_t getMaxWidth(const bool withStatusIcon);

        MessageStatusWidget(HistoryControlPageItem *messageItem);

        void setDeliveredToClient();

        void setDeliveredToServer();

        void setTime(const int32_t timestamp);

        virtual QSize sizeHint() const override;
        
        void setContact(const QString _aimId) { aimId_ = _aimId; }
        
        QColor getTimeColor() const;
        
        void setMessageBubbleVisible(const bool _visible);

    protected:
        virtual void paintEvent(QPaintEvent *) override;

    private:
        bool IsOutgoing_;

        bool IsDeliveredToServer_;

        bool IsDeliveredToClient_;
        
        bool IsMessageBubbleVisible_;

        Themes::PixmapResourceId StatusIcon_;
        
        QString aimId_;

        QString TimeText_;

        QSize TimeTextSize_;

    };

}