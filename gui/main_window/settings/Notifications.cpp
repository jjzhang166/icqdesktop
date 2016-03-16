#include "stdafx.h"

#include "../../utils/utils.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LineEditEx.h"
#include "../../utils/gui_coll_helper.h"
#include "../../gui_settings.h"
#include "../../core_dispatcher.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initNotifications(QWidget* parent, std::map<std::string, Synchronizator> &collector)
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

    addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
    {
        auto enableSoundsWidgets = addSwitcher(&collector, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Play sounds"), get_gui_settings()->get_value<bool>(settings_sounds_enabled, true), [](bool c) -> QString
        {
            GetDispatcher()->getVoipController().setMuteSounds(!c);
            if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != c)
                get_gui_settings()->set_value<bool>(settings_sounds_enabled, c);
            return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        auto outgoingSoundWidgets = addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Outgoing messages sound"), get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, false), [](bool c) -> QString
        {
            if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != c)
                get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, c);
            return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        GetDisconnector()->add("sounds/outgoing", connect(enableSoundsWidgets.check_, &QCheckBox::toggled, [enableSoundsWidgets, outgoingSoundWidgets]()
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
        }));
        addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Show notifications"), get_gui_settings()->get_value<bool>(settings_notify_new_messages, true), [](bool c) -> QString
        {
            if (get_gui_settings()->get_value<bool>(settings_notify_new_messages, true) != c)
                get_gui_settings()->set_value<bool>(settings_notify_new_messages, c);
            return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        /*
        addSwitcher(0, scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages", "Birthdays"), get_gui_settings()->get_value<bool>(settings_notify_birthdays, true), [](bool c) -> QString
        {
        if (get_gui_settings()->get_value(settings_notify_birthdays, true) != c)
        get_gui_settings()->set_value(settings_notify_birthdays, c);
        return (c ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        */
        /*
        addSwitcher(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("settings_pages","Contacts coming online"), get_gui_settings()->get_value<bool>(settings_notify_contact_appearance, false), [](bool c) -> QString
        {
        if (get_gui_settings()->get_value<bool>(settings_notify_contact_appearance, false) != c)
        get_gui_settings()->set_value<bool>(settings_notify_contact_appearance, false);
        return (c ? QT_TRANSLATE_NOOP("settings_pages","On") : QT_TRANSLATE_NOOP("settings_pages","Off"));
        });
        */

    }
}

