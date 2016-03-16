#ifndef __CALL_MENU_H__
#define __CALL_MENU_H__

namespace voip_manager {
    struct ContactEx;
    struct Contact;
}

namespace Ui {

    class callMenu;
    class CallMenu : public QMenu {
        Q_OBJECT

    Q_SIGNALS:
        void onMenuOpenChanged(bool opened);

    public:
        CallMenu(QWidget* parent);
        ~CallMenu();

        void add_widget(unsigned id, QWidget* w);
        QWidget* get_widget(unsigned id);

    protected:
        void showEvent(QShowEvent* e) override;
        void hideEvent(QHideEvent* e) override;
        void changeEvent(QEvent* e) override;
        
    private:
        std::map<unsigned, QWidget*> items_;
        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QWidget *container_;
    };
}

#endif//__CALL_MENU_H__