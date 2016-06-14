#ifndef __SECURE_CALL_WND_H__
#define __SECURE_CALL_WND_H__
#include "../controls/TextEmojiWidget.h"

namespace Ui {

    class ImageContainer : public QWidget
    { Q_OBJECT

    public:
        ImageContainer(QWidget* parent);
        virtual ~ImageContainer();

    public:
        void swapImagePack(std::vector<std::shared_ptr<QImage> >& images);
        void setKerning(int kerning);

    private:
        void calculateRectDraw();

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent *) override;

    private:
        std::vector<std::shared_ptr<QImage> > images_;
        int kerning_;
        QRect rcDraw_;
    };

    class SecureCallWnd : public QMenu 
    { Q_OBJECT

        QVBoxLayout* rootLayout_;
        QWidget*     rootWidget_;
        ImageContainer* textSecureCode_;

    public:
        SecureCallWnd(QWidget* parent = NULL);
        virtual ~SecureCallWnd();

        void setSecureCode(const std::string& text);

    private:
        void showEvent  (QShowEvent* e) override;
        void hideEvent  (QHideEvent* e) override;
        void changeEvent(QEvent* e)     override;

        QLabel* createUniformLabel_(const QString& text, const unsigned fontSize, QSizePolicy policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        void updateMask();

    Q_SIGNALS:
        void onSecureCallWndOpened();
        void onSecureCallWndClosed();

    private Q_SLOTS:
        void onBtnOkClicked();
        void onDetailsButtonClicked();
    };

}

#endif//__SECURE_CALL_WND_H__