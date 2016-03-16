#include "stdafx.h"
#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"
#include "../utils/utils.h"
#include "../types/contact.h"
#include "VoipTools.h"

Ui::AvatarContainerWidget::AvatarContainerWidget(QWidget* parent, int avatarSize, int x_offset, int y_offset, int border_w)
    : QWidget(parent)
    , _avatarSize(avatarSize)
    , _x_offset(x_offset)
    , _y_offset(y_offset)
    , _border(NULL)
    , _overlapPer01(0.0f) {
        assert(_avatarSize > 0);

        if (border_w < _avatarSize / 2) {
            _border = new QPixmap(_avatarSize, _avatarSize);
            _border->fill(Qt::transparent);

            QPainter p(_border);
            QPainterPath path(QPoint(0, 0));
            path.addEllipse(QRect(0, 0, _avatarSize, _avatarSize));

            QPainterPath innerpath(QPoint(0, 0));
            innerpath.addEllipse(QRect(border_w, border_w, _avatarSize - 2*border_w, _avatarSize - 2*border_w));
            path = path.subtracted(innerpath);

            p.setClipPath(path);
            QBrush brush(QColor(0, 0, 0, 0.1f * 255));
            p.setPen(Qt::PenStyle::NoPen);
            p.setBrush(brush);
            p.setRenderHint(QPainter::HighQualityAntialiasing);
            p.drawEllipse(QRect(0, 0, _avatarSize, _avatarSize));
        }

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(_avatarChanged(QString)), Qt::QueuedConnection);
}

Ui::AvatarContainerWidget::~AvatarContainerWidget() {

}

void Ui::AvatarContainerWidget::setOverlap(float per01) {
    _overlapPer01 = std::max(0.0f, std::min(1.0f, per01));
}

void Ui::AvatarContainerWidget::_addAvatarTo(const std::string& userId, std::map<std::string, Logic::QPixmapSCptr>& avatars) {
    if (_avatarSize <= 0) {
        assert(!"wrong avatar size");
        return;
    }

    bool isDefault = false;
    avatars[userId] = Logic::GetAvatarStorage()->GetRounded(userId.c_str(), QString(), _avatarSize, QString(), true, isDefault);
}

void Ui::AvatarContainerWidget::_avatarChanged(QString userId) {
    std::string userIdUtf8 = userId.toStdString();
    std::map<std::string, Logic::QPixmapSCptr>::iterator it = _avatars.find(userIdUtf8);
    if (it != _avatars.end()) {
        _addAvatarTo(userIdUtf8, _avatars);
    }
}

void Ui::AvatarContainerWidget::addAvatar(const std::string& userId) {
    _addAvatarTo(userId, _avatars);

    QSize sz = _calculateAvatarSize();
    setMinimumSize(sz);

    _avatarRects = _calculateAvatarPositions(rect(), sz);
}

QSize Ui::AvatarContainerWidget::_calculateAvatarSize() {
    QSize sz;
    sz.setHeight(_avatarSize + 2*_y_offset);
    if (!_avatars.empty()) {
        sz.setWidth(_avatarSize + (int(_avatarSize * (1.0f - _overlapPer01) + 0.5f) * ((int)_avatars.size() - 1)) + 2*_x_offset);
    }
    return sz;
}

void Ui::AvatarContainerWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    QSize result;
    _avatarRects = _calculateAvatarPositions(rect(), result);
}

std::vector<QRect> Ui::AvatarContainerWidget::_calculateAvatarPositions(const QRect& rcParent, QSize& avatars_size) {
    std::vector<QRect> positions;
    _avatarRects.clear();

    if (!_avatars.empty()) {
        if (avatars_size.width() <= 0 || avatars_size.height() <= 0) {
            avatars_size = _calculateAvatarSize();
        }

        QRect remains = rcParent;
        remains.setLeft  (remains.left()   + _x_offset);
        remains.setRight (remains.right()  - _x_offset);
        remains.setTop   (remains.top()    + _y_offset);
        remains.setBottom(remains.bottom() - _y_offset);

        remains.setLeft(std::max((remains.right() + remains.left() - avatars_size.width()) / 2, remains.left()));
        remains.setRight(std::min(remains.right(), remains.left() + avatars_size.width()));
        remains.setTop(std::max((remains.bottom() + remains.top() - avatars_size.height()) / 2, remains.top()));
        remains.setBottom(std::min(remains.bottom(), remains.top() + avatars_size.height()));

        for (std::map<std::string, Logic::QPixmapSCptr>::iterator it = _avatars.begin(); it != _avatars.end(); it++) {
            if (remains.width() < _avatarSize || remains.height() < _avatarSize) {
                break;
            }

            QRect rc_draw = remains;
            rc_draw.setRight(rc_draw.left() + _avatarSize);
            positions.push_back(rc_draw);

            remains.setLeft(remains.left() + _avatarSize * (1.0f - _overlapPer01));
        }
    }

    return positions;
}

void Ui::AvatarContainerWidget::dropExcess(const std::vector<std::string>& users) {
    std::map<std::string, Logic::QPixmapSCptr> tmp_avatars;
    for (unsigned ix = 0; ix < users.size(); ix++) {
        const std::string& userId = users[ix];

        auto it = _avatars.find(userId);
        if (it != _avatars.end()) {
            tmp_avatars[userId] = it->second;
        } else {
            _addAvatarTo(userId, tmp_avatars);
        }
    }

    tmp_avatars.swap(_avatars);

    QSize sz = _calculateAvatarSize();
    setMinimumSize(sz);
    _avatarRects = _calculateAvatarPositions(rect(), sz);
}

void Ui::AvatarContainerWidget::removeAvatar(const std::string& userId) {
    _avatars.erase(userId);
}

void Ui::AvatarContainerWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    assert(_avatarRects.size() <= _avatars.size());
    for (unsigned ix = 0; ix < _avatarRects.size(); ix++) {
        const QRect& rc = _avatarRects[ix];
        
        auto avatar = _avatars.begin();
        std::advance(avatar, ix);

        Logic::QPixmapSCptr pixmap = avatar->second;
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        painter.drawPixmap(rc, *pixmap);

        if (!!_border) {
            painter.drawPixmap(rc, *_border);
        }
    }
}