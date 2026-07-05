#include <gtest/gtest.h>

#include "Browser/download_url_utils.h"

using gdl::ui::browser::NormalizeDownloadUrlForAria2;

TEST(DownloadUrlUtilsTest, AddsHttpSchemeForHostOnlyUrl) {
	auto normalized = NormalizeDownloadUrlForAria2(QStringLiteral("example.com/file.zip"));

	ASSERT_TRUE(normalized.has_value());
	EXPECT_EQ(normalized.value(), QStringLiteral("http://example.com/file.zip"));
}

TEST(DownloadUrlUtilsTest, RejectsUnsupportedSchemes) {
	auto normalized = NormalizeDownloadUrlForAria2(QStringLiteral("javascript:alert(1)"));

	EXPECT_FALSE(normalized.has_value());
}
