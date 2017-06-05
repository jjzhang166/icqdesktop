#pragma once

#include "../../cache/avatars/AvatarStorage.h"
#include "../../types/snap.h"

namespace Ui
{
    class CustomButton;
    class SearchWidget;

    class MyMailWidget : public QWidget
    {
        Q_OBJECT

    public:
        MyMailWidget(QWidget* parent);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);
        virtual void enterEvent(QEvent *);
        virtual void leaveEvent(QEvent *);

    private Q_SLOTS:
        void mailStatus(QString, unsigned, bool);
        void mrimKey(qint64, QString);

    private:
        void updateSize();

    private:
        bool Hovered_;
        unsigned Unreads_;
        QString Email_;
        int64_t LastSeq_;
    };

    class BurgerWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();
        void back();

    public:
        BurgerWidget(QWidget* parent);
        void setBack(bool _back);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        bool Back_;
    };

    class TopPanelWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void back();
        void burgerClicked();
        void discoverClicked();
        
    public:
        enum Mode
        {
            NORMAL = 0,
            COMPACT = 1,
            SPREADED = 2,
        };

        TopPanelWidget(QWidget* parent, SearchWidget* searchWidget);

        void setMode(Mode _mode);
        void setBack(bool _back);
        void searchActivityChanged(bool _active);


    protected:
        virtual void paintEvent(QPaintEvent* _e) override;

    private Q_SLOTS:
        void infoUpdated();

    private:
        BurgerWidget* Burger_;
        MyMailWidget* Mail_;
        QWidget* LeftSpacer_;
        QWidget* RightSpacer_;
        QHBoxLayout* mainLayout;
        CustomButton* Discover_;
        SearchWidget* Search_;
        Mode Mode_;
    };
}
