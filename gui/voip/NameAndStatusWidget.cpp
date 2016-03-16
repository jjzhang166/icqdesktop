#include "stdafx.h"
#include "NameAndStatusWidget.h"
#include "VoipTools.h"
#include "../controls/TextEmojiWidget.h"
#include "../utils/utils.h"

template<typename __Base>
Ui::ShadowedWidget<__Base>::ShadowedWidget(QWidget* parent, int tailLen, int alphaFrom, int alphaTo)
: __Base(parent)
, _tailLenPx(tailLen) {

    QGradientStops stops;
    stops.append(qMakePair(0.0f, QColor(0, 0, 0, alphaFrom)));
    stops.append(qMakePair(1.0f, QColor(0, 0, 0, alphaTo)));

    _linearGradient.setStops(stops);
}

template<typename __Base>
Ui::ShadowedWidget<__Base>::~ShadowedWidget() {
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::resizeEvent(QResizeEvent* e) {
    __Base::resizeEvent(e);

    const QRect rc = __Base::rect();
    _linearGradient.setStart(rc.right() - _tailLenPx, 0.0f);
    _linearGradient.setFinalStop(rc.right(), 0.0f);
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.save();

    const QRect rc = __Base::rect();
    if (__Base::internalWidth(painter) <= rc.width()) {
        __Base::internalDraw(painter, rc);
        painter.restore();
        return;
    }

    QPixmap pixmap(QSize(rc.width(), rc.height()));
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setPen(painter.pen());
    p.setFont(painter.font());

    __Base::internalDraw(p, rc);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    p.fillRect(QRect(rc.right() - _tailLenPx + 1, 0, _tailLenPx, pixmap.rect().height()), _linearGradient);
    p.end();

    painter.drawPixmap(QPoint(rc.left(), rc.top()), pixmap);
    painter.restore();
}

Ui::NameAndStatusWidget::NameAndStatusWidget(QWidget* parent, int nameBaseline, int statusBaseline)
    : QWidget(parent)
    , _name(NULL)
    , _status(NULL) {
    setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignLeft);
    setLayout(root_layout);

    root_layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);

    _name = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    _name->setFont(font);
    _name->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    _name->setSizeToBaseline(nameBaseline); // SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(_name);

    _status = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    _status->setFont(font);
    _status->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    _status->setSizeToBaseline(statusBaseline);// SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(_status);

    root_layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

Ui::NameAndStatusWidget::~NameAndStatusWidget() {
    
}

void Ui::NameAndStatusWidget::setNameProperty(const char* propName, bool val) {
    _name->setProperty(propName, val);
}

void Ui::NameAndStatusWidget::setStatusProperty(const char* propName, bool val) {
    _status->setProperty(propName, val);
}

void Ui::NameAndStatusWidget::setName(const char* name) {
    assert(!!name);
    if (name && _name) {
        _name->setText(name);
    }
}

void Ui::NameAndStatusWidget::setStatus(const char* status) {
    assert(!!status);
    if (status && _status) {
        _status->setText(status);
    }
}

//----

Ui::NameWidget::NameWidget(QWidget* parent, int nameBaseline)
: QWidget(parent)
, _name(NULL) {
    setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout* root_layout = new QVBoxLayout();
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->setAlignment(Qt::AlignLeft);
    setLayout(root_layout);
    
    root_layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    
    _name = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    _name->setFont(font);
    _name->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    _name->setSizeToBaseline(nameBaseline); // SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(_name);
    
    root_layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

Ui::NameWidget::~NameWidget() {
    
}

void Ui::NameWidget::setNameProperty(const char* propName, bool val) {
    _name->setProperty(propName, val);
}

void Ui::NameWidget::setName(const char* name) {
    assert(!!name);
    if (name && _name) {
        _name->setText(name);
    }
}
