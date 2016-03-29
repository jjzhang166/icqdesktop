#include "stdafx.h"
#include "application.h"
#include "utils.h"
#include "log/log.h"
#include "profiling/auto_stop_watch.h"
#include "Text2DocConverter.h"
#include "../main_window/MainWindow.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../main_window/history_control/MessagesModel.h"
#include "../main_window/contact_list/RecentsModel.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../cache/emoji/Emoji.h"
#include "../themes/Themes.h"
#include "../constants.h"
#include "../app_config.h"
#include "../../gui.shared/constants.h"
#include "../../common.shared/version_info.h"

namespace
{
    const int shadow_width = 10;
}

namespace Utils
{
#ifdef _WIN32
    AppGuard::AppGuard()
        : Mutex_(0)
        , Exist_(false)
    {
        Mutex_ = ::CreateSemaphore(NULL, 0, 1, crossprocess_mutex_name);
        Exist_ = GetLastError() == ERROR_ALREADY_EXISTS;
    }

    AppGuard::~AppGuard()
    {
        if (Mutex_)
            CloseHandle(Mutex_);
    }

    bool AppGuard::succeeded() const
    {
        return !Exist_;
    }
#endif //_WIN32

    Application::Application(int argc, char *argv[])
        : QObject(0)
    {
#ifndef _WIN32
#ifdef __APPLE__
        QDir dir(argv[0]);
        assert(dir.cdUp());
        assert(dir.cdUp());
        assert(dir.cd("plugins"));
        QCoreApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#else
        std::string appDir(argv[0]);
        appDir = appDir.substr(0, appDir.rfind("/") + 1);
        appDir += "plugins";
        QCoreApplication::setLibraryPaths(QStringList(QString::fromStdString(appDir)));
#endif //__APPLE__
#endif //_WIN32

        app_.reset(new QApplication(argc, argv));

        peer_.reset(new LocalPeer(0, !isMainInstance()));
        if (isMainInstance())
            peer_->listen();
    }

    Application::~Application()
    {
        Emoji::Cleanup();
#ifndef INTERNAL_RESOURCES
        QResource::unregisterResource(QApplication::applicationDirPath() + "/qresource");
#endif
        main_window_.reset();
        Logic::ResetRecentsModel();
        Logic::ResetContactListModel();
    }

    int Application::exec()
    {
        int res = app_->exec();

        Ui::destroy_dispatcher();

        return res;
    }

    bool Application::init()
    {
        Ui::create_dispatcher();
        
        init_win7_features();

        app_->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);
#ifndef INTERNAL_RESOURCES
        QResource::registerResource(app_->applicationDirPath() + "/qresource");
#endif

        QPixmapCache::setCacheLimit(0);

        QObject::connect(Ui::GetDispatcher(), SIGNAL(guiSettings()), this, SLOT(initMainWindow()), Qt::DirectConnection);

        return true;
    }

    void Application::init_win7_features()
    {
        Utils::groupTaskbarIcon(true);
    }

    bool Application::isMainInstance()
    {
#ifdef _WIN32
        return guard_.succeeded();
#endif //_WIN32
        return true;
    }

    void Application::open_url(const QUrl& url)
    {
        QString urlStr = url.toString(QUrl::FullyEncoded);
        if (urlStr.isEmpty())
            return;

        QString decoded = url.fromPercentEncoding(urlStr.toUtf8());
        decoded.remove(QString(QChar::SoftHyphen));
        
        QDesktopServices::openUrl(decoded);
    }
    
    void Application::setUrlHandler()
    {
        QDesktopServices::setUrlHandler("http", this, "open_url");
        QDesktopServices::setUrlHandler("https", this, "open_url");
    }

    void Application::unsetUrlHandler()
    {
        QDesktopServices::unsetUrlHandler("http");
        QDesktopServices::unsetUrlHandler("https");
    }

    void Application::switchInstance()
    {
#ifdef _WIN32
        unsigned int hwnd = peer_->get_hwnd_and_activate();
        if (hwnd)
        {
            ::SetForegroundWindow((HWND) hwnd);
        }
#endif //_WIN32
    }

    void Application::initMainWindow()
    {
        double dpi = app_->primaryScreen()->logicalDotsPerInchX();
#ifdef __APPLE__
        if (QSysInfo().macVersion() >= QSysInfo().MV_10_11)
        {
            QFontDatabase().addApplicationFont("/System/Library/Fonts/SFNSDisplay-Medium.otf");
            QFontDatabase().addApplicationFont("/System/Library/Fonts/SFNSDisplay-Regular.otf");
            QFontDatabase().addApplicationFont("/System/Library/Fonts/SFNSDisplay-Light.otf");
            QFontDatabase().addApplicationFont("/System/Library/Fonts/SFNSDisplay-Thin.otf");
            QFontDatabase().addApplicationFont("/System/Library/Fonts/SFNSDisplay-Ultralight.otf");

            QFontDatabase().addApplicationFont(":/resources/fonts/OpenSans-Semibold.ttf");
            QFontDatabase().addApplicationFont(":/resources/fonts/OpenSans-Bold.ttf");
            QFontDatabase().addApplicationFont(":/resources/fonts/OpenSans-Light.ttf");
            QFontDatabase().addApplicationFont(":/resources/fonts/OpenSans-Regular.ttf");
            
#ifdef DEBUG
            qDebug() << "Installed fonts:";
            
            QStringList families = QFontDatabase().families();
            for (QStringList::iterator i = families.begin(); i != families.end(); i++)
            {
                qDebug() << (*i);
            }

            qDebug() << "Available styles for font " << Utils::appFontFamily(FontsFamily::SEGOE_UI);
            
            QStringList list = QFontDatabase().styles(Utils::appFontFamily(FontsFamily::SEGOE_UI));
            for (QStringList::iterator i = list.begin(); i != list.end(); i++)
            {
                qDebug() << (*i);
            }
#endif
        }
        
        dpi = 96;
        bool isMacRetina = false;
        isMacRetina = app_->primaryScreen()->devicePixelRatio() == 2;
        Utils::set_mac_retina(isMacRetina);
        
        if (isMacRetina)
        {
            app_->setAttribute(Qt::AA_UseHighDpiPixmaps);
        }
#endif

        setUrlHandler();

        const auto guiScaleCoefficient = std::min(dpi / 96.0, 2.0);

        Utils::init_basic_scale_coefficient(guiScaleCoefficient);
        
        Utils::set_scale_coefficient(Ui::get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::get_basic_scale_coefficient()));
        app_->setStyleSheet(Utils::LoadStyle(":/resources/qss/styles.qss", Utils::get_scale_coefficient(), true));

        Themes::SetCurrentThemeId(Themes::ThemeId::Default);
        Emoji::InitializeSubsystem();

        Ui::get_gui_settings()->set_shadow_width(Utils::scale_value(shadow_width));

        Utils::GetTranslator()->init();
        main_window_.reset(new Ui::MainWindow(app_.get()));
        main_window_->show();
        main_window_->activateWindow();
#ifdef _WIN32
        peer_->set_main_window(main_window_.get());
#endif //_WIN32
    }

    bool Application::updating()
    {
#ifdef _WIN32

        CHandle mutex(::CreateSemaphore(NULL, 0, 1, updater_singlton_mutex_name.c_str()));
        if (ERROR_ALREADY_EXISTS == ::GetLastError())
            return true;
        
        CRegKey key_software;

        if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
            return false;

        CRegKey key_product;
        if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) product_name.utf16()))
            return false;

        wchar_t version_buffer[1025];
        unsigned long len = 1024;
        if (ERROR_SUCCESS != key_product.QueryStringValue(L"update_version", version_buffer, &len))
            return false;

        QString version_update = QString::fromUtf16((const ushort*) version_buffer);

        core::tools::version_info info_current;
        core::tools::version_info info_update(version_update.toStdString());

        if (info_current < info_update)
        {
            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return false;

            QDir dir = QFileInfo(QString::fromUtf16((const ushort*) this_module_name)).absoluteDir();
            if (!dir.cdUp())
                return false;

            QString update_folder = dir.absolutePath() + "/" + updates_folder_short  + "/" + version_update;

            QDir update_dir(update_folder);
            if (update_dir.exists())
            {
                QString setup_name = update_folder + "/" + installer_exe_name;
                QFileInfo setup_info(setup_name);
                if (!setup_info.exists())
                    return false;

                mutex.Close();

                const auto command = QString("\"") + QDir::toNativeSeparators(setup_name) + "\"" + " " + update_final_command;
                QProcess::startDetached(command);

                return true;
            }
        }

#endif //_WIN32
        return false;
    }
}
