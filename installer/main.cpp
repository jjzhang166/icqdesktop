#include "stdafx.h"

#ifdef _WIN32
#include "ui/main_window/main_window.h"
#include "utils/styles.h"
#include <comutil.h>
#include <comdef.h>

#include "logic/tools.h"
#include "logic/worker.h"

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QICOPlugin);
using namespace installer;
const wchar_t* installer_singlton_mutex_name = L"{5686F5E0-121F-449B-ABB7-B01021D7DE65}";
const wchar_t* old_version_launch_mutex_name = L"MRAICQLAUNCH";
#endif //_WIN32

int main(int _argc, char* _argv[])
{
    int res = 0;

#ifdef _WIN32
    CHandle mutex(::CreateSemaphore(NULL, 0, 1, installer_singlton_mutex_name));
    if (ERROR_ALREADY_EXISTS == ::GetLastError())
        return res;
#endif //_WIN32


    QApplication app(_argc, _argv);

#ifdef __linux__
	QString icq_path = app.applicationDirPath() += "/icq";
	QFile::remove(icq_path);
#ifdef __x86_64__
    QFile::copy(":/bin/release64/gui/icq", icq_path);
#else
    QFile::copy(":/bin/release32/gui/icq", icq_path);
#endif
    std::string chmod("chmod 755 ");
    icq_path.replace(" ", "\\ ");
    chmod += icq_path.toStdString();
    system(chmod.c_str());
#else

    logic::get_translator()->init();

    std::unique_ptr<ui::main_window> wnd;

    {
        QStringList arguments = app.arguments();

        logic::install_config config;

        for (auto iter = arguments.cbegin(); iter != arguments.cend(); ++iter)
        {
            if (*iter == "-uninstalltmp")
            {
                config.set_uninstalltmp(true);
            }
            else if (*iter == "-uninstall")
            {
                config.set_uninstall(true);
            }
            else if (*iter == "-silent")
            {
                config.set_silent(true);
            }
            else if (*iter == "-update")
            {
                config.set_update(true);
            }
            else if (*iter == autoupdate_from_8x)
            {
                config.set_autoupdate_from_8x(true);
            }
            else if (*iter == update_final_command)
            {
                config.set_update_final(true);
            }
            else if (*iter == delete_updates_command)
            {
                config.set_delete_updates(true);
            }
            else if (*iter == nolaunch_from_8x)
            {
                config.set_nolaunch_from_8x(true);
            }
        }

        set_install_config(config);

        auto connect_to_events = [&wnd, &app]()
        {
            QObject::connect(logic::get_worker(), &logic::worker::finish, []()
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::error, [](installer::error _err)
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::select_account, [&wnd, &app]()
            {
                app.setStyleSheet(ui::styles::load_style(":/styles/styles.qss"));

                wnd.reset(new ui::main_window(ui::main_window_start_page::page_accounts));

                QIcon icon(":/images/appicon.ico");
                wnd->setWindowIcon(icon);

                wnd->show();
            });
        };

        if (logic::get_install_config().is_nolaunch_from_8x() && logic::get_install_config().is_autoupdate_from_8x())
        {
            logic::get_worker()->set_installed_flag_8x();
            logic::get_worker()->copy_self_to_bin_8x();
        }
        else if (logic::get_install_config().is_autoupdate_from_8x())
        {
#ifdef _WIN32
            CHandle mutex_8x(::CreateSemaphore(NULL, 0, 1, old_version_launch_mutex_name));
#endif //_WIN32

            connect_to_events();

            logic::get_worker()->autoupdate_from_8x();

            res = app.exec();
        }
        else if (logic::get_install_config().is_uninstalltmp())
        {
            logic::get_worker()->uninstalltmp();
        }
        else if (logic::get_install_config().is_delete_updates())
        {
            logic::get_worker()->clear_updates();
        }
        else if (logic::get_install_config().is_uninstall())
        {
            connect_to_events();

            logic::get_worker()->uninstall();

            res = app.exec();
        }
        else if (logic::get_install_config().is_update())
        {
            connect_to_events();

            logic::get_worker()->update();

            res = app.exec();
        }
        else if (logic::get_install_config().is_update_final())
        {
            CHandle mutex(::CreateSemaphore(NULL, 0, 1, updater_singlton_mutex_name.c_str()));
            if (ERROR_ALREADY_EXISTS == ::GetLastError())
                return true;

            connect_to_events();

            logic::get_worker()->update_final(mutex);

            res = app.exec();
        }
        else
        {
            app.setStyleSheet(ui::styles::load_style(":/styles/styles.qss"));

            wnd.reset(new ui::main_window());

            QIcon icon(":/images/appicon.ico");
            wnd->setWindowIcon(icon);

            wnd->show();

            res = app.exec();
        }

    }
#endif //__linux__
    return res;
}
