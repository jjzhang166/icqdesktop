#ifndef HistoryControlPageThemePanel_h
#define HistoryControlPageThemePanel_h

namespace Ui
{
    class CustomButton;
    class HistoryControlPage;
    
    enum ThemePanelChoice: int {
        ThemePanelCancel,
        ThemePanelSet,
        ThemePanelSetToAll,
        ThemePanelBackToSettings
    };
    
    typedef std::function<void(ThemePanelChoice)> ThemePanelCallback;
    
    class HistoryControlPageThemePanel : public QWidget
    {
        Q_OBJECT
        
        virtual void paintEvent(QPaintEvent*) override;
        virtual void resizeEvent(QResizeEvent *) override;
        
        CustomButton *previewButton_;
        CustomButton *cancelButton_;
        CustomButton *setToAllButton_;
        CustomButton *setButton_;
        CustomButton *backFromThemeButton_;
        
        QSpacerItem *h_spacer_0_;
        QSpacerItem *h_spacer_1_;
        QSpacerItem *h_spacer_2_;
        QSpacerItem *h_spacer_3_;
        QSpacerItem *h_spacer_4_;
        QSpacerItem *h_spacer_5_;
        
        HistoryControlPage *historyControlPage_;
        ThemePanelCallback callback_;
        
        bool selectionThemeFromSettings_;
        
    public:
        HistoryControlPageThemePanel(HistoryControlPage* _parent);
        void updateTopThemeButtonsVisibility();
        void setShowSetThemeButton(const bool _show);
        
        void setCallback(ThemePanelCallback);
        bool settingThemeToAll_;
        void setSelectionToAll(bool _fromSettings);
        
    private Q_SLOTS:
        void backFromThemePressed();
        void cancelThemePressed();
        void setToAllThemePressed();
        void setThemePressed();
    };
}

#endif /* HistoryControlPageThemePanel_h */
