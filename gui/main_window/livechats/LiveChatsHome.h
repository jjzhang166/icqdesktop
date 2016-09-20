#pragma once

#include "../../types/chat.h"

namespace Ui
{
    class LiveChatProfileWidget;

    class LiveChatHomeWidget : public QWidget
    {
        Q_OBJECT
    public:
        LiveChatHomeWidget(QWidget* _parent, const Data::ChatInfo& _info);
        virtual ~LiveChatHomeWidget();

    protected:
        virtual void paintEvent(QPaintEvent* _e);

    private Q_SLOTS:
        void joinButtonClicked();
        void chatJoined(QString);
        void chatRemoved(QString);

    private:
        void initButtonText();

    private:
        Data::ChatInfo info_;
        LiveChatProfileWidget* profile_;
        QPushButton* joinButton_;
    };

    class LiveChatHome : public QWidget
    {
        Q_OBJECT
    public:

        LiveChatHome(QWidget* _parent);
        virtual ~LiveChatHome();

    protected:
        virtual void paintEvent(QPaintEvent* _e);

    private Q_SLOTS:
        void liveChatSelected(Data::ChatInfo);

    private:
        QVBoxLayout* layout_;
    };
}
