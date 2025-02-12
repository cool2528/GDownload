#include "os_mac.h"
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <CFNetwork/CFNetwork.h>
#include <optional>
#include <utility>
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

std::optional<std::pair<String, int>> GetSystemHTTPProxy(){
     @autoreleasepool {
        CFStringRef http_proxyHost = NULL;
        CFNumberRef http_proxyPort = NULL;
        CFStringRef https_proxyHost = NULL;
        CFNumberRef https_proxyPort = NULL;
        CFDictionaryRef proxySettings = CFNetworkCopySystemProxySettings();
        if (proxySettings) {

            CFDictionaryGetValueIfPresent(proxySettings, kCFNetworkProxiesHTTPProxy, (const void**)&http_proxyHost);
            CFDictionaryGetValueIfPresent(proxySettings, kCFNetworkProxiesHTTPPort, (const void**)&http_proxyPort);
            CFDictionaryGetValueIfPresent(proxySettings, kCFNetworkProxiesHTTPSProxy, (const void**)&https_proxyHost);
            CFDictionaryGetValueIfPresent(proxySettings, kCFNetworkProxiesHTTPSPort, (const void**)&https_proxyPort);
            CFRelease(proxySettings);
        }
        char proxyBuffer[1024];
        int port = 0;
        if (http_proxyHost && http_proxyPort) {
            CFNumberGetValue(http_proxyPort, kCFNumberIntType, &port);
            if(CFStringGetCString(http_proxyHost, proxyBuffer, sizeof(proxyBuffer), kCFStringEncodingUTF8))
                return std::make_pair(proxyBuffer, port);
        }else if(https_proxyHost && https_proxyPort){
            CFNumberGetValue(https_proxyPort, kCFNumberIntType, &port);
            if(CFStringGetCString(https_proxyHost, proxyBuffer, sizeof(proxyBuffer), kCFStringEncodingUTF8))
                return std::make_pair(proxyBuffer, port);
        }
    }
        return std::nullopt;
}

}  // namespace mac
}  // namespace osx

}  // namespace gdl
