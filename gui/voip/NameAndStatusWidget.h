#ifndef __NAME_AND_STATUS_WIDGET_H__
#define __NAME_AND_STATUS_WIDGET_H__
#include "../controls/TextEmojiWidget.h"

namespace Ui {

    template<typename __Base>
    class ShadowedWidget : public __Base {
    public:
        ShadowedWidget(QWidget* parent = NULL, int tailLen = 30, int alphaFrom= 255, int alphaTo = 0);
        virtual ~ShadowedWidget();

    protected:
        void paintEvent(QPaintEvent*);
        void resizeEvent(QResizeEvent*);

    private:
        QLinearGradient _linearGradient;
        int _tailLenPx;
    };

    class NameAndStatusWidget : public QWidget { Q_OBJECT
    public:
        NameAndStatusWidget(QWidget* parent, int nameBaseline, int statusBaseline);
        virtual ~NameAndStatusWidget();

        void setName  (const char* name);
        void setStatus(const char* status);

        void setNameProperty(const char* propName, bool val);
        void setStatusProperty(const char* propName, bool val);

    private:
        TextEmojiLabel* _name;
        TextEmojiLabel* _status;
    };
    
    class NameWidget : public QWidget { Q_OBJECT
    public:
        NameWidget(QWidget* parent, int nameBaseline);
        virtual ~NameWidget();
        
        void setName  (const char* name);
        
        void setNameProperty(const char* propName, bool val);
        
    private:
        TextEmojiLabel* _name;
    };
}

#endif//__NAME_AND_STATUS_WIDGET_H__