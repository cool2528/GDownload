#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <string>

#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <toml++/toml.h>

#include "Module/GDLCore/config/detail/toml_config_save.h"
#include "Module/GDLCore/filesystem/atomic_file_replace.h"
#ifdef _WIN32
#include "Module/GDLCore/filesystem/detail/atomic_file_replace_windows_detail.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

	gdl::Result<void> WriteText(std::ostream& output, const std::string& text) {
		output << text;
		return gdl::Error(0, "");
	}

	std::string ReadText(const std::filesystem::path& path) {
		std::ifstream input(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
	}

	std::filesystem::path TemporaryRootPath(const QTemporaryDir& temporaryDirectory) {
#ifdef _WIN32
		return std::filesystem::path(temporaryDirectory.path().toStdWString());
#else
		return std::filesystem::path(temporaryDirectory.path().toUtf8().toStdString());
#endif
	}

	class FailingStreamBuffer final : public std::streambuf {
	   public:
		explicit FailingStreamBuffer(std::size_t acceptedBytes) : acceptedBytes_(acceptedBytes) {}

	   protected:
		std::streamsize xsputn(const char*, std::streamsize count) override {
			const auto remaining = acceptedBytes_ - writtenBytes_;
			const auto accepted = static_cast<std::streamsize>(
				std::min<std::size_t>(remaining, static_cast<std::size_t>(count)));
			writtenBytes_ += static_cast<std::size_t>(accepted);
			return accepted;
		}

		int_type overflow(int_type character) override {
			if (traits_type::eq_int_type(character, traits_type::eof()) || writtenBytes_ >= acceptedBytes_) {
				return traits_type::eof();
			}
			++writtenBytes_;
			return character;
		}

	   private:
		std::size_t acceptedBytes_{};
		std::size_t writtenBytes_{};
	};

	void WriteOldTomlConfig(const std::filesystem::path& path) {
		std::ofstream output(path, std::ios::binary);
		output << "[general]\nvalue = \"old-value\"\n";
	}

	void ExpectOldTomlConfig(const std::filesystem::path& path) {
		const auto parsed = toml::parse_file(path.string());
		EXPECT_EQ(parsed["general"]["value"].value<std::string>(), "old-value");
	}

#ifndef _WIN32
	class ScopedUmask {
	   public:
		explicit ScopedUmask(mode_t mask) : previousMask_(umask(mask)) {}
		~ScopedUmask() { umask(previousMask_); }
		ScopedUmask(const ScopedUmask&)			   = delete;
		ScopedUmask& operator=(const ScopedUmask&) = delete;

	   private:
		mode_t previousMask_;
	};
#endif

	TEST(AtomicFileReplaceTest, ExposesDistinctFlushCloseAndDirectorySyncErrors) {
		EXPECT_NE(static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kFlush),
				  static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kClose));
		EXPECT_NE(static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kClose),
				  static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kSyncDirectory));
	}

#ifdef _WIN32
	TEST(AtomicFileReplaceTest, WindowsReplaceErrorsChooseSafeRecoveryActions) {
		using gdl::filesystem::detail::ClassifyWindowsReplaceError;
		using gdl::filesystem::detail::WindowsReplaceFailureAction;

		EXPECT_EQ(ClassifyWindowsReplaceError(ERROR_FILE_NOT_FOUND), WindowsReplaceFailureAction::kMoveToMissingTarget);
		EXPECT_EQ(ClassifyWindowsReplaceError(ERROR_UNABLE_TO_MOVE_REPLACEMENT),
				  WindowsReplaceFailureAction::kOldTargetIntact);
		EXPECT_EQ(ClassifyWindowsReplaceError(ERROR_UNABLE_TO_MOVE_REPLACEMENT_2),
				  WindowsReplaceFailureAction::kRestoreBackup);
		EXPECT_EQ(ClassifyWindowsReplaceError(ERROR_SHARING_VIOLATION), WindowsReplaceFailureAction::kFailAndCleanup);
	}
#endif

	TEST(AtomicFileReplaceTest, CreatesNewTargetAndParentDirectories) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "nested" / "settings.toml";

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "new-content"); });

		ASSERT_TRUE(result) << result.GetError().what();
		EXPECT_EQ(ReadText(target), "new-content");
	}

	TEST(AtomicFileReplaceTest, ReplacesExistingTarget) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		{
			std::ofstream output(target, std::ios::binary);
			output << "old-content";
		}

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "replacement-content"); });

		ASSERT_TRUE(result) << result.GetError().what();
		EXPECT_EQ(ReadText(target), "replacement-content");
	}

	TEST(AtomicFileReplaceTest, WriterFailurePreservesExistingContent) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		{
			std::ofstream output(target, std::ios::binary);
			output << "old-content";
		}

		const auto result = gdl::filesystem::AtomicFileReplace(target, [](std::ostream& output) {
			output << "partial-content";
			return gdl::MakeFail(42, "writer rejected content");
		});

		ASSERT_FALSE(result);
		EXPECT_EQ(result.GetError().Code(), static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kWrite));
		EXPECT_EQ(ReadText(target), "old-content");
	}

	TEST(AtomicFileReplaceTest, TomlWriterFailureKeepsPreviouslyParsableConfig) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		WriteOldTomlConfig(target);
		const toml::table replacement{{"general", toml::table{{"value", "new-value"}}}};

		const auto result = gdl::config::detail::SaveTomlAtomically(
			target, replacement,
			[](const std::filesystem::path& path, const gdl::filesystem::AtomicFileWriter& writer) {
				return gdl::filesystem::AtomicFileReplace(
					path, [&writer](std::ostream&) {
						FailingStreamBuffer buffer(8);
						std::ostream output(&buffer);
						return writer(output);
					});
			});

		ASSERT_FALSE(result);
		EXPECT_EQ(result.GetError().Code(),
				  static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kWrite));
		EXPECT_NO_THROW(ExpectOldTomlConfig(target));
	}

	TEST(AtomicFileReplaceTest, WriterExceptionPreservesExistingContentAndCleansTemporaryFile) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto directory = TemporaryRootPath(temporaryDirectory);
		const auto target	 = directory / "settings.toml";
		{
			std::ofstream output(target, std::ios::binary);
			output << "old-content";
		}

		const auto result = gdl::filesystem::AtomicFileReplace(target, [](std::ostream& output) -> gdl::Result<void> {
			output << "partial-content";
			throw std::runtime_error("writer exploded");
		});

		ASSERT_FALSE(result);
		EXPECT_EQ(result.GetError().Code(), static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kWrite));
		EXPECT_NE(std::string(result.GetError().what()).find("writer exploded"), std::string::npos);
		EXPECT_EQ(ReadText(target), "old-content");
		const auto fileCount =
			std::distance(std::filesystem::directory_iterator(directory), std::filesystem::directory_iterator());
		EXPECT_EQ(fileCount, 1);
	}

#ifdef _WIN32
	TEST(AtomicFileReplaceTest, ReplaceFailurePreservesExistingContentAndCleansTemporaryFile) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto directory = TemporaryRootPath(temporaryDirectory);
		const auto target	 = directory / "settings.toml";
		{
			std::ofstream output(target, std::ios::binary);
			output << "old-content";
		}

		const HANDLE lockedTarget = CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
												FILE_ATTRIBUTE_NORMAL, nullptr);
		ASSERT_NE(lockedTarget, INVALID_HANDLE_VALUE);

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "replacement-content"); });
		CloseHandle(lockedTarget);

		ASSERT_FALSE(result);
		EXPECT_EQ(result.GetError().Code(),
				  static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kReplace));
		EXPECT_EQ(ReadText(target), "old-content");
		const auto fileCount =
			std::distance(std::filesystem::directory_iterator(directory), std::filesystem::directory_iterator());
		EXPECT_EQ(fileCount, 1);
	}

	TEST(AtomicFileReplaceTest, TomlReplacementFailureKeepsPreviouslyParsableConfig) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		WriteOldTomlConfig(target);
		const toml::table replacement{{"general", toml::table{{"value", "new-value"}}}};

		const HANDLE lockedTarget = CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
											FILE_ATTRIBUTE_NORMAL, nullptr);
		ASSERT_NE(lockedTarget, INVALID_HANDLE_VALUE);
		const auto result = gdl::config::detail::SaveTomlAtomically(
			target, replacement,
			[](const std::filesystem::path& path, const gdl::filesystem::AtomicFileWriter& writer) {
				return gdl::filesystem::AtomicFileReplace(path, writer);
			});
		CloseHandle(lockedTarget);

		ASSERT_FALSE(result);
		EXPECT_EQ(result.GetError().Code(),
				  static_cast<std::int64_t>(gdl::filesystem::AtomicFileReplaceError::kReplace));
		EXPECT_NO_THROW(ExpectOldTomlConfig(target));
	}
#endif

	TEST(AtomicFileReplaceTest, RepeatedWritesLeaveNoTemporaryFiles) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto directory = TemporaryRootPath(temporaryDirectory);
		const auto target	 = directory / "settings.toml";

		for (int index = 0; index < 8; ++index) {
			const auto result = gdl::filesystem::AtomicFileReplace(target, [index](std::ostream& output) {
				return WriteText(output, "content-" + std::to_string(index));
			});
			ASSERT_TRUE(result) << result.GetError().what();
		}

		EXPECT_EQ(ReadText(target), "content-7");
		const auto fileCount =
			std::distance(std::filesystem::directory_iterator(directory), std::filesystem::directory_iterator());
		EXPECT_EQ(fileCount, 1);
	}

#ifndef _WIN32
	TEST(AtomicFileReplaceTest, PreservesExistingTargetPermissions) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		{
			std::ofstream output(target, std::ios::binary);
			output << "old-content";
		}
		ASSERT_EQ(chmod(target.c_str(), 0640), 0);

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "replacement-content"); });

		ASSERT_TRUE(result) << result.GetError().what();
		struct stat status {};
		ASSERT_EQ(stat(target.c_str(), &status), 0);
		EXPECT_EQ(status.st_mode & 0777, 0640);
	}

	TEST(AtomicFileReplaceTest, NewTargetPermissionsRespectUmask) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target = TemporaryRootPath(temporaryDirectory) / "settings.toml";
		ScopedUmask mask(0027);

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "new-content"); });

		ASSERT_TRUE(result) << result.GetError().what();
		struct stat status {};
		ASSERT_EQ(stat(target.c_str(), &status), 0);
		EXPECT_EQ(status.st_mode & 0777, 0640);
	}
#endif

#ifdef _WIN32
	TEST(AtomicFileReplaceTest, SupportsUnicodeAndSpacePaths) {
		QTemporaryDir temporaryDirectory;
		ASSERT_TRUE(temporaryDirectory.isValid());
		const auto target =
			TemporaryRootPath(temporaryDirectory) / L"\u914d\u7f6e \u76ee\u5f55" / L"\u8bbe\u7f6e \u6587\u4ef6.toml";

		const auto result = gdl::filesystem::AtomicFileReplace(
			target, [](std::ostream& output) { return WriteText(output, "unicode-path-content"); });

		ASSERT_TRUE(result) << result.GetError().what();
		EXPECT_EQ(ReadText(target), "unicode-path-content");
	}
#endif

}  // namespace
