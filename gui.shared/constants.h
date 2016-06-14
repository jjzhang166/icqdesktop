#pragma once

#if defined (_WIN32)
#define CORELIBRARY "corelib.dll"
#elif defined (__APPLE__)
#define CORELIBRARY "libcorelib.dylib"
#elif defined (__linux__)
#define CORELIBRARY "libcorelib.so"
#endif

const QString product_name = "icq.desktop";
const QString updates_folder_short = "updates";
const QString installer_exe_name = "icqsetup.exe";
const QString update_final_command = "-update_final";
const QString delete_updates_command = "-delete_updates";
const QString autoupdate_from_8x = "-autoupdate";
const QString nolaunch_from_8x = "-nolaunch";
const QString send_dump_arg = "-send_dump";

const std::wstring updater_singlton_mutex_name = L"{D7364340-9348-4397-9F56-73CE62AAAEA8}";

namespace Ui
{
    enum KeyToSendMessage
    {
        Enter = 0x0,
        Shift_Enter = 0x01000020,
        Ctrl_Enter = 0x01000021
    };
}
