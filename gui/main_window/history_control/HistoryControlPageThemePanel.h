#ifndef HistoryControlPageThemePanel_h
#define HistoryControlPageThemePanel_h

namespace Ui
{
    class BackButton;
    class CustomButton;
    class HistoryControlPage;
    
    enum ThemePanelChoice: int
    {
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
        BackButton *backFromThemeButton_;
        
        HistoryControlPage *historyControlPage_;
        ThemePanelCallback callback_;
        
        bool selectionThemeFromSettings_;
        
    public:
        HistoryControlPageThemePanel(HistoryControlPage* _parent);
        ~HistoryControlPageThemePanel();
        
        void updateTopThemeButtonsVisibility();
        void setShowSetThemeButton(const bool _show);
        
        void setCallback(ThemePanelCallback);
        bool settingThemeToAll_;
        void setSelectionToAll(bool _fromSettings);

    public Q_SLOTS:
        void cancelThemePressed();

    private Q_SLOTS:
        void backFromThemePressed();
        void setToAllThemePressed();
        void setThemePressed();
        void timerUpdateTopThemeButtonsVisibility();
    };
}

#endif /* HistoryControlPageThemePanel_h */
