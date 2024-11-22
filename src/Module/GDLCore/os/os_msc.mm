#include "os_mac.h"
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
namespace gdl {
namespace os {
namespace mac {
static String GetDirectory(NSSearchPathDirectory directoryType) {
    @autoreleasepool {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(directoryType, NSUserDomainMask, YES);
        if ([paths count] > 0) {
            NSString *path = [paths objectAtIndex:0];
            return String([path UTF8String]);
        }
    }
    return "";
}
String GetUserHomeDir() {
    return GetDirectory(NSUserDirectory);
}

String GetUserDocumentsDir() {
    return GetDirectory(NSDocumentDirectory);
}

String GetUserDownloadsDir() {
    return GetDirectory(NSDownloadsDirectory);
}

String GetUserDesktopDir() {
   return GetDirectory(NSDesktopDirectory);
}

String GetUserVideosDir() {
    return GetDirectory(NSMoviesDirectory);
}

String GetUserMusicDir() {
     return GetDirectory(NSMusicDirectory);
}

String GetUserPicturesDir() {
    return GetDirectory(NSPicturesDirectory);
}

String GetAppDataDir() {
   return GetDirectory(NSApplicationSupportDirectory);
}

String GetTempDir() {
    return GetDirectory(NSItemReplacementDirectory);
}

String GetExecutableDir() {
    @autoreleasepool {
        NSString *executablePath = [[NSBundle mainBundle] executablePath];
        NSString *executableDir = [executablePath stringByDeletingLastPathComponent];
        return String([executableDir UTF8String]);
    }
    return String();
}

String GetCurrentWorkingDir() {
    @autoreleasepool {
        NSString *cwd = [[NSFileManager defaultManager] currentDirectoryPath];
        return String([cwd UTF8String]);
    }
}
}  // namespace mac
}  // namespace osx

}  // namespace gdl
