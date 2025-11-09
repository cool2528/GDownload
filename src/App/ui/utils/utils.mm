#include "utils.h"
#include <QWindow>
#include <cmath>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

@interface GDLTaskbarProgressView : NSView
@property(nonatomic) double progress;
@end

@implementation GDLTaskbarProgressView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) {
        _progress = -1.0;
    }
    return self;
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)setProgress:(double)progress
{
    double target = progress;
    if (!std::isfinite(progress) || progress < 0.0) {
        target = -1.0;
    } else if (progress > 1.0) {
        target = 1.0;
    }

    if (_progress == target) {
        return;
    }

    _progress = target;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    NSImage* icon = [NSApp applicationIconImage];
    if (icon) {
        [icon drawInRect:self.bounds];
    }

    if (_progress < 0.0) {
        return;
    }

    const CGFloat horizontalInset = NSWidth(self.bounds) * 0.12;
    const CGFloat barHeight = MAX(6.0, NSHeight(self.bounds) * 0.15);
    NSRect barRect = NSMakeRect(horizontalInset,
                                NSHeight(self.bounds) - barHeight - horizontalInset,
                                NSWidth(self.bounds) - horizontalInset * 2,
                                barHeight);

    [[NSColor colorWithCalibratedWhite:0 alpha:0.45] setFill];
    NSBezierPath* backgroundPath =
        [NSBezierPath bezierPathWithRoundedRect:barRect xRadius:barHeight / 2 yRadius:barHeight / 2];
    [backgroundPath fill];

    NSRect fillRect = barRect;
    fillRect.size.width = barRect.size.width * _progress;
    [[NSColor colorWithCalibratedRed:0.2 green:0.65 blue:0.16 alpha:0.9] setFill];
    NSBezierPath* fillPath =
        [NSBezierPath bezierPathWithRoundedRect:fillRect xRadius:barHeight / 2 yRadius:barHeight / 2];
    [fillPath fill];
}

@end

namespace gdl {

namespace ui {

namespace utils {
void hideWindowStandardButtons(WId wid) {
    @autoreleasepool {
        if (wid) {
            NSView *view = reinterpret_cast<NSView *>(wid);
            NSWindow *nsWindow = [view window];
            if (nsWindow) {
                [nsWindow standardWindowButton:NSWindowCloseButton].hidden = YES;     // 隐藏关闭按钮
                [nsWindow standardWindowButton:NSWindowMiniaturizeButton].hidden = YES; // 隐藏最小化按钮
                [nsWindow standardWindowButton:NSWindowZoomButton].hidden = YES;       // 隐藏最大化按钮
            }
        }
    }
}
void setTaskbarProgress(double progress)
{
    @autoreleasepool {
        NSDockTile* dockTile = [NSApp dockTile];
        static GDLTaskbarProgressView* progressView = nil;

        const bool shouldReset = !std::isfinite(progress) || progress < 0.0 || progress >= 1.0;
        if (shouldReset) {
            [dockTile setBadgeLabel:nil];
            [dockTile setShowsApplicationBadge:NO];
            if (progressView) {
                [dockTile setContentView:nil];
                progressView = nil;
                [dockTile display];
            }
            return;
        }

        if (!progressView) {
            progressView = [[GDLTaskbarProgressView alloc] initWithFrame:NSMakeRect(0, 0, dockTile.size.width, dockTile.size.height)];
            [dockTile setContentView:progressView];
        } else if (!NSEqualSizes(progressView.bounds.size, dockTile.size)) {
            [progressView setFrameSize:dockTile.size];
        }

        progressView.progress = progress;
        [dockTile setShowsApplicationBadge:NO];
        [dockTile setBadgeLabel:nil];
        [dockTile display];
    }
}
}//utils
}//ui
} //gdl
