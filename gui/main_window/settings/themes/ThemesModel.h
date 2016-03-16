#ifndef ThemesModel_h
#define ThemesModel_h

namespace Ui
{
    class ThemesWidget;
    class ThemesModel : public QObject
    {
        Q_OBJECT
        ThemesWidget* themes_widget_;
        QString target_contact_;
    private Q_SLOTS:
        void on_themes_meta();
    public:
        ThemesModel(ThemesWidget* _flowLayout);
        void themeSelected(int _theme_id);
        void set_target_contact(QString _aimId);
    };
}

#endif /* ThemesModel_h */
