#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initNotifications(QWidget* _parent, std::map<std::string, Synchronizator>& _collector)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    mainWidget->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(48), 0, 0, Utils::scale_value(48));

    scrollArea->setWidget(mainWidget);

    auto layout = new QHBoxLayout(_parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
    {
        auto enableSoundsWidgets = GeneralCreator::addSwitcher(
            &_collector,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Play sounds"),
            get_gui_settings()->get_value<bool>(settings_sounds_enabled, true),
            [](bool enabled) -> QString
        {
            GetDispatcher()->getVoipController().setMuteSounds(!enabled);
            if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != enabled)
                get_gui_settings()->set_value<bool>(settings_sounds_enabled, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        auto outgoingSoundWidgets = GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Outgoing messages sound"),
            get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, false),
            [](bool enabled) -> QString
        {
            if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != enabled)
                get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        if (!get_gui_settings()->get_value(settings_sounds_enabled, true))
        {
            outgoingSoundWidgets.check_->setChecked(false);
            outgoingSoundWidgets.check_->setEnabled(false);
            outgoingSoundWidgets.text_->setEnabled(false);
        }
        connect(enableSoundsWidgets.check_, &QCheckBox::toggled, scrollArea, [enableSoundsWidgets, outgoingSoundWidgets]()
        {
            bool c = enableSoundsWidgets.check_->isChecked();
            if (!c)
            {
                outgoingSoundWidgets.check_->setChecked(false);
                outgoingSoundWidgets.check_->setEnabled(false);
                outgoingSoundWidgets.text_->setEnabled(false);
                if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != c)
                    get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, c);
            }
            else
            {
                outgoingSoundWidgets.check_->setEnabled(true);
                outgoingSoundWidgets.text_->setEnabled(true);
            }
        });
        GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Show notifications"),
            get_gui_settings()->get_value<bool>(settings_notify_new_messages, true),
            [](bool enabled) -> QString
        {
            if (get_gui_settings()->get_value<bool>(settings_notify_new_messages, true) != enabled)
                get_gui_settings()->set_value<bool>(settings_notify_new_messages, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
    }
}

