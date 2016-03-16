#include "stdafx.h"
#include "NotificationCenterManager.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../utils/utils.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSDockTile.h>

static Ui::NotificationCenterManager * sharedCenter;

@interface ICQNotificationDelegate : NSObject<NSUserNotificationCenterDelegate>
@end

@implementation ICQNotificationDelegate

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didDeliverNotification:(NSUserNotification *)notification
{
    
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    if (sharedCenter)
    {
        NSString * aimId = notification.userInfo[@"aimId"];
        QString str = QString::fromCFString((__bridge CFStringRef)aimId);
        sharedCenter->Activated(str);
    }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    return YES;
}

@end

static ICQNotificationDelegate * notificationDelegate = nil;

namespace Ui
{
	NotificationCenterManager::NotificationCenterManager()
	{
        sharedCenter = this;
        
        notificationDelegate = [[ICQNotificationDelegate alloc] init];
        
        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:notificationDelegate];
	}

	NotificationCenterManager::~NotificationCenterManager()
	{
        sharedCenter = NULL;
        [notificationDelegate release];
        notificationDelegate = nil;
	}

	void NotificationCenterManager::HideNotifications(const QString& aimId)
	{
        NSArray * notifications = [[NSUserNotificationCenter defaultUserNotificationCenter] deliveredNotifications];
        
        for (NSUserNotification * notif in notifications)
        {
            NSString * aimId_ = notif.userInfo[@"aimId"];
            QString str = QString::fromCFString((__bridge CFStringRef)aimId_);
            
            if (str == aimId)
            {
                [[NSUserNotificationCenter defaultUserNotificationCenter] removeDeliveredNotification:notif];
            }
        }
	}

	void NotificationCenterManager::DisplayNotification(const QString& aimId, const QString& senderNick, const QString& message)
	{
        NSString * aimId_ = (NSString *)CFBridgingRelease(aimId.toCFString());
        
        QString displayName = Logic::GetContactListModel()->getDisplayName(aimId);
        
        NSUserNotification * notification = [[[NSUserNotification alloc] init] autorelease];
        
        if (QSysInfo().macVersion() > QSysInfo::MV_10_8)
        {
            bool isDefault = false;
            Logic::QPixmapSCptr avatar = Logic::GetAvatarStorage()->Get(aimId, displayName, Utils::scale_value(64), !Logic::GetContactListModel()->isChat(aimId), isDefault);
            
            if (avatar.get())
            {
                QByteArray bArray;
                QBuffer buffer(&bArray);
                buffer.open(QIODevice::WriteOnly);
                avatar->save(&buffer, "PNG");
                
                NSData* data = [NSData dataWithBytes:bArray.constData() length:bArray.size()];
                NSImage * ava = [[[NSImage alloc] initWithData: data] autorelease];
                
                NSImage *composedImage = [[[NSImage alloc] initWithSize:ava.size] autorelease];
                [composedImage lockFocus];
                NSRect imageFrame = NSMakeRect(0.f, 0.f, ava.size.width, ava.size.height);
                NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:imageFrame
                                                                     xRadius:ceil(ava.size.width / 2.f)
                                                                     yRadius:ceil(ava.size.width / 2.f)];
                [path addClip];
                [ava drawInRect:imageFrame fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.f];
                [composedImage unlockFocus];
                
                notification.contentImage = composedImage;
            }
        }
        
        notification.title = (NSString *)CFBridgingRelease(displayName.toCFString());
        notification.subtitle = (NSString *)CFBridgingRelease(senderNick.toCFString());
        notification.informativeText = (NSString *)CFBridgingRelease(message.toCFString());
        notification.userInfo = @{@"aimId": aimId_};
        
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
        
	}

	void NotificationCenterManager::Activated(const QString& aimId)
	{
		emit messageClicked(aimId);
	}
    
    void NotificationCenterManager::updateBadgeIcon(int unreads)
    {
        static int prevCnt = 0;
        
        NSDockTile * tile = [[NSApplication sharedApplication] dockTile];
        
        const auto text = (unreads > 999) ? QString("999+") : QVariant(unreads).toString();
        NSString *str = unreads == 0?@"":(NSString *)CFBridgingRelease(text.toCFString());
        [tile setBadgeLabel:str];
        
        if (unreads > prevCnt)
        {
            [NSApp requestUserAttention:NSInformationalRequest];
        }
        prevCnt = unreads;
    }
}