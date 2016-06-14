#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "../main_window/contact_list/ContactListModel.h"

#include "InterConnector.h"

namespace
{
    const QString hostIcqCom  = "icq.com";
    const QString liveChatPrefix = "/chat/";
}

namespace Utils
{
    InterConnector& InterConnector::instance()
    {
        static InterConnector instance = InterConnector();
        return instance;
    }

    InterConnector::InterConnector()
        : dragOverlay_(false)
    {
        //
    }

    InterConnector::~InterConnector()
    {
        //
    }

    void InterConnector::setMainWindow(Ui::MainWindow* window)
    {
        MainWindow_ = window;
    }

    Ui::MainWindow* InterConnector::getMainWindow() const
    {
        return MainWindow_;
    }

    Ui::HistoryControlPage* InterConnector::getHistoryPage(const QString& aimId) const
    {
        if (MainWindow_)
        {
            return MainWindow_->getHistoryPage(aimId);
        }

        return nullptr;
    }

    Ui::ContactDialog* InterConnector::getContactDialog() const
    {
        if (!MainWindow_)
        {
            return nullptr;
        }

        auto mainPage = MainWindow_->getMainPage();
        if (!mainPage)
        {
            return nullptr;
        }

        return mainPage->getContactDialog();
    }

    void InterConnector::insertTopWidget(const QString& aimId, QWidget* widget)
    {
        if (MainWindow_)
            MainWindow_->insertTopWidget(aimId, widget);
    }

    void InterConnector::removeTopWidget(const QString& aimId)
    {
        if (MainWindow_)
            MainWindow_->removeTopWidget(aimId);
    }


    void InterConnector::showSidebar(const QString& aimId, int page)
    {
        if (MainWindow_)
            MainWindow_->showSidebar(aimId, page);
    }

    void InterConnector::setSidebarVisible(bool show)
    {
        if (MainWindow_)
            MainWindow_->setSidebarVisible(show);
    }

    bool InterConnector::isSidebarVisible() const
    {
        if (MainWindow_)
            return MainWindow_->isSidebarVisible();

        return false;
    }

    void InterConnector::setDragOverlay(bool enable)
    {
        dragOverlay_ = enable;
    }

    bool InterConnector::isDragOverlay() const
    {
        return dragOverlay_;
    }

    bool InterConnector::parseLocalUrl(const QString& _urlString)
    {
        QUrl url(_urlString);

        const QString hostString = url.host(); //icq.com
        const QString path = url.path(); // /chat/novosibisk

        if (hostString == hostIcqCom && path.length() > liveChatPrefix.length() && path.mid(0, liveChatPrefix.length()) == liveChatPrefix)
        {
            QString stamp = path.mid(liveChatPrefix.length());
            if (stamp[stamp.length() - 1] == '/')
                stamp = stamp.mid(0, stamp.length() - 1);

            Logic::GetContactListModel()->joinLiveChat(stamp, false);

            return true;
        }

        return false;
    }

    void InterConnector::open_url(const QUrl& url)
    {
        QString urlStr = url.toString(QUrl::FullyEncoded);
        if (urlStr.isEmpty())
            return;

        QString decoded = url.fromPercentEncoding(urlStr.toUtf8());
        decoded.remove(QString(QChar::SoftHyphen));

        if (parseLocalUrl(decoded))
        {
            return;
        }

        QDesktopServices::openUrl(decoded);
    }

    void InterConnector::setUrlHandler()
    {
        QDesktopServices::setUrlHandler("http", this, "open_url");
        QDesktopServices::setUrlHandler("https", this, "open_url");
    }

    void InterConnector::unsetUrlHandler()
    {
        QDesktopServices::unsetUrlHandler("http");
        QDesktopServices::unsetUrlHandler("https");
    }
}