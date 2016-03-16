#include "stdafx.h"

#include "../../utils/utils.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../../common.shared/version_info_constants.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/)
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

    addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "About ICQ"));
    {
        auto p = new QWidget(scroll_area);
        Utils::grabTouchWidget(p);
        auto pl = new QHBoxLayout(p);
        pl->setContentsMargins(0, Utils::scale_value(28), Utils::scale_value(48), 0);
        pl->setSpacing(Utils::scale_value(32));
        pl->setAlignment(Qt::AlignTop);
        {
            auto sp = new QWidget(p);
            auto spl = new QVBoxLayout(sp);
            Utils::grabTouchWidget(sp);
            spl->setContentsMargins(0, 0, 0, 0);
            spl->setSpacing(0);
            spl->setAlignment(Qt::AlignTop);
            {
                auto i = new QPushButton(sp);
                i->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                i->setFixedSize(QSize(Utils::scale_value(80), Utils::scale_value(80)));
                i->setFlat(true);
                i->setStyleSheet("border-image: url(:/resources/main_window/content_logo_200.png);");
                spl->addWidget(i);
            }
            pl->addWidget(sp);
        }
        {
            auto sp = new QWidget(p);
            auto spl = new QVBoxLayout(sp);
            Utils::grabTouchWidget(sp);
            spl->setContentsMargins(0, 0, 0, 0);
            spl->setSpacing(0);
            spl->setAlignment(Qt::AlignTop);
            {
                //auto t = QString("%1 (%2 %3)").arg(QT_TRANSLATE_NOOP("settings_pages","ICQ")).arg(QT_TRANSLATE_NOOP("settings_pages","build")).arg(VERSION_INFO_STR);
                auto t = QString("%1 (%2)").arg(QT_TRANSLATE_NOOP("settings_pages", "ICQ")).arg(VERSION_INFO_STR);
                auto w = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), -1);
                Utils::grabTouchWidget(w);
                w->setText(t);
                spl->addWidget(w);
            }
            {
                auto w = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(w);
                w->set_multiline(true);
                w->setText(QT_TRANSLATE_NOOP("settings_pages", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
                spl->addWidget(w);
            }
            {
                auto b = new QPushButton(sp);
                auto bl = new QVBoxLayout(b);
                b->setFlat(true);
                b->setStyleSheet("background-color: transparent;");
                b->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                b->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                bl->setContentsMargins(0, 0, 0, 0);
                bl->setSpacing(0);
                bl->setAlignment(Qt::AlignTop);
                {
                    auto w = new TextEmojiWidget(b, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(28));
                    Utils::grabTouchWidget(w);
                    w->setText(QT_TRANSLATE_NOOP("settings_pages", "http://openssl.org"));
                    connect(b, &QPushButton::pressed, [w]()
                    {
                        QDesktopServices::openUrl(w->text());
                    });
                    b->setFixedHeight(w->height());
                    bl->addWidget(w);
                }
                spl->addWidget(b);
            }
            {
                auto w = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(w);
                w->set_multiline(true);
                w->setText(QT_TRANSLATE_NOOP("settings_pages", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                spl->addWidget(w);
            }
            {
                auto w = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(28));
                Utils::grabTouchWidget(w);
                w->set_multiline(true);
                w->setText(QT_TRANSLATE_NOOP("settings_pages", "Emoji provided free by Emoji One"));
                spl->addWidget(w);
            }
            {
                auto w = new TextEmojiWidget(sp, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(w);
                w->setText(QT_TRANSLATE_NOOP("settings_pages", "© ICQ LLC") + ", " + QDate::currentDate().toString("yyyy"));
                spl->addWidget(w);
            }
            pl->addWidget(sp);
        }
        scroll_area_content_layout->addWidget(p);
    }
}