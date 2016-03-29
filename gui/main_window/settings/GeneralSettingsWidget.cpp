#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "../../../common.shared/version_info_constants.h"

#include "../contact_list/ContactListModel.h"
#include "../contact_list/contact_profile.h"

#include "../../controls/CustomButton.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/FlatMenu.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/BackButton.h"

namespace Ui
{
    Utils::SignalsDisconnector* GetDisconnector()
    {
        static auto disconnector_ = std::make_shared<Utils::SignalsDisconnector>();
        return disconnector_.get();
    }

    const QList<QString> &getLanguagesStrings()
    {
        static QList<QString> slist;
        if (slist.isEmpty())
        {
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "ru"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "en"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "uk"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "de"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "pt"));
            slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "cs"));
            assert(slist.size() == Utils::GetTranslator()->getLanguages().size());
        }
        return slist;
    }

    QString languageToString(const QString &code)
    {
        auto codes = Utils::GetTranslator()->getLanguages();
        auto strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (1)");
        auto i = codes.indexOf(code);
        if (i >= 0 && i < codes.size())
            return strs[i];
        return "";
    }

    QString stringToLanguage(const QString &str)
    {
        auto codes = Utils::GetTranslator()->getLanguages();
        auto strs = getLanguagesStrings();
        assert(codes.size() == strs.size() && "Languages codes != Languages strings (2)");
        auto i = strs.indexOf(str);
        if (i >= 0 && i < strs.size())
            return codes[i];
        return "";
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

    void GeneralSettingsWidget::Creator::addHeader(QWidget* parent, QLayout* layout, const QString& text)
    {
        auto w = new TextEmojiWidget(parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor("#282828"), Utils::scale_value(50));
        w->setText(text);
        layout->addWidget(w);
        Utils::grabTouchWidget(w);
    }

    GeneralSettingsWidget::Creator::addSwitcherWidgets GeneralSettingsWidget::Creator::addSwitcher(std::map<std::string, Synchronizator> *collector, QWidget* parent, QLayout* layout, const QString& text, bool switched, std::function< QString(bool) > slot)
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
            connect(s, &QCheckBox::toggled, [s, slot]()
            {
                s->setText(slot(s->isChecked()));
            });
            spl->addWidget(s);
        }
        l->addWidget(sp);

        layout->addWidget(f);

        return asws;
    }

    void GeneralSettingsWidget::Creator::addChooser(QWidget* parent, QLayout* layout, const QString& info, const QString& value, std::function< void(QPushButton*) > slot)
    {
        auto f = new QWidget(parent);
        auto l = new QHBoxLayout(f);
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
        sp->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        spl->setAlignment(Qt::AlignBottom);
        spl->setContentsMargins(Utils::scale_value(5), 0, 0, 0);
        spl->setSpacing(0);
        {
            auto b = new QPushButton(sp);
            b->setStyleSheet("* { color: #579e1c; }");
            b->setFlat(true);
            b->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
            b->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            b->setText(value);
            connect(b, &QPushButton::pressed, [b, slot]()
            {
                slot(b);
            });
            spl->addWidget(b);
        }
        l->addWidget(sp);

        layout->addWidget(f);
    }

    GeneralSettingsWidget::Creator::DropperInfo GeneralSettingsWidget::Creator::addDropper(QWidget* parent, QLayout* layout, const QString& info, const std::vector< QString >& values, int selected, std::function< void(QString, int, TextEmojiWidget*) > slot1, bool isCheckable, bool switched, std::function< QString(bool) > slot2)
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
            d->setFixedSize(QSize(Utils::scale_value(280), Utils::scale_value(46)));
            d->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            dl->setContentsMargins(0, 0, 0, 0);
            dl->setSpacing(0);
            {
                auto w2 = new TextEmojiWidget(d, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(29));
                Utils::grabTouchWidget(w2);
                w2->setText((!values.empty() && selected < (int) values.size()) ? values[selected] : " ");
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
                    ln->setStyleSheet("border-width: 1; border-bottom-style: solid; border-color: #d7d7d7;");
                    lpl->addWidget(ln);
                }
                dl->addWidget(lp);

                auto m = new FlatMenu(d);
                for (auto v : values)
                    m->addAction(v);
                connect(m, &QMenu::triggered, parent, [m, aw1, w2, slot1](QAction* a)
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
            connect(c, &QCheckBox::toggled, [c, slot2]()
            {
                c->setText(slot2(c->isChecked()));
            });
            gl->addWidget(c);
        }
        layout->addWidget(g);

        return di;
    }

    void GeneralSettingsWidget::Creator::addProgresser(QWidget* parent, QLayout* layout, const std::vector< QString >& values, int selected, std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > slot)
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
            connect(p, &QSlider::valueChanged, [w, aw, slot](int v)
            {
                slot(w, aw, v);
                w->update();
                aw->update();
            });
            spl->addWidget(p);
        }
        layout->addWidget(sp);
    }

    void GeneralSettingsWidget::Creator::addBackButton(QWidget* parent, QLayout* layout, std::function<void()> _on_button_click)
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
        connect(back_button, &QPushButton::clicked, _on_button_click);
    }

    void GeneralSettingsWidget::Creator::initGeneral(QWidget* parent, std::map<std::string, Synchronizator> &collector)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        {
            addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","General settings"));

            if (platform::is_windows())
            {
                addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Launch ICQ when system starts"), Utils::is_start_on_startup(), [](bool c) -> QString
                {
                    if (Utils::is_start_on_startup() != c)
                        Utils::set_start_on_startup(c);
                    return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
                });
                addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show taskbar icon"), get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true), [](bool c) -> QString
                {
                    emit Utils::InterConnector::instance().showIconInTaskbar(c);
                    get_gui_settings()->set_value<bool>(settings_show_in_taskbar, c);
                    return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
                });
            }

            addSwitcher(&collector, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Sounds"), get_gui_settings()->get_value<bool>(settings_sounds_enabled, true), [](bool c) -> QString
            {
                Ui::GetDispatcher()->getVoipController().setMuteSounds(!c);
                if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != c)
                    get_gui_settings()->set_value<bool>(settings_sounds_enabled, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
            /*
            addSwitcher(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","Download files automatically"), get_gui_settings()->get_value<bool>(settings_download_files_automatically, true), [](bool c) -> QString
            {
            if (get_gui_settings()->get_value<bool>(settings_download_files_automatically, true) != c)
            get_gui_settings()->set_value<bool>(settings_download_files_automatically, c);
            return (c ? QT_TRANSLATE_NOOP("settings_pages","On") : QT_TRANSLATE_NOOP("settings_pages","Off"));
            });
            */
            {
                auto dp = get_gui_settings()->get_value(settings_download_directory, QString());
                if (!dp.length())
                    get_gui_settings()->set_value(settings_download_directory, Utils::DefaultDownloadsPath()); // workaround for core

            }
            addChooser(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Save to:"), QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())), [parent](QPushButton* b)
            {
#ifdef __linux__
                QWidget* parentForDialog = 0;
#else
                QWidget* parentForDialog = parent;
#endif //__linux__
                auto r = QFileDialog::getExistingDirectory(parentForDialog, QT_TRANSLATE_NOOP("settings_pages", "Choose new path"), QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())),
                    QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
                if (r.length())
                {
                    b->setText(r);
                    get_gui_settings()->set_value(settings_download_directory, QDir::toNativeSeparators(r));
                }
            });
            auto vs = Utils::get_keys_send_by_names();
            int ki = Utils::get_key_send_by_index();
            addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Send message by:"), vs, ki, [](QString v, int/* ix*/, TextEmojiWidget*)
            {
                auto p = v.split("+");
                if (p.length() != 2)
                    get_gui_settings()->set_value<int>(settings_key1_to_send_message, 0);
                else if (p.length() == 2 && (p[0] == "Ctrl" || p[0] == "Cmd"))
                    get_gui_settings()->set_value<int>(settings_key1_to_send_message, Qt::Key_Control);
                else if (p.length() == 2 && p[0] == "Shift")
                    get_gui_settings()->set_value<int>(settings_key1_to_send_message, Qt::Key_Shift);
                else if (p.length() == 2)
                    get_gui_settings()->set_value<int>(settings_key1_to_send_message, Qt::Key_Enter);
            },
                false, false, [](bool) -> QString { return ""; });
        }
        {
            addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Privacy"));
            addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show last message in recents"), get_gui_settings()->get_value<bool>(settings_show_last_message, true), [](bool c) -> QString
            {
                if (get_gui_settings()->get_value<bool>(settings_show_last_message, true) != c)
                    get_gui_settings()->set_value<bool>(settings_show_last_message, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });

            addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Preview images and videos"), get_gui_settings()->get_value<bool>(settings_show_video_and_images, true), [](bool c)
            {
                if (get_gui_settings()->get_value<bool>(settings_show_video_and_images, true) != c)
                    get_gui_settings()->set_value<bool>(settings_show_video_and_images, c);
                return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
        }
        {
            addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Interface"));
#ifndef __APPLE__
            auto i = (get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::get_basic_scale_coefficient()) - 1.f) / .25f; if (i > 3) i = 3;
            std::vector< QString > sc; sc.push_back("100"); sc.push_back("125"); sc.push_back("150"); sc.push_back("200");
            addProgresser(scroll_area, scroll_area_content_layout, sc, i, [sc](TextEmojiWidget* w, TextEmojiWidget* aw, int i) -> void
            {
                static auto su = get_gui_settings()->get_value(settings_scale_coefficient, Utils::get_basic_scale_coefficient());
                double r = sc[i].toDouble() / 100.f;
                if (fabs(get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::get_basic_scale_coefficient()) - r) >= 0.25f)
                    get_gui_settings()->set_value<double>(settings_scale_coefficient, r);
                w->setText(QString("%1 %2%").arg(QT_TRANSLATE_NOOP("settings_pages", "Interface scale:")).arg(sc[i]), QColor("#282828"));
                if (fabs(su - r) >= 0.25f)
                    aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(ICQ restart required)"), QColor("#579e1c"));
                else if (fabs(Utils::get_basic_scale_coefficient() - r) < 0.05f)
                    aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(Recommended)"), QColor("#282828"));
                else
                    aw->setText(" ", QColor("#282828"));
            });
#endif

            auto ls = getLanguagesStrings();
            auto lc = languageToString(get_gui_settings()->get_value(settings_language, QString("")));
            auto li = ls.indexOf(lc);
            addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Language:"), ls.toVector().toStdVector(), li, [scroll_area](QString v, int /*ix*/, TextEmojiWidget* ad)
            {
                static auto sl = get_gui_settings()->get_value(settings_language, QString(""));
                {
                    auto cl = stringToLanguage(v);
                    get_gui_settings()->set_value(settings_language, cl);
                    if (ad && cl != sl)
                        ad->setText(QT_TRANSLATE_NOOP("settings_pages", "(ICQ restart required)"), QColor("#579e1c"));
                    else if (ad)
                        ad->setText(" ", QColor("#282828"));
                }
            },
                false, false, [](bool) -> QString { return ""; });
        }
    }

    void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* parent, VoiceAndVideoOptions& voiceAndVideo, std::map<std::string, Synchronizator> &/*collector*/)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        auto __deviceChanged = [&voiceAndVideo] (const int ix, const voip_proxy::evoip_dev_types dev_type)
        {
            assert(ix >= 0);
            if (ix < 0) { return; }

            std::vector<DeviceInfo>* devList = NULL;
            QString settingsName;
            switch (dev_type) {
            case voip_proxy::kvoip_dev_type_audio_playback: { settingsName = settings_speakers; devList   = &voiceAndVideo.aPlaDeviceList; break; }
            case voip_proxy::kvoip_dev_type_audio_capture:  { settingsName = settings_microphone; devList = &voiceAndVideo.aCapDeviceList; break; }
            case voip_proxy::kvoip_dev_type_video_capture:  { settingsName = settings_webcam; devList     = &voiceAndVideo.vCapDeviceList; break; }
            case voip_proxy::kvoip_dev_type_undefined:
            default:
                assert(!"unexpected device type");
                return;
            };

            assert(devList);
            if (devList->empty()) { return; }

            assert(ix < (int)devList->size());
            const DeviceInfo& info = (*devList)[ix];

            voip_proxy::device_desc description;
            description.name = info.name;
            description.uid  = info.uid;
            description.dev_type = dev_type;

            Ui::GetDispatcher()->getVoipController().setActiveDevice(description);

            if (get_gui_settings()->get_value<QString>(settingsName, "") != description.uid.c_str())
                get_gui_settings()->set_value<QString>(settingsName, description.uid.c_str());
        };

        addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
        {
            std::vector< QString > vs;
            const auto di = addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Microphone:"), vs, 0, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_audio_capture);
            },
                /*
                true, get_gui_settings()->get_value<bool>(settings_microphone_gain, false), [](bool c) -> QString
                {
                if (get_gui_settings()->get_value<bool>(settings_microphone_gain, false) != c)
                get_gui_settings()->set_value<bool>(settings_microphone_gain, c);
                return QT_TRANSLATE_NOOP("settings_pages", "Gain");
                });
                */
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.audioCaptureDevices = di.menu;
            voiceAndVideo.aCapSelected = di.currentSelected;
        }
        {
            std::vector< QString > vs;
            const auto di = addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Speakers:"), vs, 0, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_audio_playback);
            },
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.audioPlaybackDevices = di.menu;
            voiceAndVideo.aPlaSelected = di.currentSelected;
        }
        {
            std::vector< QString > vs;
            const auto di = addDropper(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Webcam:"), vs, 0, [__deviceChanged](QString v, int ix, TextEmojiWidget*)
            {
                __deviceChanged(ix, voip_proxy::kvoip_dev_type_video_capture);
            },
                false, false, [](bool) -> QString { return ""; });

            voiceAndVideo.videoCaptureDevices = di.menu;
            voiceAndVideo.vCapSelected = di.currentSelected;
        }
    }

    void GeneralSettingsWidget::Creator::initThemes(QWidget* parent)
    {
        auto scroll_area = new QScrollArea(parent);
        scroll_area->setWidgetResizable(true);
        Utils::grabTouchWidget(scroll_area->viewport(), true);

        auto scroll_area_content = new QWidget(scroll_area);
        scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
        Utils::grabTouchWidget(scroll_area_content, true);

        auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
        scroll_area_content_layout->setSpacing(0);
        scroll_area_content_layout->setAlignment(Qt::AlignTop);
        scroll_area_content_layout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

        scroll_area->setWidget(scroll_area_content);

        auto layout = new QHBoxLayout(parent);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scroll_area);

        addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Wallpapers"));
    }

    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* parent):
        QStackedWidget(parent),
        general_(nullptr),
        notifications_(nullptr),
        about_(nullptr),
        contactus_(nullptr)
    {
        _voiceAndVideo.rootWidget = NULL;
        _voiceAndVideo.audioCaptureDevices = NULL;
        _voiceAndVideo.audioPlaybackDevices = NULL;
        _voiceAndVideo.videoCaptureDevices = NULL;
        _voiceAndVideo.aCapSelected = NULL;
        _voiceAndVideo.aPlaSelected = NULL;
        _voiceAndVideo.vCapSelected = NULL;

        setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss", Utils::get_scale_coefficient(), true));
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        QHBoxLayout* main_layout;
        main_layout = new QHBoxLayout();
        main_layout->setSpacing(0);
        main_layout->setContentsMargins(0, 0, 0, 0);

        std::map<std::string, Synchronizator> collector;

        general_ = new QWidget(this);
        general_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initGeneral(general_, collector);
        addWidget(general_);

        _voiceAndVideo.rootWidget = new QWidget(this);
        _voiceAndVideo.rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initVoiceVideo(_voiceAndVideo.rootWidget, _voiceAndVideo, collector);
        addWidget(_voiceAndVideo.rootWidget);

        notifications_ = new QWidget(this);
        notifications_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initNotifications(notifications_, collector);
        addWidget(notifications_);

        themes_ = new QWidget(this);
        themes_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initThemes(themes_);
        addWidget(themes_);

        about_ = new QWidget(this);
        about_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAbout(about_, collector);
        addWidget(about_);

        contactus_ = new QWidget(this);
        contactus_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initContactUs(contactus_, collector);
        addWidget(contactus_);

        attachUin_ = new QWidget(this);
        attachUin_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachUin(attachUin_, collector);
        addWidget(attachUin_);

        attachPhone_ = new QWidget(this);
        attachPhone_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachPhone(attachPhone_, collector);
        addWidget(attachPhone_);

        setCurrentWidget(general_);

        for (auto cs: collector)
        {
            auto &s = cs.second;
            for (size_t si = 0, sz = s.widgets_.size(); sz > 1 && si < (sz - 1); ++si)
                for (size_t sj = si + 1; sj < sz; ++sj)
                    connect(s.widgets_[si], s.signal_, s.widgets_[sj], s.slot_),
                    connect(s.widgets_[sj], s.signal_, s.widgets_[si], s.slot_);
        }

        auto setActiveDevice = [] (const voip_proxy::evoip_dev_types& type) {
            QString settingsName;
            switch (type) {
            case voip_proxy::kvoip_dev_type_audio_playback: { settingsName = settings_speakers;   break; }
            case voip_proxy::kvoip_dev_type_audio_capture:  { settingsName = settings_microphone; break; }
            case voip_proxy::kvoip_dev_type_video_capture:  { settingsName = settings_webcam;     break; }
            case voip_proxy::kvoip_dev_type_undefined:
            default:
                assert(!"unexpected device type");
                return;
            };

            QString val = get_gui_settings()->get_value<QString>(settingsName, "");
            if (val != "") {
                voip_proxy::device_desc description;
                description.uid      = std::string(val.toUtf8());
                description.dev_type = type;

                Ui::GetDispatcher()->getVoipController().setActiveDevice(description);
            }
        };

        setActiveDevice(voip_proxy::kvoip_dev_type_audio_playback);
        setActiveDevice(voip_proxy::kvoip_dev_type_audio_capture);
        setActiveDevice(voip_proxy::kvoip_dev_type_video_capture);

        QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), this, SLOT(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), Qt::DirectConnection);
    }

    GeneralSettingsWidget::~GeneralSettingsWidget()
    {
        GetDisconnector()->clean();
    }

    void GeneralSettingsWidget::setType(int _type)
    {
        Utils::CommonSettingsType type = (Utils::CommonSettingsType)_type;

        if (type == Utils::CommonSettingsType::CommonSettingsType_General)
        {
            setCurrentWidget(general_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo)
        {
            setCurrentWidget(_voiceAndVideo.rootWidget);
            if (devices_.empty())
                Ui::GetDispatcher()->getVoipController().setRequestSettings();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Notifications)
        {
            setCurrentWidget(notifications_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Themes)
        {
            setCurrentWidget(themes_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_About)
        {
            setCurrentWidget(about_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settings_about_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_ContactUs)
        {
            setCurrentWidget(contactus_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::feedback_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachPhone)
        {
            setCurrentWidget(attachPhone_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachUin)
        {
            setCurrentWidget(attachUin_);
        }
    }


    void GeneralSettingsWidget::onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& devices) {
        devices_ = devices;

        bool video_ca_upd = false;
        bool audio_pl_upd = false;
        bool audio_ca_upd = false;

        //const QString aCapDev = get_gui_settings()->get_value<QString>(settings_microphone, "");
        //const QString aPlaDev = get_gui_settings()->get_value<QString>(settings_speakers, "");
        //const QString vCapDev = get_gui_settings()->get_value<QString>(settings_webcam, "");

        using namespace voip_proxy;
        for (unsigned ix = 0; ix < devices_.size(); ix++) {
            const device_desc& desc = devices_[ix];

            QMenu* menu  = NULL;
            bool* flag_ptr = NULL;
            std::vector<DeviceInfo>* deviceList = NULL;
            TextEmojiWidget* currentSelected = NULL;
            //const QString* currentUID = NULL;

            switch (desc.dev_type) {
            case kvoip_dev_type_audio_capture:
                menu = _voiceAndVideo.audioCaptureDevices;
                flag_ptr = &audio_ca_upd;
                deviceList = &_voiceAndVideo.aCapDeviceList;
                currentSelected = _voiceAndVideo.aCapSelected;
                //currentUID = &aCapDev;
                break;

            case kvoip_dev_type_audio_playback:
                menu = _voiceAndVideo.audioPlaybackDevices;
                flag_ptr = &audio_pl_upd;
                deviceList = &_voiceAndVideo.aPlaDeviceList;
                currentSelected = _voiceAndVideo.aPlaSelected;
                //currentUID = &aPlaDev;
                break;

            case  kvoip_dev_type_video_capture:
                menu = _voiceAndVideo.videoCaptureDevices;
                flag_ptr = &video_ca_upd;
                deviceList = &_voiceAndVideo.vCapDeviceList;
                currentSelected = _voiceAndVideo.vCapSelected;
                //currentUID = &vCapDev;
                break;

            case kvoip_dev_type_undefined:
            default:
                assert(false);
                continue;
            }

            assert(menu && flag_ptr && deviceList);
            if (!menu || !flag_ptr || !deviceList) {
                continue;
            }

            if (!*flag_ptr)
            {
                *flag_ptr = true;
                menu->clear();
                deviceList->clear();
                if (currentSelected) {
                    currentSelected->setText(desc.name.c_str());
                }
            }

            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            menu->addAction(desc.name.c_str());
            deviceList->push_back(di);

            if ((currentSelected && desc.isActive)) {
                currentSelected->setText(desc.name.c_str());
            }
        }
    }

    void GeneralSettingsWidget::hideEvent(QHideEvent *e)
    {
        QStackedWidget::hideEvent(e);
    }

    void GeneralSettingsWidget::paintEvent(QPaintEvent* event)
    {
        QStackedWidget::paintEvent(event);

        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(geometry().x() - 1, geometry().y() - 1, visibleRegion().boundingRect().width() + 2, visibleRegion().boundingRect().height() + 2);
    }

}
