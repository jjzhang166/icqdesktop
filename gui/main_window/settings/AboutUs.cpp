#include "stdafx.h"

#include "../../utils/utils.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../../common.shared/version_info_constants.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    auto scrollArea = new QScrollArea(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto scrollAreaWidget = new QWidget(scrollArea);
    scrollAreaWidget->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(scrollAreaWidget);

    auto scrollAreaLayout = new QVBoxLayout(scrollAreaWidget);
    scrollAreaLayout->setSpacing(0);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

    scrollArea->setWidget(scrollAreaWidget);

    auto layout = new QHBoxLayout(_parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);

    addHeader(scrollArea, scrollAreaLayout, QT_TRANSLATE_NOOP("settings_pages", "About ICQ"));
    {
        auto mainWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = new QHBoxLayout(mainWidget);
        mainLayout->setContentsMargins(0, Utils::scale_value(28), Utils::scale_value(48), 0);
        mainLayout->setSpacing(Utils::scale_value(32));
        mainLayout->setAlignment(Qt::AlignTop);
        {
            auto logoWidget = new QWidget(mainWidget);
            auto logoLayout = new QVBoxLayout(logoWidget);
            Utils::grabTouchWidget(logoWidget);
            logoLayout->setContentsMargins(0, 0, 0, 0);
            logoLayout->setSpacing(0);
            logoLayout->setAlignment(Qt::AlignTop);
            {
                auto logo = new QPushButton(logoWidget);
                logo->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                logo->setFixedSize(QSize(Utils::scale_value(80), Utils::scale_value(80)));
                logo->setFlat(true);
                logo->setStyleSheet("border-image: url(:/resources/main_window/content_logo_200.png);");
                logoLayout->addWidget(logo);
            }
            mainLayout->addWidget(logoWidget);
        }
        {
            auto aboutWidget = new QWidget(mainWidget);
            auto aboutLayout = new QVBoxLayout(aboutWidget);
            Utils::grabTouchWidget(aboutWidget);
            aboutLayout->setContentsMargins(0, 0, 0, 0);
            aboutLayout->setSpacing(0);
            aboutLayout->setAlignment(Qt::AlignTop);
            {
                //auto t = QString("%1 (%2 %3)").arg(QT_TRANSLATE_NOOP("settings_pages","ICQ")).arg(QT_TRANSLATE_NOOP("settings_pages","build")).arg(VERSION_INFO_STR);
                auto versionLabel = QString("%1 (%2)").arg(QT_TRANSLATE_NOOP("settings_pages", "ICQ")).arg(VERSION_INFO_STR);
                auto versionText = new TextEmojiWidget(aboutWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), -1);
                Utils::grabTouchWidget(versionText);
                versionText->setText(versionLabel);
                aboutLayout->addWidget(versionText);
            }
            {
                auto opensslLabel = new TextEmojiWidget(aboutWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(opensslLabel);
                opensslLabel->set_multiline(true);
                opensslLabel->setText(QT_TRANSLATE_NOOP("settings_pages", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
                aboutLayout->addWidget(opensslLabel);
            }
            {
                auto opensslButton = new QPushButton(aboutWidget);
                auto opensslLayout = new QVBoxLayout(opensslButton);
                opensslButton->setFlat(true);
                opensslButton->setStyleSheet("background-color: transparent;");
                opensslButton->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                opensslButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                opensslLayout->setContentsMargins(0, 0, 0, 0);
                opensslLayout->setSpacing(0);
                opensslLayout->setAlignment(Qt::AlignTop);
                {
                    auto opensslLink = new TextEmojiWidget(opensslButton, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#579e1c"), Utils::scale_value(28));
                    Utils::grabTouchWidget(opensslLink);
                    opensslLink->setText(QT_TRANSLATE_NOOP("settings_pages", "http://openssl.org"));
                    connect(opensslButton, &QPushButton::pressed, [opensslLink]()
                    {
                        QDesktopServices::openUrl(opensslLink->text());
                    });
                    opensslButton->setFixedHeight(opensslLink->height());
                    opensslLayout->addWidget(opensslLink);
                }
                aboutLayout->addWidget(opensslButton);
            }
            {
                auto voipCopyrightLabel = new TextEmojiWidget(aboutWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(voipCopyrightLabel);
                voipCopyrightLabel->set_multiline(true);
                voipCopyrightLabel->setText(QT_TRANSLATE_NOOP("settings_pages", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                aboutLayout->addWidget(voipCopyrightLabel);
            }
            {
                auto emojiLabel = new TextEmojiWidget(aboutWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(28));
                Utils::grabTouchWidget(emojiLabel);
                emojiLabel->set_multiline(true);
                emojiLabel->setText(QT_TRANSLATE_NOOP("settings_pages", "Emoji provided free by Emoji One"));
                aboutLayout->addWidget(emojiLabel);
            }
            {
                auto copyrightLabel = new TextEmojiWidget(aboutWidget, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor("#282828"), Utils::scale_value(44));
                Utils::grabTouchWidget(copyrightLabel);
                copyrightLabel->setText(QT_TRANSLATE_NOOP("settings_pages", "© ICQ LLC") + ", " + QDate::currentDate().toString("yyyy"));
                aboutLayout->addWidget(copyrightLabel);
            }
            mainLayout->addWidget(aboutWidget);
        }
        scrollAreaLayout->addWidget(mainWidget);
    }
}