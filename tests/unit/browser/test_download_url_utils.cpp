#include <gtest/gtest.h>

#include "Browser/download_url_utils.h"

using gdl::ui::browser::AddSuggestedOutOptionForUrl;
using gdl::ui::browser::NormalizeDownloadUrlForAria2;
using gdl::ui::browser::SuggestDownloadFileNameFromUrl;

TEST(DownloadUrlUtilsTest, AddsHttpSchemeForHostOnlyUrl) {
	auto normalized = NormalizeDownloadUrlForAria2(QStringLiteral("example.com/file.zip"));

	ASSERT_TRUE(normalized.has_value());
	EXPECT_EQ(normalized.value(), QStringLiteral("http://example.com/file.zip"));
}

TEST(DownloadUrlUtilsTest, RejectsUnsupportedSchemes) {
	auto normalized = NormalizeDownloadUrlForAria2(QStringLiteral("javascript:alert(1)"));

	EXPECT_FALSE(normalized.has_value());
}

TEST(DownloadUrlUtilsTest, SuggestsFileNameFromHuggingFaceResolveUrl) {
	const QString url = QStringLiteral(
		"https://huggingface.co/HauhauCS/Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive/resolve/main/"
		"Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive-IQ3_M.gguf");

	EXPECT_EQ(SuggestDownloadFileNameFromUrl(url),
			  QStringLiteral("Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive-IQ3_M.gguf"));
}

TEST(DownloadUrlUtilsTest, AddsOutOptionFromUrlFileNameWhenMissing) {
	std::unordered_multimap<std::string, std::string> options;
	const QString url = QStringLiteral(
		"https://huggingface.co/HauhauCS/Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive/resolve/main/"
		"Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive-IQ3_M.gguf");

	AddSuggestedOutOptionForUrl(options, url);

	auto range = options.equal_range("out");
	ASSERT_NE(range.first, range.second);
	EXPECT_EQ(range.first->second, "Qwen3.6-35B-A3B-Uncensored-HauhauCS-Aggressive-IQ3_M.gguf");
}

TEST(DownloadUrlUtilsTest, KeepsExplicitOutOption) {
	std::unordered_multimap<std::string, std::string> options;
	options.emplace("out", "custom-name.bin");

	AddSuggestedOutOptionForUrl(options, QStringLiteral("https://example.com/file.zip"));

	auto range = options.equal_range("out");
	ASSERT_NE(range.first, range.second);
	EXPECT_EQ(range.first->second, "custom-name.bin");
	++range.first;
	EXPECT_EQ(range.first, range.second);
}
