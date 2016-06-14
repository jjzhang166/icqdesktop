#include "stdafx.h"
#include "Sidebar.h"
#include "MenuPage.h"
#include "ProfilePage.h"
#include "../contact_list/ContactListModel.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    Sidebar::Sidebar(QWidget* parent)
        : QStackedWidget(parent)
    {
        pages_.insert(menu_page, new MenuPage(this));
        pages_.insert(profile_page, new ProfilePage(this));

        for (QMap<int, SidebarPage*>::iterator page = pages_.begin(); page != pages_.end(); ++page)
        {
            insertWidget(page.key(), page.value());
        }

        connect(Logic::GetContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);
    }

    void Sidebar::contactRemoved(QString aimId)
    {
    }

    void Sidebar::preparePage(const QString& aimId, SidebarPages page)
    {
        if (pages_.contains(page))
        {
            pages_[page]->initFor(aimId);
            if (width() != 0 && currentIndex() == menu_page)
                pages_[page]->setPrev(currentAimId_);
            else
                pages_[page]->setPrev(QString());

            setCurrentIndex(page);
            currentAimId_ = aimId;
        }
    }

    void Sidebar::showAllMembers()
    {
        if (currentIndex() == menu_page)
            qobject_cast<MenuPage*>(currentWidget())->allMemebersClicked();
    }

    void Sidebar::setSidebarWidth(int width)
    {
        for (auto page : pages_)
        {
            page->setSidebarWidth(width);
        }
    }
}