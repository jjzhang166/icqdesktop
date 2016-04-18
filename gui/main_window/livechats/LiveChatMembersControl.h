#pragma once

namespace Data
{
    class ChatInfo;
}

namespace Ui
{
    const int controlHeight = 36;
    const int avatarWithFrameHeight = controlHeight;
    const int avatarHeight = 32;
    const int maxMembersCount = 7;
    const int avatarIntersection = 10;
    const int frameWidth = (avatarWithFrameHeight - avatarHeight)/2;
    
    class LiveChatMembersControl : public QWidget
    {
        Q_OBJECT

        std::list<std::pair<QString, QString>> members_;

        const int maxCount_;

    private Q_SLOTS:

        void onAvatarLoaded(QString _aimid);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

        const int sv(const int _v);

    public:

        void adjustWidth();

        LiveChatMembersControl(QWidget* _parent, std::shared_ptr<Data::ChatInfo> _info, const int _maxCount = maxMembersCount);
        virtual ~LiveChatMembersControl();
    };

}
