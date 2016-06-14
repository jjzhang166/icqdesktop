//
//  mac_updater.m
//  ICQ
//
//  Created by Vladimir Kubyshev on 03/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>
#import <HockeySDK/HockeySDK.h>

#import <Quartz/Quartz.h>

#include "stdafx.h"
#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "../main_window/contact_list/ContactListModel.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"

#import "mac_support.h"

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

enum
{
    edit_undo = 100,
    edit_redo,
    edit_cut,
    edit_copy,
    edit_paste,
    edit_pasteAsQuote,
    edit_pasteEmoji,
    contact_addBuddy,
    contact_profile,
    contact_close,
    view_fullScreen,
    view_unreadMessage,
    window_nextChat,
    window_prevChat,
    window_minimizeWindow,
    window_mainWindow,
    global_about,
    global_settings,
    global_update,
    // TODO: add other items when needed.
};
    
static QMap<int, QAction *> menuItems_;
static Ui::MainWindow * mainWindow_ = nil;

static QString toQString(NSString * src)
{
    return QString::fromCFString((__bridge CFStringRef)src);
}

static NSString * fromQString(const QString & src)
{
    return (NSString *)CFBridgingRelease(src.toCFString());
}

@interface AppDelegate : NSObject
- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation AppDelegate
- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* url = [[event paramDescriptorForKeyword:keyDirectObject]
                     stringValue];
    
    emit Utils::InterConnector::instance().schemeUrlClicked(QString::fromNSString(url));
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if (mainWindow_ != nil)
    {
        mainWindow_->exit();
    }
    return NSTerminateCancel;
}
@end



@interface LinkPreviewItem : NSObject <QLPreviewItem>
@property (readonly) NSURL *previewItemURL;
@property (readonly) NSString *previewItemURLString;
@property (readonly) NSString *previewItemTitle;
@property (readonly) CGPoint point;

- (id)initWithPath:(NSString *)path andPoint:(CGPoint)point;

@end


@implementation LinkPreviewItem

- (id)initWithPath:(NSString *)path andPoint:(CGPoint)point
{
    self = [super init];
    if (self)
    {
        if ([[NSFileManager defaultManager] fileExistsAtPath:path])
        {
            _previewItemURL = [NSURL fileURLWithPath:path];
        }
        else
        {
            _previewItemURL = [NSURL URLWithString:path];
        }
        _previewItemURLString = [[NSString alloc] initWithString:path];
        _previewItemTitle = [[NSString alloc] initWithString:_previewItemURL.lastPathComponent];
        _point = point;
    }
    return self;
}

- (void)dealloc
{
    [_previewItemURL release];
    _previewItemURL = nil;

    [_previewItemURLString release];
    _previewItemURLString = nil;

    [_previewItemTitle release];
    _previewItemTitle = nil;
    
    [super dealloc];
}

@end

@interface MacPreviewProxy : NSViewController<QLPreviewPanelDelegate, QLPreviewPanelDataSource>

@property (strong) LinkPreviewItem *previewItem;

- (void)showPreviewPopupWithPath:(NSString *)path atPos:(CGPoint)point;

@end

@implementation MacPreviewProxy

- (instancetype)initInWindow:(NSWindow *)window
{
    if (self = [super init])
    {
        self.view = [[[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)] autorelease];
        
        [window.contentView addSubview:self.view];
        
        NSResponder * aNextResponder = [window nextResponder];
        
        [window setNextResponder:self];
        [self setNextResponder:aNextResponder];
        
//        [QLPreviewPanel sharedPreviewPanel].delegate = self;
    }
    
    return self;
}

- (BOOL)hidePreviewPopup
{
    if ([QLPreviewPanel sharedPreviewPanelExists] && [[QLPreviewPanel sharedPreviewPanel] isVisible])
    {
        [[QLPreviewPanel sharedPreviewPanel] orderOut:nil];
        self.previewItem = nil;
        
        return YES;
    }
    return NO;
}

- (void)showPreviewPopupWithPath:(NSString *)path atPos:(CGPoint)point
{
    if (![self hidePreviewPopup])
    {
        self.previewItem = [[LinkPreviewItem alloc] initWithPath:path andPoint:point];
        [[QLPreviewPanel sharedPreviewPanel] makeKeyAndOrderFront:nil];
    }
}

- (void)beginPreviewPanelControl:(QLPreviewPanel *)panel
{
    panel.delegate = self;
    panel.dataSource = self;
}

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel *)panel;
{
    return YES;
}

- (NSInteger)numberOfPreviewItemsInPreviewPanel:(QLPreviewPanel *)panel
{
    return (self.previewItem) ? 1 : 0;
}

- (CGRect)previewPanel:(QLPreviewPanel *)panel sourceFrameOnScreenForPreviewItem:(id<QLPreviewItem>)item
{
    LinkPreviewItem * link = item;
    
    CGPoint point = link.point;
    
    if (link.point.x == -1 ||
        link.point.y == -1)
    {
        return NSZeroRect;
    }
    
    CGRect rect = CGRectMake(0, 0, 30, 20);
    rect.origin = point;
    
    return rect;
}

- (id <QLPreviewItem>)previewPanel:(QLPreviewPanel *)panel previewItemAtIndex:(NSInteger)index
{
    return self.previewItem;
}

- (void)endPreviewPanelControl:(QLPreviewPanel *)panel
{
    panel.delegate = nil;
    panel.dataSource = nil;
    
    self.previewItem = nil;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    if (!self.previewItem)
    {
        [self hidePreviewPopup];
    }
}

@end

// Events catcher

@interface EventsCatcher : NSObject

@property (nonatomic, copy) void(^sleep)(void);
@property (nonatomic, copy) void(^wake)(void);

- (instancetype)initWithSleepLambda:(void(^)(void))sleep andWakeLambda:(void(^)(void))wake;
- (void)receiveSleepEvent:(NSNotification *)notification;
- (void)receiveWakeEvent:(NSNotification *)notification;

@end

@implementation EventsCatcher

- (instancetype)initWithSleepLambda:(void (^)())sleep andWakeLambda:(void (^)())wake
{
    self = [super init];
    if (self)
    {
        self.sleep = sleep;
        self.wake = wake;
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveSleepEvent:)
                                                                   name:NSWorkspaceWillSleepNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveWakeEvent:)
                                                                   name:NSWorkspaceDidWakeNotification object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveSleepEvent:)
                                                                name:@"com.apple.screenIsLocked"
                                                              object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveWakeEvent:)
                                                                name:@"com.apple.screenIsUnlocked"
                                                              object:nil];
    }
    return self;
}

- (void)receiveSleepEvent:(NSNotification *)notification
{
    self.sleep();
}

- (void)receiveWakeEvent:(NSNotification *)notification
{
    self.wake();
}

@end

// ! Events catcher


static SUUpdater * sparkleUpdater_ = nil;
static MacPreviewProxy * macPreviewProxy_ = nil;

MacSupport::MacSupport(Ui::MainWindow * mainWindow): mainMenu_(nullptr)
{
    sparkleUpdater_ = nil;
    mainWindow_ = mainWindow;
    registerDelegate();
    setupDockClickHandler();
}

MacSupport::~MacSupport()
{
    menuItems_.clear();

    mainWindow_ = nil;
    
    cleanMacUpdater();
    
    [macPreviewProxy_ hidePreviewPopup];
    [macPreviewProxy_ release];
    macPreviewProxy_ = nil;
}

void MacSupport::enableMacUpdater()
{
#ifdef UPDATES
    if (sparkleUpdater_ != nil)
    {
        return;
    }
    
    NSDictionary *plist = [[NSBundle mainBundle] infoDictionary];
    
    BOOL automaticChecks = [plist[@"SUEnableAutomaticChecks"] boolValue];
    NSURL * updateFeed = [NSURL URLWithString:plist[@"SUFeedURL"]];
    
    sparkleUpdater_ = [[SUUpdater alloc] init];
    
    [sparkleUpdater_ setAutomaticallyChecksForUpdates:automaticChecks];
    
    
#ifdef DEBUG
    updateFeed = [NSURL URLWithString:@"http://testmra.mail.ru/icq_mac_update/icq_update.xml"];
#endif
    
    if (updateFeed)
    {
        [sparkleUpdater_ setFeedURL:updateFeed];
        [sparkleUpdater_ setUpdateCheckInterval:86400];
        
//        [sparkleUpdater_ checkForUpdates:nil];
    }
#endif
}

void MacSupport::enableMacCrashReport()
{
#ifndef _DEBUG
    [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:HOCKEY_APPID companyName:@"Mail.Ru Group" delegate:nil];
    
    [[BITHockeyManager sharedHockeyManager] startManager];
#endif
}

void MacSupport::listenSleepAwakeEvents()
{
    static EventsCatcher *eventsCatcher = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        //
        eventsCatcher = [[EventsCatcher alloc] initWithSleepLambda:^{
            //
            if (mainWindow_)
                mainWindow_->gotoSleep();
            //
        } andWakeLambda:^{
            //
            if (mainWindow_)
                mainWindow_->gotoWake();
            //
        }];
        //
    });
}

void MacSupport::runMacUpdater()
{
    if (sparkleUpdater_)
    {
        [sparkleUpdater_ checkForUpdates:nil];
    }
}

void MacSupport::cleanMacUpdater()
{
    [sparkleUpdater_ release];
    sparkleUpdater_ = nil;
}

void MacSupport::forceEnglishInputSource()
{
    NSArray *sources = CFBridgingRelease(TISCreateInputSourceList((__bridge CFDictionaryRef)@{ (__bridge NSString*)kTISPropertyInputSourceID:@"com.apple.keylayout.US" }, FALSE));
    TISInputSourceRef source = (__bridge TISInputSourceRef)sources[0];
    TISSelectInputSource(source);
}

void MacSupport::enableMacPreview(WId wid)
{
    void * pntr = (void*)wid;
    NSView * view = (__bridge NSView *)pntr;
    
    macPreviewProxy_ = [[MacPreviewProxy alloc] initInWindow:view.window];
}

void MacSupport::toggleFullScreen(WId wid)
{
    emit Utils::InterConnector::instance().closeAnyPopupWindow();

    void * pntr = (void*)wid;
    NSView * view = (__bridge NSView *)pntr;
    [view.window toggleFullScreen:nil];
}

void MacSupport::showPreview(QString previewPath, int x, int y)
{
    if ([[[macPreviewProxy_ view] window] isKeyWindow])
    {
        NSString *path = fromQString(previewPath);
        if (y != -1)
        {
            y = [NSScreen mainScreen].visibleFrame.size.height - y;
        }
        [macPreviewProxy_ showPreviewPopupWithPath:path atPos:NSMakePoint(x, y)];
    }
}

bool MacSupport::previewIsShown()
{
    return ([QLPreviewPanel sharedPreviewPanelExists] && [[QLPreviewPanel sharedPreviewPanel] isVisible]);
}

void MacSupport::openFinder(QString previewPath)
{
    NSString * previewPath_ = fromQString(previewPath);
    NSURL * previewUrl = [NSURL fileURLWithPath:previewPath_];
    
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[previewUrl]];
}

QString MacSupport::currentRegion()
{
    NSLocale * locale = [NSLocale currentLocale];
    
    return toQString(locale.localeIdentifier);
}

QString MacSupport::currentTheme()
{
    NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    BOOL darkModeOn = ( style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"] );
    
    return darkModeOn?"black":"white";
}

QString MacSupport::settingsPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *applicationSupportDirectory = [paths firstObject];
    return toQString([applicationSupportDirectory stringByAppendingPathComponent:@"ICQ"]);
}

template< typename MenuT >
QAction * createAction(MenuT * menu, QString title, QString shortcut, const QObject * receiver = NULL, const char * method = NULL)
{
    QAction * action = menu->addAction(title, receiver, method);
    action->setShortcut(QKeySequence(shortcut));
    
    return action;
}

void MacSupport::createMenuBar(bool simple)
{
    if (!mainMenu_)
    {
        mainMenu_ = mainWindow_->menuBar();
        QObject::connect(mainMenu_, &QMenuBar::triggered, [](QAction *)
        {
            emit Utils::InterConnector::instance().closeAnyPopupWindow();
        });

        QMenu *menu = 0;
        QAction *action = 0;
        
        menu = mainMenu_->addMenu(QT_TRANSLATE_NOOP("main_menu","&Edit"));
        menuItems_.insert(edit_undo, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Undo"), "Ctrl+Z", mainWindow_, SLOT(undo())));
        menuItems_.insert(edit_redo, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Redo"), "Shift+Ctrl+Z", mainWindow_, SLOT(redo())));
        menu->addSeparator();
        menuItems_.insert(edit_cut, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Cut"), "Ctrl+X", mainWindow_, SLOT(cut())));
        menuItems_.insert(edit_copy, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Copy"), "Ctrl+C", mainWindow_, SLOT(copy())));
        menuItems_.insert(edit_paste, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Paste"), "Ctrl+V", mainWindow_, SLOT(paste())));
        menuItems_.insert(edit_pasteAsQuote, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Paste as Quote"), "Alt+Ctrl+V", mainWindow_, SLOT(quote())));
        menu->addSeparator();
        extendedMenus_.push_back(menu);
        
        menu = mainMenu_->addMenu(QT_TRANSLATE_NOOP("main_menu","Contact"));
        menuItems_.insert(contact_addBuddy, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Add Buddy"), "Ctrl+N", mainWindow_, SLOT(activateContactSearch())));
        menuItems_.insert(contact_profile, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Profile"), "Ctrl+I", mainWindow_, SLOT(activateProfile())));
        menuItems_.insert(contact_close, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Close"), "Ctrl+W", mainWindow_, SLOT(closeCurrent())));
        extendedMenus_.push_back(menu);
        
        menu = mainMenu_->addMenu(QT_TRANSLATE_NOOP("main_menu","View"));
        menuItems_.insert(view_unreadMessage, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Next Unread Message"), "Ctrl+]", mainWindow_, SLOT(activateNextUnread())));
        menuItems_.insert(view_fullScreen, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Enter Full Screen"), "Meta+Ctrl+F", mainWindow_, SLOT(toggleFullScreen())));
        extendedMenus_.push_back(menu);
        
        menu = mainMenu_->addMenu(QT_TRANSLATE_NOOP("main_menu","Window"));
        menuItems_.insert(window_nextChat, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Select Next Chat"), "Shift+Ctrl+]", mainWindow_, SLOT(activateNextChat())));
        menuItems_.insert(window_prevChat, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Select Previous Chat"), "Shift+Ctrl+[", mainWindow_, SLOT(activatePrevChat())));
        menuItems_.insert(window_minimizeWindow, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Minimize"), "Ctrl+M", mainWindow_, SLOT(minimize())));
        menuItems_.insert(window_mainWindow, createAction(menu, QT_TRANSLATE_NOOP("main_menu","Main Window"), "Ctrl+1", mainWindow_, SLOT(activate())));
        extendedMenus_.push_back(menu);
        
        action = createAction(menu, "about.*", "", mainWindow_, SLOT(activateAbout()));
        menuItems_.insert(global_about, action);
        extendedActions_.push_back(action);
        
        action = createAction(menu, "settings", "", mainWindow_, SLOT(activateSettings()));
        menuItems_.insert(global_settings, action);
        extendedActions_.push_back(action);

#ifdef UPDATES
        action = createAction(menu, QT_TRANSLATE_NOOP("context_menu", "Check for updates..."), "", mainWindow_, SLOT(checkForUpdates()));
        action->setMenuRole(QAction::MenuRole::ApplicationSpecificRole);
        menuItems_.insert(global_update, action);
        extendedActions_.push_back(action);
#endif

        createAction(mainMenu_, "quit", "", mainWindow_, SLOT(exit()));
        
        mainWindow_->setMenuBar(mainMenu_);
    }

    for (auto menu: extendedMenus_)
        menu->setEnabled(!simple);
    
    for (auto action: extendedActions_)
        action->setEnabled(!simple);
    
    updateMainMenu();
    
//    Edit
//    Undo cmd+Z
//    Redo shift+cmd+Z
//    Cut cmd+X
//    Copy cmd+C
//    Paste cmd+V
//    Paste as quote alt+cmd+V
//    Delete
//    Select all cmd+A
    
//    Юзер залогинен
//    Меню состоит из пунктов:
//    ICQ
//    About
//    Preferences cmd+,
//    Logout username (userid)
//    Hide cmd+H
//    Hide other alt+cmd+H
//    Quit
    
//    Contact
//    Add buddy cmd+N
//    Profile cmd+I
//    Close cmd+W
    
//    View
//    Next unread messsage cmd+]
//    Enter Full Screen ^cmd+F
    
//    Window
//    Select next chat shift+cmd+]
//    Select previous chat shift+cmd+[
//    Main window cmd+1
}

void MacSupport::updateMainMenu()
{
    auto actions = mainWindow_->menuBar()->actions();
    
    QAction * nextChat = menuItems_[window_nextChat];
    QAction * prevChat = menuItems_[window_prevChat];
    QAction * fullScreen = menuItems_[view_fullScreen];
   
    if (mainWindow_ && mainWindow_->getMainPage() && mainWindow_->getMainPage()->getContactDialog() && nextChat && prevChat)
    {
        const QString & aimId = Logic::GetContactListModel()->selectedContact();
        
        nextChat->setEnabled(aimId.length() > 0);
        prevChat->setEnabled(aimId.length() > 0);
    }
    
    if (mainWindow_ && fullScreen)
    {
        void * pntr = (void*)mainWindow_->winId();
        NSView * view = (__bridge NSView *)pntr;
        if ((([view.window styleMask] & NSFullScreenWindowMask) == NSFullScreenWindowMask))
            fullScreen->setText(QT_TRANSLATE_NOOP("main_menu", "Exit Full Screen"));
        else
            fullScreen->setText(QT_TRANSLATE_NOOP("main_menu", "Enter Full Screen"));
    }
}

void MacSupport::log(QString logString)
{
    NSString * logString_ = fromQString(logString);

    NSLog(@"%@", logString_);
}

void MacSupport::replacePasteboard(const QString &text)
{
    NSPasteboard * pb = [NSPasteboard generalPasteboard];
    
    [pb clearContents];
    [pb declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
    [pb setString:fromQString(text) forType:NSPasteboardTypeString];
}

typedef const UCKeyboardLayout * LayoutsPtr;

LayoutsPtr layoutFromSource(TISInputSourceRef source)
{
    CFDataRef keyboardLayoutUchr = (CFDataRef)TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
    if (keyboardLayoutUchr == nil)
    {
        return nil;
    }
    return (LayoutsPtr)CFDataGetBytePtr(keyboardLayoutUchr);
}

void setupLayouts(QList<LayoutsPtr> & layouts)
{
    NSDictionary *filter = @{(NSString *)kTISPropertyInputSourceCategory: (NSString *)kTISCategoryKeyboardInputSource};
    
    CFArrayRef inputSourcesListRef = TISCreateInputSourceList((CFDictionaryRef)filter, false);
    
    NSArray * sources = (NSArray *)inputSourcesListRef;
    
    NSMutableArray * usingSources = [[NSMutableArray alloc] init];
    
    NSInteger availableSourcesNumber = [sources count];
    for (NSInteger j = 0; j < availableSourcesNumber; ++j)
    {
        LayoutsPtr layout = layoutFromSource((TISInputSourceRef)sources[j]);
        if(layout)
        {
            layouts.append(layout);
        }
    }
    
    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
    LayoutsPtr currentLayout = layoutFromSource(currentKeyboard);
    
    for (int i = 0; i < layouts.size(); i++)
    {
        LayoutsPtr l = layouts[i];
        if (l == currentLayout)
        {
            layouts.removeAt(i);
            layouts.insert(0, l);
            break;
        }
    }
    
    CFRelease(inputSourcesListRef);
    CFRelease(currentKeyboard);
    
    [usingSources release];
}

UniChar stringForKey(CGKeyCode keyCode, const UCKeyboardLayout * layout)
{
    UInt32 keysDown = 0;
    UniChar chars[4];
    UniCharCount realLength = 0;
    UInt32	 modifierFlags = 0;
    
    OSStatus status = UCKeyTranslate(layout,
                                     keyCode,
                                     kUCKeyActionDisplay,
                                     modifierFlags,
                                     LMGetKbdType(),
                                     kUCKeyTranslateNoDeadKeysBit,
                                     &keysDown,
                                     sizeof(chars) / sizeof(chars[0]),
                                     &realLength,
                                     chars);
    if( (status != noErr) || (realLength == 0) )
    {
        return 0;
    }
    return chars[0];
}

CGKeyCode keyCodeForChar(unichar letter, UCKeyboardLayout const * layout)
{
    for (int i =0; i<52; ++i)
    {
        if (letter == stringForKey(i, layout))
        {
            return i;
        }
    }
    
    return UINT16_MAX;
}

NSString * translitedString(NSString * sourceString, LayoutsPtr sourceLayout, LayoutsPtr targetLayout)
{
    NSMutableString *possibleString = [NSMutableString string];
    
    for (NSInteger i = 0; i < sourceString.length; ++i)
    {
        unichar letter = [sourceString characterAtIndex:i];
        
        unichar translit = letter;
        CGKeyCode keyCode = UINT16_MAX;
        keyCode = keyCodeForChar(letter, sourceLayout);
        if(keyCode != UINT16_MAX)
        {
            translit = stringForKey(keyCode, targetLayout);
        }
        
        [possibleString appendString:[NSString stringWithCharacters:&translit length:1]];
    }
    
    return possibleString;
}

void MacSupport::getPossibleStrings(const QString& text, QStringList & result)
{
    QList<LayoutsPtr> layouts;
    
    setupLayouts(layouts);
    
    result.append(text);
    
    if (layouts.size() == 0)
    {
        return;
    }
    
    NSString * sourceString = fromQString(text);
    
    LayoutsPtr sourceLayout = layouts[0];
    
    for (int i = 1; i < layouts.size(); i++)
    {
        LayoutsPtr targetLayout = layouts[i];
        
        if (sourceLayout != targetLayout)
        {
            NSString * translited = translitedString(sourceString, sourceLayout, targetLayout);
            
            if (translited.length)
            {
                result.push_back(toQString(translited));
            }
        }
    }
}


bool dockClickHandler(id self,SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    
    if (mainWindow_ && (mainWindow_->isHidden() || mainWindow_->isMinimized()))
    {
        mainWindow_->activate();
    }
    
    // Return NO (false) to suppress the default OS X actions
    return false;
}

void MacSupport::setupDockClickHandler()
{
    NSApplication * appInst = [NSApplication sharedApplication];
    
    if (appInst != NULL)
    {
        id<NSApplicationDelegate> delegate = appInst.delegate;
        Class delClass = delegate.class;
        SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
        if (class_getInstanceMethod(delClass, shouldHandle))
        {
            if (class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:"))
                qDebug() << "Registered dock click handler (replaced original method)";
            else
                qWarning() << "Failed to replace method for dock click handler";
        }
        else
        {
            if (class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:"))
                qDebug() << "Registered dock click handler";
            else
                qWarning() << "Failed to register dock click handler";
        }
    }
}

bool MacSupport::nativeEventFilter(const QByteArray &data, void *message, long *result)
{
    NSEvent *e = (NSEvent *)message;
    // NSLog(@"------------\n%@\n%@", data.toNSData(), e);

    if ([e type] == NSEventType::NSKeyDown && ([e modifierFlags] & (NSControlKeyMask | NSCommandKeyMask)))
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow();
    }
    else if ([e type] == NSAppKitDefined && [e subtype] == NSApplicationDeactivatedEventType)
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow();
    }
    
    return false;
}

void MacSupport::activateWindow(unsigned long long view/* = 0*/)
{
    if (view)
    {
        auto p = (NSView *)view;
        if (p)
        {
            auto w = [p window];
            if (w)
            {
                [w setIsVisible:YES];
            }
        }
    }
    [NSApp activateIgnoringOtherApps:YES];
}

void MacSupport::registerDelegate()
{
    AppDelegate* delegate = [AppDelegate new];
    [NSApp setDelegate: delegate];
    
    [[NSAppleEventManager sharedAppleEventManager]
     setEventHandler:delegate
     andSelector:@selector(handleURLEvent:withReplyEvent:)
     forEventClass:kInternetEventClass
     andEventID:kAEGetURL];
}


