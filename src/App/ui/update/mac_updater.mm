#include "mac_updater.h"
#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

// 将 interface 声明移到全局作用域
@interface SparkleDelegate : NSObject <SPUUpdaterDelegate>
@property(nonatomic, copy) void (^checkCallback)(bool, const gdl::update::UpdateInfo&);
@property(nonatomic, copy) void (^progressCallback)(const gdl::update::UpdateProgress&);
@property(nonatomic, strong) NSString* currentVersion;
@property(nonatomic, assign) BOOL updateInProgress;
@property(nonatomic, assign) gdl::update::UpdateInfo* updateInfo;
@end

// 实现也必须在全局作用域
@implementation SparkleDelegate

- (instancetype)init {
	self = [super init];
	if (self) {
		_updateInProgress = NO;
		_updateInfo		  = new gdl::update::UpdateInfo();
	}
	return self;
}

- (void)dealloc {
	delete _updateInfo;
	[super dealloc];
}

#pragma mark - SPUUpdaterDelegate

- (void)updater:(SPUUpdater*)updater didFindValidUpdate:(SUAppcastItem*)item {
	if (_checkCallback) {
		_updateInfo->version	   = std::string([[item versionString] UTF8String]);
		_updateInfo->release_notes = std::string([[item itemDescription] UTF8String]);
		_updateInfo->download_url  = std::string([[[item fileURL] absoluteString] UTF8String]);
		_updateInfo->release_date  = std::string([[[item date] description] UTF8String]);
		_updateInfo->is_mandatory  = [item isCriticalUpdate];
		_updateInfo->package_size  = [[item contentLength] longLongValue];

		_checkCallback(true, *_updateInfo);
	}
}

- (void)updaterDidNotFindUpdate:(SPUUpdater*)updater {
	if (_checkCallback) {
		_checkCallback(false, gdl::update::UpdateInfo{});
	}
}

- (void)updater:(SPUUpdater*)updater
	willDownloadUpdate:(SUAppcastItem*)item
		   withRequest:(NSMutableURLRequest*)request {
	if (_progressCallback) {
		gdl::update::UpdateProgress progress;
		progress.stage		= gdl::update::UpdateProgress::Stage::kDownloading;
		progress.percentage = 0;
		progress.message	= "Starting update download";
		_progressCallback(progress);
	}
}

- (void)updater:(SPUUpdater*)updater willInstallUpdate:(SUAppcastItem*)item {
	if (_progressCallback) {
		gdl::update::UpdateProgress progress;
		progress.stage		= gdl::update::UpdateProgress::Stage::kInstalling;
		progress.percentage = 0;
		progress.message	= "Starting update installation";
		_progressCallback(progress);
	}
}

- (void)updater:(SPUUpdater*)updater didAbortWithError:(NSError*)error {
	if (_progressCallback) {
		gdl::update::UpdateProgress progress;
		progress.stage		= gdl::update::UpdateProgress::Stage::kFailed;
		progress.percentage = 0;
		progress.message	= std::string([[error localizedDescription] UTF8String]);
		_progressCallback(progress);
	}
	_updateInProgress = NO;
}

@end

namespace gdl {
	namespace update {

		class MacUpdaterImpl {
		   public:
			MacUpdaterImpl() {
				@autoreleasepool {
					delegate_ = [[SparkleDelegate alloc] init];
					updater_  = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES
																   updaterDelegate:delegate_
																userDriverDelegate:nil];
				}
			}

			~MacUpdaterImpl() {
				@autoreleasepool {
					[updater_ release];
					[delegate_ release];
				}
			}

			bool Initialize(const UpdateConfig& config) {
				@autoreleasepool {
					[delegate_ setCurrentVersion:[NSString stringWithUTF8String:config.current_version.c_str()]];
					
					SPUUpdater* updater = [updater_ updater];
					[updater setAutomaticallyChecksForUpdates:YES];
					[updater setUpdateCheckInterval:config.check_interval_hours * 3600];
					
					return true;
				}
			}

			void CheckForUpdates(AutoUpdater::UpdateCheckCallback callback) {
				@autoreleasepool {
					// 使用 Block 转换 std::function
					void (^objcCallback)(bool, const gdl::update::UpdateInfo&) = ^(bool found, const gdl::update::UpdateInfo& info) {
						callback(found, info);
					};
					[delegate_ setCheckCallback:objcCallback];
					[[updater_ updater] checkForUpdatesInBackground];
				}
			}

			bool StartUpdate(AutoUpdater::ProgressCallback progress_callback) {
				@autoreleasepool {
					if ([delegate_ updateInProgress]) {
						return false;
					}
					
					// 使用 Block 转换 std::function
					void (^objcProgressCallback)(const gdl::update::UpdateProgress&) = ^(const gdl::update::UpdateProgress& progress) {
						progress_callback(progress);
					};
					[delegate_ setProgressCallback:objcProgressCallback];
					[delegate_ setUpdateInProgress:YES];
					[[updater_ updater] checkForUpdates];
					return true;
				}
			}

			void CancelUpdate() {
				@autoreleasepool {
					[delegate_ setUpdateInProgress:NO];
				}
			}

			bool ApplyUpdate(bool restart_app) {
				return true;
			}

			std::string GetLastError() { return last_error_; }

		   private:
			SPUStandardUpdaterController* updater_;
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
#endif
