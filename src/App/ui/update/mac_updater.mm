#include "mac_updater.h"
#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

namespace gdl {
namespace update {

// Sparkle代理类
@interface SparkleDelegate : NSObject <SPUUpdaterDelegate>
@property (nonatomic, copy) AutoUpdater::UpdateCheckCallback checkCallback;
@property (nonatomic, copy) AutoUpdater::ProgressCallback progressCallback;
@property (nonatomic, strong) NSString* currentVersion;
@property (nonatomic, assign) BOOL updateInProgress;
@property (nonatomic, strong) UpdateInfo* updateInfo;
@end

@implementation SparkleDelegate

- (instancetype)init {
  self = [super init];
  if (self) {
    _updateInProgress = NO;
    _updateInfo = new UpdateInfo();
  }
  return self;
}

- (void)dealloc {
  delete _updateInfo;
  [super dealloc];
}

#pragma mark - SPUUpdaterDelegate

- (void)updater:(SPUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)item {
  if (_checkCallback) {
    _updateInfo->version = std::string([[item versionString] UTF8String]);
    _updateInfo->release_notes = std::string([[item itemDescription] UTF8String]);
    _updateInfo->download_url = std::string([[[item fileURL] absoluteString] UTF8String]);
    _updateInfo->release_date = std::string([[[item date] description] UTF8String]);
    _updateInfo->is_mandatory = [item isCriticalUpdate];
    _updateInfo->package_size = [[item contentLength] longLongValue];
    
    _checkCallback(true, *_updateInfo);
  }
}

- (void)updaterDidNotFindUpdate:(SPUUpdater *)updater {
  if (_checkCallback) {
    _checkCallback(false, UpdateInfo{});
  }
}

- (void)updater:(SPUUpdater *)updater willDownloadUpdate:(SUAppcastItem *)item withRequest:(NSMutableURLRequest *)request {
  if (_progressCallback) {
    UpdateProgress progress;
    progress.stage = UpdateProgress::Stage::kDownloading;
    progress.percentage = 0;
    progress.message = "Starting update download";
    _progressCallback(progress);
  }
}

- (void)updater:(SPUUpdater *)updater willInstallUpdate:(SUAppcastItem *)item {
  if (_progressCallback) {
    UpdateProgress progress;
    progress.stage = UpdateProgress::Stage::kInstalling;
    progress.percentage = 0;
    progress.message = "Starting update installation";
    _progressCallback(progress);
  }
}

- (void)updater:(SPUUpdater *)updater didAbortWithError:(NSError *)error {
  if (_progressCallback) {
    UpdateProgress progress;
    progress.stage = UpdateProgress::Stage::kFailed;
    progress.percentage = 0;
    progress.message = std::string([[error localizedDescription] UTF8String]);
    _progressCallback(progress);
  }
  _updateInProgress = NO;
}

@end

// C++实现类
class MacUpdaterImpl {
 public:
  MacUpdaterImpl() {
    delegate_ = [[SparkleDelegate alloc] init];
    updater_ = [[SPUUpdater alloc] init];
    [updater_ setDelegate:delegate_];
  }
  
  ~MacUpdaterImpl() {
    [updater_ setDelegate:nil];
    [updater_ release];
    [delegate_ release];
  }
  
  bool Initialize(const UpdateConfig& config) {
    NSString* feedURL = [NSString stringWithUTF8String:config.update_url.c_str()];
    [updater_ setFeedURL:[NSURL URLWithString:feedURL]];
    [delegate_ setCurrentVersion:[NSString stringWithUTF8String:config.current_version.c_str()]];
    
    // 设置自动检查间隔
    [updater_ setAutomaticallyChecksForUpdates:YES];
    [updater_ setUpdateCheckInterval:config.check_interval_hours * 3600];
    
    return true;
  }
  
  void CheckForUpdates(AutoUpdater::UpdateCheckCallback callback) {
    [delegate_ setCheckCallback:callback];
    [updater_ checkForUpdatesInBackground];
  }
  
  bool StartUpdate(AutoUpdater::ProgressCallback progress_callback) {
    if ([delegate_ updateInProgress]) {
      return false;
    }
    
    [delegate_ setProgressCallback:progress_callback];
    [delegate_ setUpdateInProgress:YES];
    [updater_ checkForUpdates:nil];
    return true;
  }
  
  void CancelUpdate() {
    // Sparkle不直接支持取消，但我们可以标记状态
    [delegate_ setUpdateInProgress:NO];
  }
  
  bool ApplyUpdate(bool restart_app) {
    // Sparkle会自动处理应用更新
    return true;
  }
  
  std::string GetLastError() {
    return last_error_;
  }
  
 private:
  SPUUpdater* updater_;
  SparkleDelegate* delegate_;
  std::string last_error_;
};

// MacUpdater实现
MacUpdater::MacUpdater() : impl_(new MacUpdaterImpl()) {}

MacUpdater::~MacUpdater() = default;

bool MacUpdater::Initialize(const UpdateConfig& config) {
  return impl_->Initialize(config);
}

void MacUpdater::CheckForUpdates(UpdateCheckCallback callback) {
  impl_->CheckForUpdates(callback);
}

bool MacUpdater::StartUpdate(ProgressCallback progress_callback) {
  return impl_->StartUpdate(progress_callback);
}

void MacUpdater::CancelUpdate() {
  impl_->CancelUpdate();
}

bool MacUpdater::ApplyUpdate(bool restart_app) {
  return impl_->ApplyUpdate(restart_app);
}

}  // namespace update
}  // namespace gdl