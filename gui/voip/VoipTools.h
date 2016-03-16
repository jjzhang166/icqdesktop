#ifndef __VOIP_TOOLS_H__
#define __VOIP_TOOLS_H__

namespace voipTools {
    inline void __showBounds(QPainter& painter, const QRect& rc) {
        const QPen wasPen = painter.pen();
        painter.setPen  (QColor(0, 255, 0, 255));
        painter.fillRect(QRect(rc.left(), rc.top(), rc.width() - 1, rc.height() - 1), QColor(0, 0, 255, 100));
        painter.drawRect(QRect(rc.left(), rc.left(), rc.width() - 1, rc.height() - 1));
        painter.setPen(wasPen);
    }

    template<typename __Base> class BoundBox : public __Base {
    public:
        template<typename __Param1> BoundBox(__Param1 param1) : __Base(param1) { }
        template<typename __Param1, typename __Param2> BoundBox(__Param1 param1, __Param2 param2) : __Base(param1, param2) { }
        template<typename __Param1, typename __Param2, typename __Param3, typename __Param4, typename __Param5> BoundBox(__Param1 param1, __Param2 param2, __Param3 param3, __Param4 param4, __Param5 param5) : __Base(param1, param2, param3, param4, param5) { }

    protected:
        void paintEvent(QPaintEvent* e) override {
            __Base::paintEvent(e);
#if defined(_DEBUG)
            QPainter painter(this);
            voipTools::__showBounds((painter), __Base::rect());
#endif
        }
    };

}

#endif//__VOIP_TOOLS_H__