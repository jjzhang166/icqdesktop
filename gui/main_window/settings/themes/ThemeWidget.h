#ifndef ThemeWidget_h
#define ThemeWidget_h

namespace Ui
{
    class ThemesModel;
    class ThemeWidget : public QWidget
    {
        Q_OBJECT
        QPixmap pixmap_;
        int theme_id_;
        ThemesModel* themesModel_;
        QWidget *borderWidget_;
    public:
        ThemeWidget(QWidget* parent, QPixmap& pixmap, ThemesModel* _themesModel, int _theme_id);
        int get_id() const { return theme_id_; }
        void setBorder(const bool border);
    private Q_SLOTS:
        void on_theme_pressed();
    };
}

#endif /* ThemeWidget_h */
