#pragma once

namespace Ui
{

    class MessagesScrollbar : public QScrollBar
    {
        Q_OBJECT

    public Q_SLOTS:
        void onAutoScrollTimer();

    public:
        MessagesScrollbar(QWidget *page);

        bool canScrollDown() const;
        bool isInFetchRange(const int32_t scrollPos) const;
        void scrollToBottom();
        bool isAtBottom() const;

    private:
        QTimer *AutoscrollEnablerTimer_;

        bool CanScrollDown_;
    };

}