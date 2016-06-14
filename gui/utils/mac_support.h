//
//  mac_support.h
//  ICQ
//
//  Created by Vladimir Kubyshev on 03/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

namespace Ui
{
    class MainWindow;
}

class MacSupport
{
private:
    QMenuBar *mainMenu_;
    std::vector<QMenu *> extendedMenus_;
    std::vector<QAction *> extendedActions_;
    
public:
    MacSupport(Ui::MainWindow * mainWindow);
    virtual ~MacSupport();
    
    void enableMacUpdater();
    void enableMacCrashReport();
    void enableMacPreview(WId wid);
    
    void listenSleepAwakeEvents();
    
    void runMacUpdater();
    void cleanMacUpdater();
    
    void forceEnglishInputSource();
    
    static void toggleFullScreen(WId wid);
    static void showPreview(QString previewPath, int x, int y);
    static bool previewIsShown();
    static void openFinder(QString previewPath);
    
    static QString currentRegion();
    
    static QString currentTheme();
    
    static QString settingsPath();
    
    static void log(QString logString);
    
    static void getPossibleStrings(const QString& text, QStringList & result);
    
    static bool nativeEventFilter(const QByteArray &data, void *message, long *result);
    
    static void replacePasteboard(const QString & text);

    void createMenuBar(bool simple);
    void updateMainMenu();
    
    void activateWindow(unsigned long long view = 0);
    
    void registerDelegate();
    
private:
    void setupDockClickHandler();
};


