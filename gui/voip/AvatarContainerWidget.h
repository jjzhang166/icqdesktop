#ifndef __AVATAR_CONTAINER_WIDGET_H__
#define __AVATAR_CONTAINER_WIDGET_H__

#include <string>
#include "../cache/avatars/AvatarStorage.h"

namespace Ui {
    
    class AvatarContainerWidget : public QWidget { Q_OBJECT
    Q_SIGNALS:

    public:
        AvatarContainerWidget(QWidget* parent, int avatarSize, int x_offset = 0, int y_offset = 0, int border_w = 0);
        virtual ~AvatarContainerWidget();

        void setOverlap(float per01);
        void addAvatar(const std::string& userId);
        void removeAvatar(const std::string& userId);
        void dropExcess(const std::vector<std::string>& users);

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    private Q_SLOTS:
        void _avatarChanged(QString);

    private:
        std::map<std::string, Logic::QPixmapSCptr> _avatars;
        std::vector<QRect> _avatarRects;
        float _overlapPer01;

        int _avatarSize;
        int _x_offset; 
        int _y_offset;
        //int _border_w;
        QPixmap* _border;

        void _addAvatarTo(const std::string& userId, std::map<std::string, Logic::QPixmapSCptr>& avatars);
        std::vector<QRect> _calculateAvatarPositions(const QRect& rcParent, QSize& avatars_size);
        QSize _calculateAvatarSize();
    };

}

#endif//__AVATAR_CONTAINER_WIDGET_H__