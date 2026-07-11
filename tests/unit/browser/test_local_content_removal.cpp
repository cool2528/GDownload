#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <filesystem>
#include <system_error>

#include "Browser/local_content_removal.h"

namespace {

using gdl::ui::browser::LocalRemovalStatus;
using gdl::ui::browser::RemoveLocalContent;

std::filesystem::path ToFilesystemPath(const QString& path) {
#ifdef Q_OS_WIN
	return std::filesystem::path(path.toStdWString());
#else
	return std::filesystem::path(path.toStdString());
#endif
}

TEST(LocalContentRemovalTest, ReportsMissingPathAsNotFound) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	const QString missing_path = temporary_dir.filePath(QStringLiteral("missing.bin"));

	const auto result = RemoveLocalContent(missing_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kNotFound);
	EXPECT_EQ(result.path, missing_path);
	EXPECT_TRUE(result.error.isEmpty());
	ASSERT_TRUE(result.removed_count.has_value());
	EXPECT_EQ(result.removed_count.value(), 0);
	EXPECT_EQ(result.error_code, 0);
	EXPECT_FALSE(result.partial_possible);
}

TEST(LocalContentRemovalTest, RemovesRegularFile) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	const QString file_path = temporary_dir.filePath(QStringLiteral("download.bin"));
	QFile file(file_path);
	ASSERT_TRUE(file.open(QIODevice::WriteOnly));
	ASSERT_EQ(file.write("content"), 7);
	file.close();

	const auto result = RemoveLocalContent(file_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kRemoved);
	EXPECT_EQ(result.path, file_path);
	EXPECT_TRUE(result.error.isEmpty());
	ASSERT_TRUE(result.removed_count.has_value());
	EXPECT_EQ(result.removed_count.value(), 1);
	EXPECT_FALSE(QFile::exists(file_path));
}

TEST(LocalContentRemovalTest, RemovesNestedDirectoryRecursively) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	const QString directory_path = temporary_dir.filePath(QStringLiteral("download"));
	const QString nested_path = QDir(directory_path).filePath(QStringLiteral("nested"));
	ASSERT_TRUE(QDir().mkpath(nested_path));
	QFile nested_file(QDir(nested_path).filePath(QStringLiteral("piece.bin")));
	ASSERT_TRUE(nested_file.open(QIODevice::WriteOnly));
	nested_file.close();

	const auto result = RemoveLocalContent(directory_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kRemoved);
	EXPECT_EQ(result.path, directory_path);
	EXPECT_TRUE(result.error.isEmpty());
	ASSERT_TRUE(result.removed_count.has_value());
	EXPECT_EQ(result.removed_count.value(), 3);
	EXPECT_FALSE(QFile::exists(directory_path));
}

TEST(LocalContentRemovalTest, RemovesDirectorySymlinkWithoutRemovingTarget) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	const QString target_path = temporary_dir.filePath(QStringLiteral("target"));
	const QString target_file_path = QDir(target_path).filePath(QStringLiteral("content.bin"));
	const QString link_path = temporary_dir.filePath(QStringLiteral("directory-link"));
	ASSERT_TRUE(QDir().mkpath(target_path));
	QFile target_file(target_file_path);
	ASSERT_TRUE(target_file.open(QIODevice::WriteOnly));
	target_file.close();

	std::error_code link_error;
	std::filesystem::create_directory_symlink(
		ToFilesystemPath(target_path), ToFilesystemPath(link_path), link_error);
	if (link_error) {
		GTEST_SKIP() << "Directory symlink creation is unavailable: " << link_error.message();
	}

	const auto result = RemoveLocalContent(link_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kRemoved);
	ASSERT_TRUE(result.removed_count.has_value());
	EXPECT_EQ(result.removed_count.value(), 1);
	EXPECT_FALSE(QFileInfo(link_path).isSymLink());
	EXPECT_TRUE(QDir(target_path).exists());
	EXPECT_TRUE(QFile::exists(target_file_path));
}

TEST(LocalContentRemovalTest, RemovesDanglingDirectorySymlink) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	const QString target_path = temporary_dir.filePath(QStringLiteral("target"));
	const QString link_path = temporary_dir.filePath(QStringLiteral("dangling-directory-link"));
	ASSERT_TRUE(QDir().mkpath(target_path));

	std::error_code link_error;
	std::filesystem::create_directory_symlink(
		ToFilesystemPath(target_path), ToFilesystemPath(link_path), link_error);
	if (link_error) {
		GTEST_SKIP() << "Directory symlink creation is unavailable: " << link_error.message();
	}
	ASSERT_TRUE(QDir(target_path).removeRecursively());
	ASSERT_TRUE(QFileInfo(link_path).isSymLink());

	const auto result = RemoveLocalContent(link_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kRemoved);
	ASSERT_TRUE(result.removed_count.has_value());
	EXPECT_EQ(result.removed_count.value(), 1);
	EXPECT_FALSE(QFileInfo(link_path).isSymLink());
}

#ifdef Q_OS_WIN
TEST(LocalContentRemovalTest, ReportsFailureForOpenFile) {
	QTemporaryDir temporary_dir;
	ASSERT_TRUE(temporary_dir.isValid());
	QTemporaryFile open_file(temporary_dir.filePath(QStringLiteral("locked-XXXXXX.bin")));
	ASSERT_TRUE(open_file.open());
	const QString file_path = open_file.fileName();

	const auto result = RemoveLocalContent(file_path);

	EXPECT_EQ(result.status, LocalRemovalStatus::kFailed);
	EXPECT_EQ(result.path, file_path);
	EXPECT_FALSE(result.error.isEmpty());
	EXPECT_NE(result.error_code, 0);
	EXPECT_FALSE(result.removed_count.has_value());
	EXPECT_TRUE(result.partial_possible);
	EXPECT_TRUE(QFile::exists(file_path));
}
#endif

}  // namespace
