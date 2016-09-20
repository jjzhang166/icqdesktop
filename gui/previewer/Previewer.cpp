#include "stdafx.h"

#include "Previewer.h"

#include "../main_window/MainWindow.h"
#include "../utils/InterConnector.h"
#include "../../corelib/enumerations.h"
#include "../../gui.shared/implayer.h"
#include "../utils/utils.h"

namespace
{
	std::unique_ptr<QWidget> PreviewWidget_;
    std::unique_ptr<QWidget> g_multimediaViewer;
}

const char* mplayer_exe = "mplayer.exe";

namespace Previewer
{
    void ShowMedia(const core::file_sharing_content_type /*_contentType*/, const QString& _path)
    {
        if (platform::is_windows())
        {
//             const auto exePath = QCoreApplication::applicationFilePath();
// 
//             const auto forder = QFileInfo(exePath).path();
// 
//             const int scale = Utils::scale_value(100);
// 
//             const int screenNumber = Utils::InterConnector::instance().getMainWindow()->getScreen();
// 
//             const QString command = "\"" + forder + "/" + QString(mplayer_exe) + "\"" + QString(" /media \"") + _path + "\"" + " /scale " + QString::number(scale) + " /screen_number " + QString::number(screenNumber);
// 
//             QProcess::startDetached(command);

        }
        else
        {
            QUrl url = QUrl::fromLocalFile(_path);
            if (!url.isLocalFile() || !platform::is_apple())
                url = QUrl(QDir::fromNativeSeparators(_path));
            QDesktopServices::openUrl(url);
        }
    }

    void CloseMedia()
    {
        g_multimediaViewer.reset();
    }
}

namespace
{

}