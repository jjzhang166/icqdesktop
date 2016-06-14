//
//  PromoPage.h
//  ICQ
//
//  Created by g.ulyanov on 24/05/16.
//  Copyright © 2016 Mail.RU. All rights reserved.
//

#ifndef ICQ_PromoPage_h
#define ICQ_PromoPage_h

namespace Ui
{
    class PromoImage: public QPushButton
    {
        Q_OBJECT

    private:
        QImage image_;
        
    protected:
        virtual void resizeEvent(QResizeEvent *e) override;
        virtual void paintEvent(QPaintEvent *e) override;
        
    public:
        explicit PromoImage(QWidget *parent = nullptr);
        ~PromoImage();
        
        void setImage(const QImage &image);
    };
    
    class PromoPageDot: public QCheckBox
    {
        Q_OBJECT
 
    protected:
        virtual void paintEvent(QPaintEvent *e) override;

    public:
        explicit PromoPageDot(QWidget *parent = nullptr);
        ~PromoPageDot();
    };
    
    class PromoPage: public QWidget
    {
        Q_OBJECT
     
    public:
        explicit PromoPage(QWidget *parent = nullptr);
        ~PromoPage();
    };
}

#endif /* ICQ_PromoPage_h */
