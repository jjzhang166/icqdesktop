#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"

#include "InterConnector.h"

namespace Utils
{
    InterConnector& InterConnector::instance()
    {
        static InterConnector instance = InterConnector();
        return instance;
    }

    InterConnector::InterConnector()
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
}