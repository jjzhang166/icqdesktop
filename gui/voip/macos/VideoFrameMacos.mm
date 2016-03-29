#import "VideoFrameMacos.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "../../../core/Voip/libvoip/include/voip/voip_types.h"

@interface WindowRect : NSObject
{
}

@property NSRect rect;

@end

@implementation WindowRect

-(instancetype)init
{
    self = [super init];
    if (!!self) {
        NSRect rc;

        rc.origin.x = 0.0f;
        rc.origin.y = 0.0f;
        rc.size.width  = 0.0f;
        rc.size.height = 0.0f;

        self.rect = rc;
    }
    
    return self;
}

@end


@interface WindowListApplierData : NSObject
{
}

@property (strong, nonatomic) NSMutableArray* outputArray;
@property int order;

@end

@implementation WindowListApplierData

-(instancetype)initWindowListData:(NSMutableArray *)array {
    self = [super init];
    
    if (!!self) {
        self.outputArray = array;
        self.order = 0;
    }
    
    return self;
}

-(void)dealloc {
    if (!!self && !!self.outputArray) {
        for (id item in self.outputArray) {
            [item release];
        }
    }
    [super dealloc];
}

@end

NSString *kAppNameKey      = @"applicationName"; // Application Name & PID
NSString *kWindowOriginKey = @"windowOrigin";    // Window Origin as a string
NSString *kWindowSizeKey   = @"windowSize";      // Window Size as a string
NSString *kWindowRectKey   = @"windowRect";      // The overall front-to-back ordering of the windows as returned by the window server

inline uint32_t ChangeBits(uint32_t currentBits, uint32_t flagsToChange, BOOL setFlags) {
    if(setFlags) {
        return currentBits | flagsToChange;
    } else {
        return currentBits & ~flagsToChange;
    }
}

void WindowListApplierFunction(const void *inputDictionary, void *context) {
    assert(!!inputDictionary && !!context);
    if (!inputDictionary || !context) { return; }

    NSDictionary* entry         = (__bridge NSDictionary*)inputDictionary;
    WindowListApplierData* data = (__bridge WindowListApplierData*)context;
    
    int sharingState = [entry[(id)kCGWindowSharingState] intValue];
    if(sharingState != kCGWindowSharingNone) {
        NSMutableDictionary *outputEntry = [NSMutableDictionary dictionary];
        assert(!!outputEntry);
        if (!outputEntry) { return; }
        
        NSString* applicationName = entry[(id)kCGWindowOwnerName];
        if(applicationName != NULL) {
            if ([applicationName compare:@"dock" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
                return;
            }
#ifdef _DEBUG
            NSString *nameAndPID = [NSString stringWithFormat:@"%@ (%@)", applicationName, entry[(id)kCGWindowOwnerPID]];
            outputEntry[kAppNameKey] = nameAndPID;
#endif
        } else {
            return;
        }
        
        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)entry[(id)kCGWindowBounds], &bounds);
        
#ifdef _DEBUG
        NSString *originString = [NSString stringWithFormat:@"%.0f/%.0f", bounds.origin.x, bounds.origin.y];
        outputEntry[kWindowOriginKey] = originString;

        NSString *sizeString = [NSString stringWithFormat:@"%.0f*%.0f", bounds.size.width, bounds.size.height];
        outputEntry[kWindowSizeKey] = sizeString;
#endif
        
        WindowRect* wr = [[WindowRect alloc] init];
        wr.rect = bounds;
        outputEntry[kWindowRectKey] = wr;
        
        [data.outputArray addObject:outputEntry];
    }
}

void platform_macos::fadeIn(QWidget* wnd) {
    assert(!!wnd);
    if (!wnd) { return; }
    
    NSView* view = (NSView*)wnd->winId();
    assert(view);
    
    NSWindow* window = [view window];
    assert(window);
    
    if ([[window animator] alphaValue] < 0.01f) {
        [[window animator] setAlphaValue:1.0f];
    }
}

void platform_macos::fadeOut(QWidget* wnd) {
    assert(!!wnd);
    if (!wnd) { return; }
    
    NSView* view = (NSView*)wnd->winId();
    assert(view);
    
    NSWindow* window = [view window];
    assert(window);
    
    if ([[window animator] alphaValue] > 0.9f) {
        [[window animator] setAlphaValue:0.0f];
    }
}

void platform_macos::setPanelAttachedAsChild(bool attach, QWidget& parent, QWidget& child) {
    NSView* parentView = (NSView*)parent.winId();
    assert(parentView);
    
    NSWindow* window = [parentView window];
    assert(window);
    
    NSView* childView = (NSView*)child.winId();
    assert(childView);
    if (!childView) { return; }
    
    NSWindow* childWnd = [childView window];
    assert(childWnd);
    if (!childWnd) { return; }
    
    if (attach) {
        [window addChildWindow:childWnd ordered:NSWindowAbove];
    } else {
        [window removeChildWindow:childWnd];
    }
}

void platform_macos::setWindowPosition(QWidget& widget, const QWidget& parent, const bool top) {
    NSRect rect;
    {// get parent window rect
        NSView* view = (NSView*)parent.winId();
        assert(view);
        if (!view) { return; }
        
        NSWindow* window = [view window];
        assert(window);
        if (!window) { return; }
        
        rect = [window frame];
    }
    
    NSView* view = (NSView*)widget.winId();
    assert(view);
    
    NSWindow* window = [view window];
    assert(window);
    
    const int widgetH = widget.height();
    if (top) {
        rect.origin.y = rect.origin.y + rect.size.height - widgetH;
    }

    rect.size.height = widgetH;
    [window setFrame:rect display:YES];
}

void platform_macos::setAspectRatioForWindow(QWidget& wnd, float aspectRatio) {
    NSView* view = (NSView*)wnd.winId();
    assert(view);
    if (view) {
        NSWindow* window = [view window];
        assert(window);
        if (window) {
            [window setContentAspectRatio:NSMakeSize(10.0f * aspectRatio, 10.0f)];
        }
    }
}

bool platform_macos::windowIsOverlapped(QWidget* frame) {
    assert(!!frame);
    if (!frame) { return false; }
    
    NSView* view = (NSView*)frame->winId();
    assert(!!view);
    if (!view) { return false; }
    
    NSWindow* window = [view window];
    assert(!!window);
    if (!window) { return false; }
    
    const CGWindowID windowID = (CGWindowID)[window windowNumber];
    CGWindowListOption listOptions = kCGWindowListOptionOnScreenAboveWindow;
    listOptions = ChangeBits(listOptions, kCGWindowListExcludeDesktopElements, YES);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(listOptions, windowID);
    NSMutableArray * prunedWindowList = [NSMutableArray array];
    WindowListApplierData* windowListData = [[WindowListApplierData alloc] initWindowListData:prunedWindowList];
    
    CFArrayApplyFunction(windowList, CFRangeMake(0, CFArrayGetCount(windowList)), &WindowListApplierFunction, (__bridge void *)(windowListData));

    const QRect frameRect([window frame].origin.x, [window frame].origin.y, [window frame].size.width, [window frame].size.height);
    const int originalSquare = frameRect.width() * frameRect.height();
    
    QRegion selfRegion(frameRect);
    for (NSMutableDictionary* params in windowListData.outputArray) {
        WindowRect* wr = params[kWindowRectKey];
        QRegion wrRegion(QRect(wr.rect.origin.x, wr.rect.origin.y, wr.rect.size.width, wr.rect.size.height));
        selfRegion -= wrRegion;
    }
    
    int remainsSquare = 0;
    const auto remainsRects = selfRegion.rects();
    for (unsigned ix = 0; ix < remainsRects.count(); ++ix) {
        const QRect& rc = remainsRects.at(ix);
        remainsSquare += rc.width() * rc.height();
    }
    
    [windowListData release];
    CFRelease(windowList);
    
    return remainsSquare < originalSquare*0.6f;
}


namespace platform_macos {
    
class GraphicsPanelMacosImpl : public platform_specific::GraphicsPanel {
    std::vector<QWidget*> _panels;
    WebrtcRenderView* _renderView;
    
    virtual void moveEvent(QMoveEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;
    virtual void showEvent(QShowEvent*) override;
    virtual void hideEvent(QHideEvent*) override;
    
    void _setPanelsAttached(bool attach);

public:
    GraphicsPanelMacosImpl(QWidget* parent, std::vector<QWidget*>& panels);
    virtual ~GraphicsPanelMacosImpl();
    
    virtual WId frameId() const override;

    void addPanels(std::vector<QWidget*>& panels) override;
    void clearPanels() override;
    void fullscreenModeChanged(bool fullscreen) override;
};

GraphicsPanelMacosImpl::GraphicsPanelMacosImpl(QWidget* parent, std::vector<QWidget*>& panels)
: platform_specific::GraphicsPanel(parent)
, _panels(panels) {
    NSRect frame;
    frame.origin.x    = 0;
    frame.origin.y    = 0;
    frame.size.width  = 1;
    frame.size.height = 1;
    
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setAttribute(Qt::WA_UpdatesDisabled);
    
    //NSView* window = (NSView*)parent->winId();
    //[[window window] setStyleMask:NSTitledWindowMask];
    
    _renderView = [[WebrtcRenderView alloc] initWithFrame:frame];
    assert(_renderView);

    NSView* parentView = (NSView*)(parent ? parent->winId() : winId());
    assert(parentView);
    
    NSWindow* window = [parentView window];
    assert(window);
    
    [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    
    window.movable = NO;
    window.movableByWindowBackground  = NO;
    
    if (QSysInfo().macVersion() >= QSysInfo::MV_10_10)
    {
        [window setValue:@(YES) forKey:@"titlebarAppearsTransparent"];
        [window setValue:@(1) forKey:@"titleVisibility"];   
//        window.titlebarAppearsTransparent = YES;
//        window.titleVisibility = NSWindowTitleHidden;
        window.styleMask |= (1 << 15);//NSFullSizeContentViewWindowMask;
    }
    
    [parentView addSubview:_renderView];
}
    
GraphicsPanelMacosImpl::~GraphicsPanelMacosImpl() {
    [_renderView release];
}
    
WId GraphicsPanelMacosImpl::frameId() const {
    return (WId)_renderView;
}

void GraphicsPanelMacosImpl::fullscreenModeChanged(bool fullscreen) {
    NSView* parentView = (NSView*)winId();
    assert(parentView);
    if (!parentView) { return; }
    
    NSWindow* window = [parentView window];
    assert(window);
    if (!window) { return; }

    if (fullscreen) {
        [[window standardWindowButton:NSWindowCloseButton] setHidden:NO];
    } else {
        [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    }
}
    
void GraphicsPanelMacosImpl::moveEvent(QMoveEvent* e) {
    platform_specific::GraphicsPanel::moveEvent(e);
}

void GraphicsPanelMacosImpl::resizeEvent(QResizeEvent* e) {
    platform_specific::GraphicsPanel::resizeEvent(e);
    
    const QRect windowRc = rect();
    NSRect frame;
    frame.origin.x    = windowRc.left();
    frame.origin.y    = windowRc.top();
    frame.size.width  = windowRc.width();
    frame.size.height = windowRc.height();
    
    _renderView.frame = frame;
}
    
void GraphicsPanelMacosImpl::_setPanelsAttached(bool attach) {
    if (!parent()) { return; }
    for (unsigned ix = 0; ix < _panels.size(); ix++) {
        assert(_panels[ix]);
        if (!_panels[ix]) { continue; }
        setPanelAttachedAsChild(attach, *(QWidget*)parent(), *_panels[ix]);
    }
}

void GraphicsPanelMacosImpl::showEvent(QShowEvent* e) {
    platform_specific::GraphicsPanel::showEvent(e);
    [_renderView setHidden:NO];
    _setPanelsAttached(true);
}

void GraphicsPanelMacosImpl::hideEvent(QHideEvent* e) {
    platform_specific::GraphicsPanel::hideEvent(e);
    [_renderView setHidden:YES];
    _setPanelsAttached(false);
}
    
void GraphicsPanelMacosImpl::addPanels(std::vector<QWidget*>& panels) {
    _panels = panels;
    _setPanelsAttached(true);
}
    
void GraphicsPanelMacosImpl::clearPanels() {
    _setPanelsAttached(false);
    _panels.clear();
}
}

platform_specific::GraphicsPanel* platform_macos::GraphicsPanelMacos::create(QWidget* parent, std::vector<QWidget*>& panels) {
    return new platform_macos::GraphicsPanelMacosImpl(parent, panels);
}
