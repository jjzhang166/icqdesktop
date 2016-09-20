#ifndef ThemesWidget_h
#define ThemesWidget_h
#include "../../../cache/themes/themes.h"

namespace Ui {
    class FlowLayout;
    class ThemesModel;
    class BackButton;
    
    class ThemesWidget : public QWidget
    {
        Q_OBJECT
        FlowLayout *flowLayout_;
        ThemesModel *themesModel_;
        themes::themesList themesList_;
        QMap<int,bool> loadedThemes_;
        BackButton* backButton_;
        QWidget *backButtonAndCaptionSpacer_;
        QWidget *captionWithoutBackButtonSpacer_;
        
        bool firstThemeAdded_;
        void checkFirstTheme_(themes::themePtr _theme);

    public:
        ThemesWidget(QWidget* _parent, int _spacing);
        ~ThemesWidget();

        void onThemeGot(themes::themePtr theme);
        void setTargetContact(QString _aimId);
        void setBackButton(bool _doSet);

    protected:
        virtual void resizeEvent(QResizeEvent* _e) override;

    private Q_SLOTS:
        void touchScrollStateChanged(QScroller::State);
        void backPressed();
    };

}
#endif /* ThemesTableWidget_h */
