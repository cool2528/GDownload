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
	NSLog(@"Found update: %@", [item versionString]);
	NSLog(@"Current version: %@", [self currentVersion]);
	
	if (_checkCallback) {
		// 版本信息
		_updateInfo->version = std::string([[item versionString] UTF8String]);
		
		// 发布说明 - 可以是 URL 或者内联描述
		if ([item releaseNotesURL]) {
			_updateInfo->release_notes = std::string([[[item releaseNotesURL] absoluteString] UTF8String]);
		} else if ([item itemDescription]) {
			_updateInfo->release_notes = std::string([[item itemDescription] UTF8String]);
		}
		
		// 下载 URL
		NSURL* fileURL = [item fileURL];
		if (fileURL) {
			_updateInfo->download_url = std::string([[fileURL absoluteString] UTF8String]);
			NSLog(@"Update file URL: %@", fileURL);
		} else {
			NSLog(@"Warning: No file URL found in appcast item");
		}
		
		// 发布日期
		if ([item date]) {
			_updateInfo->release_date = std::string([[[item date] description] UTF8String]);
		}
		
		// 是否是强制更新
		_updateInfo->is_mandatory = false;
		
		// 包大小
		_updateInfo->package_size = [item contentLength];
		
		_checkCallback(true, *_updateInfo);
	}
}

- (void)updaterDidNotFindUpdate:(SPUUpdater*)updater {
	NSLog(@"=== No Update Found Debug ===");
	NSLog(@"Current version: %@", [self currentVersion]);
	NSLog(@"Feed URL: %@", [updater feedURL]);
	NSLog(@"Last check date: %@", [updater lastUpdateCheckDate]);
	NSLog(@"===========================");
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

- (void)updater:(SPUUpdater*)updater didFinishLoadingAppcast:(SUAppcast*)appcast {
	NSLog(@"=== Appcast Debug Info ===");
	NSLog(@"Current App Version: %@", [self currentVersion]);
	
	for (SUAppcastItem* item in [appcast items]) {
		NSLog(@"Comparing versions:");
		NSLog(@"- Appcast version: %@", [item versionString]);
		NSLog(@"- Current version: %@", [self currentVersion]);
		
		// 检查版本比较结果
		NSComparisonResult result = [[item versionString] compare:[self currentVersion] options:NSNumericSearch];
		NSString* compareResult = @"equal to";
		if (result == NSOrderedAscending) {
			compareResult = @"older than";
		} else if (result == NSOrderedDescending) {
			compareResult = @"newer than";
		}
		NSLog(@"Version comparison result: Appcast version is %@ current version", compareResult);
	}
	NSLog(@"========================");
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

			void ClearLastError() { last_error_.clear(); }

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

		std::string MacUpdater::GetLastError() const {
			return impl_ ? impl_->GetLastError() : std::string();
		}

		void MacUpdater::ClearLastError() {
			if (impl_) impl_->ClearLastError();
		}

	}  // namespace update
}  // namespace gdl
#endif
