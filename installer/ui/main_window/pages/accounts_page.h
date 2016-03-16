#pragma once

namespace installer
{
    namespace logic
    {
        struct wim_account;
    }

    namespace ui
    {
        class account_widget : public QPushButton
        {
            Q_OBJECT

        private Q_SLOTS:

        Q_SIGNALS:
            
        private:
            const logic::wim_account& account_;
            virtual void paintEvent(QPaintEvent* _e) override;
        public:
            account_widget(QWidget* _parent, const logic::wim_account& _account);
            virtual ~account_widget();
        };


        class accounts_page : public QWidget
        {
            Q_OBJECT

        private Q_SLOTS:

        Q_SIGNALS:
            void account_selected();
        private:

        public:
            accounts_page(QWidget* _parent);
            virtual ~accounts_page();
        };

    }
}


