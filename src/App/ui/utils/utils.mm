#include "utils.h"
#include <QWindow>
#import <Cocoa/Cocoa.h>

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
}//utils
}//ui
} //gdl
