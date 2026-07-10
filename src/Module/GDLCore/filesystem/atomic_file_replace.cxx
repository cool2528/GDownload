#include "atomic_file_replace.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <streambuf>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>

#include "detail/atomic_file_replace_windows_detail.h"
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gdl {
	namespace filesystem {
		namespace {

			enum class NativeIoOperation {
				kNone,
				kWrite,
				kFlush,
				kClose,
				kMode,
			};

			std::string DescribeSystemError(const std::string& action, const std::error_code& error) {
				if (!error) return action;
				return action + " (" + error.category().name() + ":" + std::to_string(error.value()) +
					   "): " + error.message();
			}

			class NativeTemporaryFile {
			   public:
				NativeTemporaryFile() = default;
				~NativeTemporaryFile() { Close(); }
				NativeTemporaryFile(const NativeTemporaryFile&)			   = delete;
				NativeTemporaryFile& operator=(const NativeTemporaryFile&) = delete;
				NativeTemporaryFile(NativeTemporaryFile&&)				   = delete;
				NativeTemporaryFile& operator=(NativeTemporaryFile&&)	   = delete;

				bool OpenExclusive(const std::filesystem::path& path, bool& alreadyExists, std::error_code& error) {
					alreadyExists = false;
					lastError_.clear();
					lastOperation_ = NativeIoOperation::kNone;
#ifdef _WIN32
					const HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
													  FILE_ATTRIBUTE_NORMAL, nullptr);
					if (handle != INVALID_HANDLE_VALUE) {
						handle_ = handle;
						return true;
					}

					const DWORD createError = GetLastError();
					alreadyExists			= createError == ERROR_FILE_EXISTS || createError == ERROR_ALREADY_EXISTS;
					if (!alreadyExists) {
						error = std::error_code(static_cast<int>(createError), std::system_category());
					}
#else
					int descriptor = -1;
					do {
						descriptor = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0666);
					} while (descriptor < 0 && errno == EINTR);
					if (descriptor >= 0) {
						descriptor_ = descriptor;
						return true;
					}

					alreadyExists = errno == EEXIST;
					if (!alreadyExists) {
						error = std::error_code(errno, std::generic_category());
					}
#endif
					return false;
				}

				std::streamsize Write(const char* data, std::streamsize size) {
					std::streamsize totalWritten = 0;
					while (totalWritten < size) {
#ifdef _WIN32
						const auto remaining  = static_cast<std::uint64_t>(size - totalWritten);
						const DWORD requested = static_cast<DWORD>(std::min<std::uint64_t>(remaining, MAXDWORD));
						DWORD written		  = 0;
						if (WriteFile(handle_, data + totalWritten, requested, &written, nullptr) == FALSE) {
							CaptureError(NativeIoOperation::kWrite,
										 std::error_code(static_cast<int>(GetLastError()), std::system_category()));
							break;
						}
						if (written == 0) {
							CaptureError(NativeIoOperation::kWrite,
										 std::error_code(ERROR_WRITE_FAULT, std::system_category()));
							break;
						}
						totalWritten += written;
#else
						const ssize_t written =
							write(descriptor_, data + totalWritten, static_cast<std::size_t>(size - totalWritten));
						if (written < 0 && errno == EINTR) continue;
						if (written < 0) {
							CaptureError(NativeIoOperation::kWrite, std::error_code(errno, std::generic_category()));
							break;
						}
						if (written == 0) {
							CaptureError(NativeIoOperation::kWrite, std::make_error_code(std::errc::io_error));
							break;
						}
						totalWritten += written;
#endif
					}
					return totalWritten;
				}

				bool Flush() {
#ifdef _WIN32
					if (FlushFileBuffers(handle_) != FALSE) return true;
					CaptureError(NativeIoOperation::kFlush,
								 std::error_code(static_cast<int>(GetLastError()), std::system_category()));
#else
					int result = -1;
					do {
						result = fsync(descriptor_);
					} while (result < 0 && errno == EINTR);
					if (result == 0) return true;
					CaptureError(NativeIoOperation::kFlush, std::error_code(errno, std::generic_category()));
#endif
					return false;
				}

#ifndef _WIN32
				struct Identity {
					dev_t device;
					ino_t inode;
				};

				bool SetMode(mode_t mode) {
					int result = -1;
					do {
						result = fchmod(descriptor_, mode);
					} while (result < 0 && errno == EINTR);
					if (result == 0) return true;
					CaptureError(NativeIoOperation::kMode, std::error_code(errno, std::generic_category()));
					return false;
				}

				bool GetIdentity(Identity& identity) {
					struct stat status {};
					int result = -1;
					do {
						result = fstat(descriptor_, &status);
					} while (result < 0 && errno == EINTR);
					if (result == 0) {
						identity = Identity{status.st_dev, status.st_ino};
						return true;
					}
					CaptureError(NativeIoOperation::kWrite, std::error_code(errno, std::generic_category()));
					return false;
				}
#endif

				bool Close() {
#ifdef _WIN32
					if (handle_ == INVALID_HANDLE_VALUE) return true;
					const HANDLE handle = handle_;
					handle_				= INVALID_HANDLE_VALUE;
					if (CloseHandle(handle) != FALSE) return true;
					CaptureError(NativeIoOperation::kClose,
								 std::error_code(static_cast<int>(GetLastError()), std::system_category()));
#else
					if (descriptor_ < 0) return true;
					const int descriptor = descriptor_;
					descriptor_			 = -1;
					if (close(descriptor) == 0) return true;
					CaptureError(NativeIoOperation::kClose, std::error_code(errno, std::generic_category()));
#endif
					return false;
				}

				bool IsOpen() const {
#ifdef _WIN32
					return handle_ != INVALID_HANDLE_VALUE;
#else
					return descriptor_ >= 0;
#endif
				}

				const std::error_code& LastError() const { return lastError_; }
				NativeIoOperation LastOperation() const { return lastOperation_; }

			   private:
				void CaptureError(NativeIoOperation operation, const std::error_code& error) {
					lastOperation_ = operation;
					lastError_	   = error;
				}

#ifdef _WIN32
				HANDLE handle_{INVALID_HANDLE_VALUE};
#else
				int descriptor_{-1};
#endif
				std::error_code lastError_;
				NativeIoOperation lastOperation_{NativeIoOperation::kNone};
			};

			class NativeFileStreamBuffer : public std::streambuf {
			   public:
				explicit NativeFileStreamBuffer(NativeTemporaryFile& file) : file_(file) {}

			   protected:
				std::streamsize xsputn(const char* data, std::streamsize size) override {
					return file_.Write(data, size);
				}

				int_type overflow(int_type character) override {
					if (traits_type::eq_int_type(character, traits_type::eof())) {
						return traits_type::not_eof(character);
					}
					const char value = traits_type::to_char_type(character);
					return file_.Write(&value, 1) == 1 ? character : traits_type::eof();
				}

				int sync() override { return file_.Flush() ? 0 : -1; }

			   private:
				NativeTemporaryFile& file_;
			};

			class TemporaryPathGuard {
			   public:
				TemporaryPathGuard() = default;
				~TemporaryPathGuard() { Cleanup(); }
				TemporaryPathGuard(const TemporaryPathGuard&)			 = delete;
				TemporaryPathGuard& operator=(const TemporaryPathGuard&) = delete;
				TemporaryPathGuard(TemporaryPathGuard&&)				 = delete;
				TemporaryPathGuard& operator=(TemporaryPathGuard&&)		 = delete;

				void Reset(const std::filesystem::path& path) {
					path_  = path;
					state_ = State::kActive;
				}

				std::error_code Cleanup() {
					std::error_code error;
					if (state_ != State::kActive || path_.empty()) return error;
					std::filesystem::remove(path_, error);
					if (!error) path_.clear();
					return error;
				}

				void Commit() {
					state_ = State::kCommitted;
					path_.clear();
				}

				void PreserveForRecovery() { state_ = State::kPreserved; }

			   private:
				enum class State {
					kActive,
					kCommitted,
					kPreserved,
				};

				std::filesystem::path path_;
				State state_{State::kActive};
			};

#ifndef _WIN32
			class DirectoryHandle {
			   public:
				DirectoryHandle() = default;
				~DirectoryHandle() { Close(); }
				DirectoryHandle(const DirectoryHandle&)			   = delete;
				DirectoryHandle& operator=(const DirectoryHandle&) = delete;
				DirectoryHandle(DirectoryHandle&&)				   = delete;
				DirectoryHandle& operator=(DirectoryHandle&&)	   = delete;

				bool Open(const std::filesystem::path& path) {
					int flags = O_RDONLY;
#ifdef O_DIRECTORY
					flags |= O_DIRECTORY;
#endif
#ifdef O_CLOEXEC
					flags |= O_CLOEXEC;
#endif
					do {
						descriptor_ = open(path.c_str(), flags);
					} while (descriptor_ < 0 && errno == EINTR);
					if (descriptor_ >= 0) return true;
					error_ = std::error_code(errno, std::generic_category());
					return false;
				}

				bool Sync() {
					int result = -1;
					do {
						result = fsync(descriptor_);
					} while (result < 0 && errno == EINTR);
					if (result == 0) return true;
					error_ = std::error_code(errno, std::generic_category());
					return false;
				}

				bool Close() {
					if (descriptor_ < 0) return true;
					const int descriptor = descriptor_;
					descriptor_			 = -1;
					if (close(descriptor) == 0) return true;
					error_ = std::error_code(errno, std::generic_category());
					return false;
				}

				const std::error_code& Error() const { return error_; }

			   private:
				int descriptor_{-1};
				std::error_code error_;
			};
#endif

			Result<void> Success() {
				return Error(0, "");
			}

			Result<void> Fail(AtomicFileReplaceError error, const std::string& message) {
				return MakeFail(static_cast<std::int64_t>(error), message);
			}

			std::string GenerateRandomSuffix(std::error_code& error) {
				try {
					std::random_device randomDevice;
					std::array<std::uint32_t, 4> values{};
					for (auto& value : values)
						value = randomDevice();

					std::ostringstream suffix;
					suffix << std::hex << std::setfill('0');
					for (const auto value : values)
						suffix << std::setw(8) << value;
					return suffix.str();
				} catch (const std::system_error& exception) {
					error = exception.code();
				} catch (...) {
					error = std::make_error_code(std::errc::io_error);
				}
				return {};
			}

			std::filesystem::path CreateTemporaryFile(const std::filesystem::path& target, NativeTemporaryFile& file,
													  std::error_code& error) {
				const auto directory = target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();

				for (int attempt = 0; attempt < 128; ++attempt) {
					const auto suffix = GenerateRandomSuffix(error);
					if (error) return {};
					auto candidate = directory / target.filename();
					candidate += ".tmp." + suffix;

					bool alreadyExists = false;
					if (file.OpenExclusive(candidate, alreadyExists, error)) return candidate;
					if (!alreadyExists) return {};
				}

				error = std::make_error_code(std::errc::file_exists);
				return {};
			}

			Result<void> CleanupFailure(TemporaryPathGuard& guard, AtomicFileReplaceError error,
										const std::string& message) {
				const auto cleanupError = guard.Cleanup();
				if (cleanupError) {
					return Fail(
						AtomicFileReplaceError::kCleanup,
						message + "; " + DescribeSystemError("failed to clean up temporary file", cleanupError));
				}
				return Fail(error, message);
			}

			Result<void> CloseAndCleanupFailure(NativeTemporaryFile& file, TemporaryPathGuard& guard,
												AtomicFileReplaceError error, const std::string& message) {
				if (file.IsOpen() && !file.Close()) {
					return CleanupFailure(guard, AtomicFileReplaceError::kClose,
										  DescribeSystemError("failed to close temporary file", file.LastError()) +
											  "; original error: " + message);
				}
				return CleanupFailure(guard, error, message);
			}

			AtomicFileReplaceError StageForNativeFailure(const NativeTemporaryFile& file,
														 AtomicFileReplaceError fallback) {
				return file.LastOperation() == NativeIoOperation::kFlush ? AtomicFileReplaceError::kFlush : fallback;
			}

#ifndef _WIN32
			bool ReadExistingMode(const std::filesystem::path& target, std::optional<mode_t>& mode,
								  std::error_code& error) {
				struct stat status {};
				int result = -1;
				do {
					result = stat(target.c_str(), &status);
				} while (result < 0 && errno == EINTR);
				if (result == 0) {
					mode = status.st_mode & 07777;
					return true;
				}
				if (errno == ENOENT) {
					mode.reset();
					return true;
				}
				error = std::error_code(errno, std::generic_category());
				return false;
			}

			Result<void> SyncCommittedDirectory(DirectoryHandle& directory) {
				if (!directory.Sync()) {
					const auto syncError = directory.Error();
					directory.Close();
					return Fail(
						AtomicFileReplaceError::kSyncDirectory,
						DescribeSystemError("target was replaced, but the parent directory could not be synchronized; "
											"new content is visible but crash durability is uncertain",
											syncError));
				}
				if (!directory.Close()) {
					return Fail(AtomicFileReplaceError::kSyncDirectory,
								DescribeSystemError(
									"target was replaced and synchronized, but closing the parent directory failed; "
									"new content is visible",
									directory.Error()));
				}
				return Success();
			}
#endif

#ifdef _WIN32
			std::filesystem::path CreateBackupPath(const std::filesystem::path& target, std::error_code& error) {
				const auto directory = target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();
				for (int attempt = 0; attempt < 128; ++attempt) {
					const auto suffix = GenerateRandomSuffix(error);
					if (error) return {};
					auto candidate = directory / target.filename();
					candidate += ".bak." + suffix;
					const DWORD attributes = GetFileAttributesW(candidate.c_str());
					if (attributes == INVALID_FILE_ATTRIBUTES) {
						const DWORD inspectionError = GetLastError();
						if (inspectionError == ERROR_FILE_NOT_FOUND) return candidate;
						error = std::error_code(static_cast<int>(inspectionError), std::system_category());
						return {};
					}
				}
				error = std::make_error_code(std::errc::file_exists);
				return {};
			}

			Result<void> CleanupWindowsFailure(TemporaryPathGuard& temporaryGuard, TemporaryPathGuard& backupGuard,
											   const std::string& message) {
				const auto temporaryCleanupError = temporaryGuard.Cleanup();
				const auto backupCleanupError	 = backupGuard.Cleanup();
				if (temporaryCleanupError) {
					return Fail(AtomicFileReplaceError::kCleanup,
								message + "; " +
									DescribeSystemError("failed to clean up temporary file", temporaryCleanupError));
				}
				if (backupCleanupError) {
					return Fail(
						AtomicFileReplaceError::kCleanup,
						message + "; " + DescribeSystemError("failed to clean up backup file", backupCleanupError));
				}
				return Fail(AtomicFileReplaceError::kReplace, message);
			}

			Result<void> FinishWindowsReplacement(TemporaryPathGuard& temporaryGuard, TemporaryPathGuard& backupGuard) {
				temporaryGuard.Commit();
				const auto backupCleanupError = backupGuard.Cleanup();
				if (backupCleanupError) {
					return Fail(
						AtomicFileReplaceError::kCleanup,
						DescribeSystemError("target was replaced, but the old-target backup could not be removed",
											backupCleanupError));
				}
				return Success();
			}

			Result<void> RestoreBackupAfterPartialFailure(const std::filesystem::path& target,
														  const std::filesystem::path& backupPath,
														  TemporaryPathGuard& temporaryGuard,
														  TemporaryPathGuard& backupGuard, DWORD replaceError,
														  const std::string& action) {
				if (MoveFileExW(backupPath.c_str(), target.c_str(),
								MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE) {
					backupGuard.Commit();
					const auto temporaryCleanupError = temporaryGuard.Cleanup();
					if (temporaryCleanupError) {
						return Fail(AtomicFileReplaceError::kCleanup,
									DescribeSystemError(
										action + "; old target was restored, but temporary content cleanup failed",
										temporaryCleanupError));
					}
					return Fail(
						AtomicFileReplaceError::kReplace,
						DescribeSystemError(action + "; old target was restored from backup",
											std::error_code(static_cast<int>(replaceError), std::system_category())));
				}

				const DWORD recoveryError = GetLastError();
				temporaryGuard.PreserveForRecovery();
				backupGuard.PreserveForRecovery();
				return Fail(
					AtomicFileReplaceError::kReplace,
					DescribeSystemError(action + "; restoring the old target from backup failed",
										std::error_code(static_cast<int>(replaceError), std::system_category())) +
						"; " +
						DescribeSystemError("backup recovery error",
											std::error_code(static_cast<int>(recoveryError), std::system_category())) +
						"; temporary and backup paths were preserved for recovery");
			}

			Result<void> HandleReplaceFailure(const std::filesystem::path& target,
											  const std::filesystem::path& backupPath,
											  TemporaryPathGuard& temporaryGuard, TemporaryPathGuard& backupGuard,
											  DWORD replaceError, const std::string& action) {
				const auto failureAction = detail::ClassifyWindowsReplaceError(replaceError);
				if (failureAction == detail::WindowsReplaceFailureAction::kOldTargetIntact) {
					return CleanupWindowsFailure(
						temporaryGuard, backupGuard,
						DescribeSystemError(action + "; old target remains at the destination",
											std::error_code(static_cast<int>(replaceError), std::system_category())));
				}
				if (failureAction == detail::WindowsReplaceFailureAction::kRestoreBackup) {
					// ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 保证旧目标已移动到 backup 路径，此时才取得其所有权。
					backupGuard.Reset(backupPath);
					return RestoreBackupAfterPartialFailure(target, backupPath, temporaryGuard, backupGuard,
															replaceError, action);
				}
				return CleanupWindowsFailure(temporaryGuard, backupGuard,
											 DescribeSystemError(action, std::error_code(static_cast<int>(replaceError),
																						 std::system_category())));
			}

			bool TargetExistsNow(const std::filesystem::path& target, DWORD& inspectionError) {
				const DWORD attributes = GetFileAttributesW(target.c_str());
				if (attributes != INVALID_FILE_ATTRIBUTES) {
					inspectionError = ERROR_SUCCESS;
					return true;
				}
				inspectionError = GetLastError();
				return false;
			}

			Result<void> ReplaceTemporaryFile(const std::filesystem::path& temporaryPath,
											  const std::filesystem::path& target, TemporaryPathGuard& guard) {
				std::error_code backupPathError;
				const auto backupPath = CreateBackupPath(target, backupPathError);
				if (backupPathError) {
					return CleanupFailure(
						guard, AtomicFileReplaceError::kOpenTemporaryFile,
						DescribeSystemError("failed to create a unique old-target backup path", backupPathError));
				}
				TemporaryPathGuard backupGuard;

				if (ReplaceFileW(target.c_str(), temporaryPath.c_str(), backupPath.c_str(), 0, nullptr, nullptr) !=
					FALSE) {
					backupGuard.Reset(backupPath);
					return FinishWindowsReplacement(guard, backupGuard);
				}

				const DWORD initialReplaceError = GetLastError();
				if (initialReplaceError != ERROR_FILE_NOT_FOUND) {
					return HandleReplaceFailure(target, backupPath, guard, backupGuard, initialReplaceError,
												"failed to replace target file");
				}

				DWORD inspectionError = ERROR_SUCCESS;
				if (TargetExistsNow(target, inspectionError)) {
					if (ReplaceFileW(target.c_str(), temporaryPath.c_str(), backupPath.c_str(), 0, nullptr, nullptr) !=
						FALSE) {
						backupGuard.Reset(backupPath);
						return FinishWindowsReplacement(guard, backupGuard);
					}
					const DWORD retryError = GetLastError();
					return HandleReplaceFailure(target, backupPath, guard, backupGuard, retryError,
												"failed to replace target file after it appeared concurrently");
				}
				if (inspectionError != ERROR_FILE_NOT_FOUND) {
					return CleanupWindowsFailure(
						guard, backupGuard,
						DescribeSystemError(
							"failed to inspect target after replacement reported it missing",
							std::error_code(static_cast<int>(inspectionError), std::system_category())));
				}

				if (MoveFileExW(temporaryPath.c_str(), target.c_str(),
								MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE) {
					return FinishWindowsReplacement(guard, backupGuard);
				}
				const DWORD moveError = GetLastError();

				if (TargetExistsNow(target, inspectionError)) {
					if (ReplaceFileW(target.c_str(), temporaryPath.c_str(), backupPath.c_str(), 0, nullptr, nullptr) !=
						FALSE) {
						backupGuard.Reset(backupPath);
						return FinishWindowsReplacement(guard, backupGuard);
					}
					const DWORD retryError = GetLastError();
					const auto action	   = "move fallback failed (" +
										DescribeSystemError("move error", std::error_code(static_cast<int>(moveError),
																						  std::system_category())) +
										"); bounded replace retry also failed";
					return HandleReplaceFailure(target, backupPath, guard, backupGuard, retryError, action);
				}

				return CleanupWindowsFailure(
					guard, backupGuard,
					DescribeSystemError("failed to move temporary file to a missing target",
										std::error_code(static_cast<int>(moveError), std::system_category())));
			}
#else
			Result<void> ReplaceTemporaryFile(const std::filesystem::path& temporaryPath,
											  const std::filesystem::path& target, TemporaryPathGuard& guard,
											  DirectoryHandle& directory,
											  const NativeTemporaryFile::Identity& temporaryIdentity) {
				if (rename(temporaryPath.c_str(), target.c_str()) != 0) {
					const int renameError = errno;
					if (renameError == EINTR) {
						struct stat targetStatus {};
						int statusResult = -1;
						do {
							statusResult = lstat(target.c_str(), &targetStatus);
						} while (statusResult < 0 && errno == EINTR);
						if (statusResult == 0 && targetStatus.st_dev == temporaryIdentity.device &&
							targetStatus.st_ino == temporaryIdentity.inode) {
							guard.Commit();
							return SyncCommittedDirectory(directory);
						}
					}
					directory.Close();
					return CleanupFailure(guard, AtomicFileReplaceError::kReplace,
										  DescribeSystemError("failed to replace target file",
															  std::error_code(renameError, std::generic_category())));
				}

				guard.Commit();
				return SyncCommittedDirectory(directory);
			}
#endif

		}  // namespace

		Result<void> AtomicFileReplace(const std::filesystem::path& target, const AtomicFileWriter& writer) {
			const auto parent = target.parent_path();
			if (!parent.empty()) {
				std::error_code directoryError;
				std::filesystem::create_directories(parent, directoryError);
				if (directoryError) {
					return Fail(AtomicFileReplaceError::kCreateDirectory,
								"failed to create target directory: " + directoryError.message());
				}
			}

			TemporaryPathGuard temporaryGuard;
			NativeTemporaryFile temporaryFile;
			std::error_code temporaryFileError;
			const auto temporaryPath = CreateTemporaryFile(target, temporaryFile, temporaryFileError);
			if (temporaryFileError) {
				return Fail(AtomicFileReplaceError::kOpenTemporaryFile,
							DescribeSystemError("failed to create a unique temporary file", temporaryFileError));
			}
			temporaryGuard.Reset(temporaryPath);

			NativeFileStreamBuffer streamBuffer(temporaryFile);
			std::ostream output(&streamBuffer);
			Result<void> writeResult = Success();
			try {
				writeResult = writer(output);
			} catch (const std::exception& exception) {
				const auto stage = StageForNativeFailure(temporaryFile, AtomicFileReplaceError::kWrite);
				const auto message =
					temporaryFile.LastError()
						? DescribeSystemError("writer threw after native I/O failed", temporaryFile.LastError()) +
							  "; exception: " + exception.what()
						: "writer threw an exception: " + std::string(exception.what());
				return CloseAndCleanupFailure(temporaryFile, temporaryGuard, stage, message);
			} catch (...) {
				const auto stage = StageForNativeFailure(temporaryFile, AtomicFileReplaceError::kWrite);
				return CloseAndCleanupFailure(temporaryFile, temporaryGuard, stage,
											  "writer threw an unknown exception");
			}

			if (!writeResult) {
				const auto stage = StageForNativeFailure(temporaryFile, AtomicFileReplaceError::kWrite);
				auto message	 = "writer failed: " + std::string(writeResult.GetError().what());
				if (temporaryFile.LastError()) {
					message += "; " + DescribeSystemError("native I/O failed", temporaryFile.LastError());
				}
				return CloseAndCleanupFailure(temporaryFile, temporaryGuard, stage, message);
			}
			if (!output.good()) {
				const auto stage = temporaryFile.LastOperation() == NativeIoOperation::kFlush
									   ? AtomicFileReplaceError::kFlush
									   : AtomicFileReplaceError::kWrite;
				return CloseAndCleanupFailure(
					temporaryFile, temporaryGuard, stage,
					DescribeSystemError("temporary file stream entered a failed state", temporaryFile.LastError()));
			}

#ifndef _WIN32
			NativeTemporaryFile::Identity temporaryIdentity{};
			if (!temporaryFile.GetIdentity(temporaryIdentity)) {
				return CloseAndCleanupFailure(
					temporaryFile, temporaryGuard, AtomicFileReplaceError::kWrite,
					DescribeSystemError("failed to identify temporary file before replacement",
										temporaryFile.LastError()));
			}

			std::optional<mode_t> existingMode;
			std::error_code modeError;
			if (!ReadExistingMode(target, existingMode, modeError)) {
				return CloseAndCleanupFailure(temporaryFile, temporaryGuard, AtomicFileReplaceError::kWrite,
											  DescribeSystemError("failed to inspect target permissions", modeError));
			}
			if (existingMode.has_value() && !temporaryFile.SetMode(*existingMode)) {
				return CloseAndCleanupFailure(
					temporaryFile, temporaryGuard, AtomicFileReplaceError::kWrite,
					DescribeSystemError("failed to preserve target permissions", temporaryFile.LastError()));
			}
#endif

			try {
				output.flush();
			} catch (const std::exception& exception) {
				return CloseAndCleanupFailure(
					temporaryFile, temporaryGuard, AtomicFileReplaceError::kFlush,
					DescribeSystemError("failed to flush temporary file", temporaryFile.LastError()) +
						"; exception: " + exception.what());
			} catch (...) {
				return CloseAndCleanupFailure(temporaryFile, temporaryGuard, AtomicFileReplaceError::kFlush,
											  "failed to flush temporary file with an unknown exception");
			}
			if (!output.good()) {
				return CloseAndCleanupFailure(
					temporaryFile, temporaryGuard, AtomicFileReplaceError::kFlush,
					DescribeSystemError("failed to flush temporary file", temporaryFile.LastError()));
			}
			if (!temporaryFile.Close()) {
				return CleanupFailure(temporaryGuard, AtomicFileReplaceError::kClose,
									  DescribeSystemError("failed to close temporary file", temporaryFile.LastError()));
			}

#ifdef _WIN32
			return ReplaceTemporaryFile(temporaryPath, target, temporaryGuard);
#else
			DirectoryHandle parentDirectory;
			const auto directoryPath = parent.empty() ? std::filesystem::path(".") : parent;
			if (!parentDirectory.Open(directoryPath)) {
				return CleanupFailure(
					temporaryGuard, AtomicFileReplaceError::kSyncDirectory,
					DescribeSystemError("failed to open parent directory before replacement; target was not changed",
										parentDirectory.Error()));
			}
			return ReplaceTemporaryFile(temporaryPath, target, temporaryGuard, parentDirectory, temporaryIdentity);
#endif
		}

	}  // namespace filesystem
}  // namespace gdl
