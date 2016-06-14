#include "stdafx.h"

#include "GeneralCreator.h"
#include "../utils/utils.h"
#include "TextEmojiWidget.h"
#include "../core_dispatcher.h"
#include "FlatMenu.h"
#include "BackButton.h"

namespace Ui
{
    void GeneralCreator::addHeader(QWidget* parent, QLayout* layout, const QString& text)
    {
        auto w = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(50));
        w->setText(text);
        layout->addWidget(w);
        Utils::grabTouchWidget(w);
    }

    GeneralCreator::addSwitcherWidgets GeneralCreator::addSwitcher(std::map<std::string, Synchronizator> *collector, QWidget* parent, QLayout* layout, const QString& text, bool switched, std::function< QString(bool) > slot)
    {
        addSwitcherWidgets asws;

        auto f = new QWidget(parent);
        auto l = new QHBoxLayout(f);
        l->setAlignment(Qt::AlignLeft);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);

        Utils::grabTouchWidget(f);

        auto w = new TextEmojiWidget(f, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
        asws.text_ = w;
        Utils::grabTouchWidget(w);
        w->setFixedWidth(Utils::scale_value(312));
        w->setText(text);
        w->set_multiline(true);
        l->addWidget(w);

        auto sp = new QWidget(parent);
        Utils::grabTouchWidget(sp);
        auto spl = new QVBoxLayout(sp);
        sp->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        spl->setAlignment(Qt::AlignBottom);
        spl->setContentsMargins(0, 0, 0, 0);
        spl->setSpacing(0);
        {
            auto s = new QCheckBox(sp);
            asws.check_ = s;
            s->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            s->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            s->setChecked(switched);

            GetDispatcher()->set_enabled_stats_post(false);
            s->setText(slot(s->isChecked()));
            GetDispatcher()->set_enabled_stats_post(true);

            if (collector)
            {
                auto it = collector->find("switcher/switch");
                if (it != collector->end())
                    it->second.widgets_.push_back(s);
                else
                    collector->insert(std::make_pair("switcher/switch", Synchronizator(s, SIGNAL(pressed()), SLOT(toggle()))));
            }
            QObject::connect(s, &QCheckBox::toggled, [s, slot]()
            {
                s->setText(slot(s->isChecked()));
            });
            spl->addWidget(s);
        }
        l->addWidget(sp);

        layout->addWidget(f);

        return asws;
    }

    TextEmojiWidget* GeneralCreator::addChooser(QWidget* parent, QLayout* layout, const QString& info, const QString& value, std::function< void(TextEmojiWidget*) > slot)
    {
        auto f = new QWidget(parent);
        auto l = new QHBoxLayout(f);
        f->setFixedWidth(Utils::scale_value(400));
        l->setAlignment(Qt::AlignLeft);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);

        Utils::grabTouchWidget(f);

        auto w = new TextEmojiWidget(f, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
        Utils::grabTouchWidget(w);
        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
        w->setText(info);
        l->addWidget(w);

        auto sp = new QWidget(parent);
        auto spl = new QVBoxLayout(sp);
        Utils::grabTouchWidget(sp);
        sp->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        spl->setAlignment(Qt::AlignBottom);
        spl->setContentsMargins(Utils::scale_value(5), 0, 0, 0);
        spl->setSpacing(0);

        auto b = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(44));
        {
            Utils::grabTouchWidget(b);
            b->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            b->set_ellipsis(true);
            b->setText(value);
            QObject::connect(b, &TextEmojiWidget::clicked, [b, slot]()
            {
                slot(b);
            });
            spl->addWidget(b);
        }
        l->addWidget(sp);

        layout->addWidget(f);
        return b;
    }

    GeneralCreator::DropperInfo GeneralCreator::addDropper(QWidget* parent, QLayout* layout, const QString& info, const std::vector< QString >& values, int selected, int width, std::function< void(QString, int, TextEmojiWidget*) > slot1, bool isCheckable, bool switched, std::function< QString(bool) > slot2)
    {
        TextEmojiWidget* w1 = nullptr;
        TextEmojiWidget* aw1 = nullptr;
        auto ap = new QWidget(parent);
        auto apl = new QHBoxLayout(ap);
        Utils::grabTouchWidget(ap);
        ap->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        apl->setAlignment(Qt::AlignLeft);
        apl->setContentsMargins(0, 0, 0, 0);
        apl->setSpacing(Utils::scale_value(5));
        {
            w1 = new TextEmojiWidget(ap, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(w1);
            w1->setSizePolicy(QSizePolicy::Policy::Preferred, w1->sizePolicy().verticalPolicy());
            w1->setText(info);
            apl->addWidget(w1);

            aw1 = new TextEmojiWidget(ap, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(aw1);
            aw1->setSizePolicy(QSizePolicy::Policy::Preferred, aw1->sizePolicy().verticalPolicy());
            aw1->setText(" ");
            apl->addWidget(aw1);
        }
        layout->addWidget(ap);

        auto g = new QWidget(parent);
        auto gl = new QHBoxLayout(g);
        Utils::grabTouchWidget(g);
        g->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        g->setFixedHeight(Utils::scale_value(46));
        gl->setContentsMargins(0, 0, 0, 0);
        gl->setSpacing(Utils::scale_value(32));
        gl->setAlignment(Qt::AlignLeft);

        DropperInfo di;
        di.currentSelected = NULL;
        di.menu = NULL;
        {
            auto d = new QPushButton(g);
            auto dl = new QVBoxLayout(d);
            d->setFlat(true);
            d->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            d->setFixedSize(QSize(Utils::scale_value(width == -1 ? 280 : width), Utils::scale_value(46)));
            d->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            dl->setContentsMargins(0, 0, 0, 0);
            dl->setSpacing(0);
            {
                auto w2 = new TextEmojiWidget(d, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(29));
                Utils::grabTouchWidget(w2);

                w2->setText(Utils::get_item_safe(values, selected, " "));
                w2->set_ellipsis(true);
                dl->addWidget(w2);
                di.currentSelected = w2;

                auto lp = new QWidget(d);
                auto lpl = new QHBoxLayout(lp);
                Utils::grabTouchWidget(lp);
                lpl->setContentsMargins(0, 0, 0, 0);
                lpl->setSpacing(0);
                lpl->setAlignment(Qt::AlignBottom);
                {
                    auto ln = new QFrame(lp);
                    Utils::grabTouchWidget(ln);
                    ln->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    ln->setFrameShape(QFrame::StyledPanel);
                    ln->setStyleSheet(QString("border-width: %1px; border-bottom-style: solid; border-color: #d7d7d7;").arg(Utils::scale_value(1)));
                    lpl->addWidget(ln);
                }
                dl->addWidget(lp);

                auto m = new FlatMenu(d);
                for (auto v : values)
                    m->addAction(v);
                QObject::connect(m, &QMenu::triggered, parent, [m, aw1, w2, slot1](QAction* a)
                {
                    int ix = -1;
                    QList<QAction*> allActions = m->actions();
                    for (QAction* action : allActions) {
                        ix++;
                        if (a == action) {
                            w2->setText(a->text());
                            slot1(a->text(), ix, aw1);
                            break;
                        }
                    }
                });
                d->setMenu(m);
                di.menu = m;
            }
            gl->addWidget(d);
        }
        if (isCheckable)
        {
            auto c = new QCheckBox(g);
            Utils::ApplyPropertyParameter(c, "ordinary", true);
            c->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            c->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            c->setChecked(switched);
            c->setText(slot2(c->isChecked()));
            QObject::connect(c, &QCheckBox::toggled, [c, slot2]()
            {
                c->setText(slot2(c->isChecked()));
            });
            gl->addWidget(c);
        }
        layout->addWidget(g);

        return di;
    }

    void GeneralCreator::addProgresser(QWidget* parent, QLayout* layout, const std::vector< QString >& values, int selected, std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > slot)
    {
        TextEmojiWidget* w = nullptr;
        TextEmojiWidget* aw = nullptr;
        auto ap = new QWidget(parent);
        Utils::grabTouchWidget(ap);
        auto apl = new QHBoxLayout(ap);
        ap->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        apl->setAlignment(Qt::AlignLeft);
        apl->setContentsMargins(0, 0, 0, 0);
        apl->setSpacing(Utils::scale_value(5));
        {
            w = new TextEmojiWidget(ap, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(w);
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(" ");
            apl->addWidget(w);

            aw = new TextEmojiWidget(ap, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
            Utils::grabTouchWidget(aw);
            aw->setSizePolicy(QSizePolicy::Policy::Preferred, aw->sizePolicy().verticalPolicy());
            aw->setText(" ");
            apl->addWidget(aw);

            slot(w, aw, selected);
        }
        layout->addWidget(ap);

        auto sp = new QWidget(parent);
        Utils::grabTouchWidget(sp);
        auto spl = new QVBoxLayout(sp);
        sp->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sp->setFixedWidth(Utils::scale_value(280));
        spl->setAlignment(Qt::AlignBottom);
        spl->setContentsMargins(0, Utils::scale_value(12), 0, 0);
        spl->setSpacing(0);
        {
            auto p = new SettingsSlider(Qt::Orientation::Horizontal, parent);
            Utils::grabTouchWidget(p);
            p->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            p->setFixedWidth(Utils::scale_value(280));
            p->setFixedHeight(Utils::scale_value(24));
            p->setMinimum(0);
            p->setMaximum((int)values.size() - 1);
            p->setValue(selected);
            p->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            p->setStyleSheet(QString("* QSlider::handle:horizontal { margin: %1 0 %2 0; }").arg(Utils::scale_value(-12)).arg(Utils::scale_value(-12)));
            QObject::connect(p, &QSlider::valueChanged, [w, aw, slot](int v)
            {
                slot(w, aw, v);
                w->update();
                aw->update();
            });
            spl->addWidget(p);
        }
        layout->addWidget(sp);
    }

    void GeneralCreator::addBackButton(QWidget* parent, QLayout* layout, std::function<void()> _on_button_click)
    {
        auto backbutton_area = new QWidget(parent);
        backbutton_area->setObjectName(QStringLiteral("backbutton_area"));
        backbutton_area->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        auto backbutton_area_layout = new QVBoxLayout(backbutton_area);
        backbutton_area_layout->setObjectName(QStringLiteral("backbutton_area_layout"));
        backbutton_area_layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

        auto back_button = new BackButton(backbutton_area);
        back_button->setObjectName(QStringLiteral("back_button"));
        back_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        back_button->setFlat(true);
        back_button->setFocusPolicy(Qt::NoFocus);
        back_button->setCursor(Qt::CursorShape::PointingHandCursor);
        backbutton_area_layout->addWidget(back_button);

        auto verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
        backbutton_area_layout->addItem(verticalSpacer);

        layout->addWidget(backbutton_area);
        QObject::connect(back_button, &QPushButton::clicked, _on_button_click);
    }

    SettingsSlider::SettingsSlider(Qt::Orientation orientation, QWidget *parent/* = nullptr*/):
        QSlider(orientation, parent)
    {
        //
    }

    SettingsSlider::~SettingsSlider()
    {
        //
    }

    void SettingsSlider::mousePressEvent(QMouseEvent *event)
    {
        QSlider::mousePressEvent(event);
#ifndef __APPLE__
        if (event->button() == Qt::LeftButton)
        {
            if (orientation() == Qt::Vertical)
                setValue(minimum() + ((maximum()-minimum() + 1) * (height()-event->y())) / height());
            else
                setValue(minimum() + ((maximum()-minimum() + 1) * event->x()) / width());
            event->accept();
        }
#endif // __APPLE__
    }
    
    void SettingsSlider::wheelEvent(QWheelEvent *e)
    {
        // Disable mouse wheel event for sliders
        if (parent())
        {
            parent()->event(e);
        }
    }
}
