#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../../common.shared/version_info_constants.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
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

    GeneralCreator::addHeader(scrollArea, scrollAreaLayout, QT_TRANSLATE_NOOP("about_us", "About ICQ"));
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
                logo->setObjectName("logo");
                logo->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                logo->setFixedSize(QSize(Utils::scale_value(80), Utils::scale_value(80)));
                logo->setFlat(true);
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
                auto versionLabel = QString("%1 (%2)").arg(QT_TRANSLATE_NOOP("about_us", "ICQ")).arg(VERSION_INFO_STR);
                auto versionText = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), -1);
                Utils::grabTouchWidget(versionText);
                versionText->setText(versionLabel);
                aboutLayout->addWidget(versionText);
            }
            {
                auto opensslLabel = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(opensslLabel);
                opensslLabel->setMultiline(true);
                opensslLabel->setText(QT_TRANSLATE_NOOP("about_us", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
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
                    auto opensslLink = new TextEmojiWidget(opensslButton, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getLinkColor(), Utils::scale_value(28));
                    Utils::grabTouchWidget(opensslLink);
                    opensslLink->setText(QT_TRANSLATE_NOOP("about_us", "http://openssl.org"));
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
                auto voipCopyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(voipCopyrightLabel);
                voipCopyrightLabel->setMultiline(true);
                voipCopyrightLabel->setText(QT_TRANSLATE_NOOP("about_us", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                aboutLayout->addWidget(voipCopyrightLabel);
            }
            {
                auto emojiLabel = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(28));
                Utils::grabTouchWidget(emojiLabel);
                emojiLabel->setMultiline(true);
                emojiLabel->setText(QT_TRANSLATE_NOOP("about_us", "Emoji provided free by Emoji One"));
                aboutLayout->addWidget(emojiLabel);
            }
            {
                auto copyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(copyrightLabel);
                copyrightLabel->setText(QT_TRANSLATE_NOOP("about_us", "© ICQ LLC") + ", " + QDate::currentDate().toString("yyyy"));
                aboutLayout->addWidget(copyrightLabel);

                auto presentedMailru = new TextEmojiWidget(aboutWidget, Fonts::defaultAppFontFamily(), Fonts::defaultAppFontStyle(), Utils::scale_value(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(32));
                Utils::grabTouchWidget(presentedMailru);
                presentedMailru->setText(QT_TRANSLATE_NOOP("about_us", "Presented by Mail.Ru"));
                aboutLayout->addWidget(presentedMailru);
            }
            mainLayout->addWidget(aboutWidget);
        }
        scrollAreaLayout->addWidget(mainWidget);
    }
}