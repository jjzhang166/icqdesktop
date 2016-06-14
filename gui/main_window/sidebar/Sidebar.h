#pragma once

namespace Ui
{
    enum SidebarPages
    {
        menu_page = 0,
        profile_page = 1,
        all_members = 2,
    };

    class SidebarWidth
    {
    public:
        SidebarWidth() : width_(0) {}
        virtual ~SidebarWidth() {}
        virtual void setSidebarWidth(int width) { width_ = width; updateWidth(); }

    protected:
        virtual void updateWidth() = 0;

    protected:
        int width_;
    };

    class SidebarPage : public QWidget, public SidebarWidth
    {
    public:
        SidebarPage(QWidget* parent) : QWidget(parent) {}
        virtual ~SidebarPage() {}
        virtual void setPrev(const QString& aimId) { prevAimId_ = aimId; }

        virtual void initFor(const QString& aimId) = 0;

    protected:
        QString prevAimId_;
    };

    //////////////////////////////////////////////////////////////////////////

    class Sidebar : public QStackedWidget, public SidebarWidth
    {
        Q_OBJECT
    public:
        Sidebar(QWidget* parent);
        void preparePage(const QString& aimId, SidebarPages page);
        virtual void setSidebarWidth(int width);
        void showAllMembers();

    protected:
        virtual void updateWidth() {}

    private Q_SLOTS:
        void contactRemoved(QString);

    private:
        QMap<int, SidebarPage*> pages_;
        QString currentAimId_;
    };
}
