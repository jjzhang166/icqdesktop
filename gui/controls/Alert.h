#pragma once

#ifndef __ICQ__Alert__
#define __ICQ__Alert__

namespace Ui
{
    class Alert: protected QDialog
    {
        Q_OBJECT

    private:
        bool withShadow_;
        QLabel *title_;
        QWidget *content_;
        QVBoxLayout *contentArea_;
        QWidget *buttons_;
        QHBoxLayout *buttonsArea_;

    private:
        explicit Alert(bool setShadowBackground = true);

    public:
        static void setMainWindow(QWidget *mainWindow);
        static Alert *create();
        
        ~Alert();

        void show();
        void hide();
        
        void setText(const QString &text);
        void addButton(const QString &title, const QString &accessible_name, bool isDefault, const std::function<void(QPushButton *)> &action);
        void addWidget(QWidget *userContent);

    private:
        void closeEvent(QCloseEvent *e) override;
        void showEvent(QShowEvent *e) override;
        void keyPressEvent(QKeyEvent *e) override;
        
    public:
        /* Common alerts. */
        static void notify(const QString &notification, const QString &okText = "OK");
        static int custom(const QString &notification, QWidget *userContent, const QVector<QString> &buttons, int defaultButtonIndex = -1);
    };
}

#endif /* defined(__ICQ__Alert__) */
