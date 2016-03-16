#pragma once

namespace Ui
{
    class MenuStyle: public QProxyStyle
    {
        Q_OBJECT
    public:
        virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;
    };

   class ContextMenu : public QMenu
   {
       Q_OBJECT
   public:
       ContextMenu(QWidget* parent);

       QAction* addActionWithIcon(const QIcon& icon, const QString& name, const QVariant& data);
       QAction* addActionWithIcon(const QIcon& icon, const QString& name, const QObject *receiver, const char* member);

       void invertRight(bool invert);
       void setIndent(int indent);

	   void popup(const QPoint &pos, QAction *at=0);

   protected:
       virtual void showEvent(QShowEvent *e);

   private:
       bool InvertRight_;
       int Indent_;
	   QPoint Pos_;
   };
}