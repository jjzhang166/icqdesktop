#include "stdafx.h"
#include "CallMenu.h"

Ui::CallMenu::CallMenu(QWidget* parent)
: QMenu(parent) {
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("callMenu"));
    this->resize(133, 152);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);
    vertical_layout_ = new QVBoxLayout(this);
    vertical_layout_->setSpacing(0);
    vertical_layout_->setObjectName(QStringLiteral("verticalLayout"));
    vertical_layout_->setContentsMargins(0, 0, 0, 0);
    container_ = new QWidget(this);
    container_->setObjectName(QStringLiteral("container"));
    vertical_layout_2_ = new QVBoxLayout(container_);
    vertical_layout_2_->setObjectName(QStringLiteral("verticalLayout_2"));
    vertical_layout_2_->setContentsMargins(12, 12, 12, 12);

    vertical_layout_->addWidget(container_);
    QMetaObject::connectSlotsByName(this);

    setProperty("CallMenu", true);
#ifndef _WIN32
    setWindowFlags((Qt::WindowFlags)0xc800f009);
#else
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
#endif
}

Ui::CallMenu::~CallMenu() {

}

void Ui::CallMenu::changeEvent(QEvent* e) {
    QMenu::changeEvent(e);

    if (e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            hide();
        }
    }
}

void Ui::CallMenu::add_widget(unsigned id, QWidget* w) {
    assert(w);
    if (!w) {
        return;
    }

    if (items_.find(id) != items_.end()) {
        return;
    }

    container_->layout()->addWidget(w);
    items_[id] = w;
}

void Ui::CallMenu::showEvent(QShowEvent* e) {
    QMenu::showEvent(e);
    emit onMenuOpenChanged(true);
}

void Ui::CallMenu::hideEvent(QHideEvent* e) {
    QMenu::hideEvent(e);
    emit onMenuOpenChanged(false);
}

QWidget* Ui::CallMenu::get_widget(unsigned id) {
    auto w = items_.find(id);
    if (w == items_.end()) {
        return NULL;
    } else {
        return w->second;
    }
}