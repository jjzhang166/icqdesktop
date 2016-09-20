#ifndef ThemeWidget_h
#define ThemeWidget_h

namespace Ui
{
    class ThemesModel;
    class ThemeWidget : public QWidget
    {
        Q_OBJECT
        QPixmap pixmap_;
        int themeId_;
        ThemesModel* themesModel_;
        QWidget *borderWidget_;
    public:
        ThemeWidget(QWidget* _parent, QPixmap& _pixmap, ThemesModel* _themesModel, int _themeId);
        int get_id() const { return themeId_; }
        void setBorder(const bool border);
    private Q_SLOTS:
        void onThemePressed();
    };
}

#endif /* ThemeWidget_h */
