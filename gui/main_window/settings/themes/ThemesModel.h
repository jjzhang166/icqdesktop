#ifndef ThemesModel_h
#define ThemesModel_h

namespace Ui
{
    class ThemesWidget;
    class ThemesModel : public QObject
    {
        Q_OBJECT
        ThemesWidget* themesWidget_;
        QString targetContact_;
    private Q_SLOTS:
        void onThemesMeta();
        void onThemesMetaError();
        void im_created();

    public:
        ThemesModel(ThemesWidget* _flowLayout);
        void themeSelected(int _themeId);
        void setTargetContact(QString _aimId);
    };
}

#endif /* ThemesModel_h */
