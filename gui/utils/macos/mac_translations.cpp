//
//  mac_translations.cpp
//  ICQ
//
//  Created by g.ulyanov on 14/06/16.
//  Copyright © 2016 Mail.RU. All rights reserved.
//

#include "stdafx.h"
#include "../../utils/utils.h"

#include "mac_translations.h"

namespace Utils
{
    const QString &Translations::Get(const QString &key)
    {
        static Translations translations_;
        if (translations_.strings_.find(key) != translations_.strings_.end())
        {
            return translations_.strings_[key];
        }
        assert(!">> TRANSLATIONS: unknown key");
        return key;
    }

    Translations::Translations()
    {
        // Main Menu
        strings_.insert(std::make_pair("&Edit", QT_TRANSLATE_NOOP("macos_menu", "&Edit")));
        strings_.insert(std::make_pair("Undo", QT_TRANSLATE_NOOP("macos_menu", "Undo")));
        strings_.insert(std::make_pair("Redo", QT_TRANSLATE_NOOP("macos_menu", "Redo")));
        strings_.insert(std::make_pair("Cut", QT_TRANSLATE_NOOP("macos_menu", "Cut")));
        strings_.insert(std::make_pair("Copy", QT_TRANSLATE_NOOP("macos_menu", "Copy")));
        strings_.insert(std::make_pair("Paste", QT_TRANSLATE_NOOP("macos_menu", "Paste")));
        strings_.insert(std::make_pair("Paste as Quote", QT_TRANSLATE_NOOP("macos_menu", "Paste as Quote")));
        strings_.insert(std::make_pair("Emoji & Symbols", QT_TRANSLATE_NOOP("macos_menu", "Emoji & Symbols")));
        strings_.insert(std::make_pair("Contact", QT_TRANSLATE_NOOP("macos_menu", "Contact")));
        strings_.insert(std::make_pair("Add Buddy", QT_TRANSLATE_NOOP("macos_menu", "Add Buddy")));
        strings_.insert(std::make_pair("Profile", QT_TRANSLATE_NOOP("macos_menu", "Profile")));
        strings_.insert(std::make_pair("Close", QT_TRANSLATE_NOOP("macos_menu", "Close")));
        strings_.insert(std::make_pair("View", QT_TRANSLATE_NOOP("macos_menu", "View")));
        strings_.insert(std::make_pair("Next Unread Message", QT_TRANSLATE_NOOP("macos_menu", "Next Unread Message")));
        strings_.insert(std::make_pair("Enter Full Screen", QT_TRANSLATE_NOOP("macos_menu", "Enter Full Screen")));
        strings_.insert(std::make_pair("Exit Full Screen", QT_TRANSLATE_NOOP("macos_menu", "Exit Full Screen")));
        strings_.insert(std::make_pair("Window", QT_TRANSLATE_NOOP("macos_menu", "Window")));
        strings_.insert(std::make_pair("Select Next Chat", QT_TRANSLATE_NOOP("macos_menu", "Select Next Chat")));
        strings_.insert(std::make_pair("Select Previous Chat", QT_TRANSLATE_NOOP("macos_menu", "Select Previous Chat")));
        strings_.insert(std::make_pair("Minimize", QT_TRANSLATE_NOOP("macos_menu", "Minimize")));
        strings_.insert(std::make_pair("Main Window", QT_TRANSLATE_NOOP("macos_menu", "Main Window")));
        strings_.insert(std::make_pair("Check For Updates...", QT_TRANSLATE_NOOP("macos_menu", "Check For Updates...")));
        strings_.insert(std::make_pair("About...", QT_TRANSLATE_NOOP("macos_menu", "About...")));
        strings_.insert(std::make_pair("Settings", QT_TRANSLATE_NOOP("macos_menu", "Settings")));
        
        //strings_.insert(std::make_pair("", QT_TRANSLATE_NOOP("macos_menu", "")));

    }
}
