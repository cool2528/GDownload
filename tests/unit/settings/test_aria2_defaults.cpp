#include <gtest/gtest.h>

#include "Browser/stopped_task_delete_utils.h"
#include "Settings/default_user_agent.h"
#include "Settings/setting.h"
#include "config/config_ini.h"
#include "config/config_key.h"

using gdl::ui::browser::DecideStoppedTaskDeletionAfterAria2Cleanup;
using gdl::ui::browser::ExtractAria2RpcErrorMessage;
using gdl::ui::browser::IsMissingAria2ResultError;
using gdl::ui::browser::StoppedTaskAria2CleanupStatus;
using gdl::ui::settings::DefaultBrowserUserAgent;

TEST(Aria2DefaultsTest, SplitDefaultsTo64) {
	EXPECT_EQ(std::string(gdl::config::Keys::Split.val()), "64");

	gdl::ui::settings::SplitInstance.Default();
	EXPECT_EQ(gdl::ui::settings::SplitInstance.Get(), 64);
}

// 线上事故:配置文件里 aria2c.split=0 通过了"是整数就放行"的加载校验,
// 透传给 aria2c 后被 "split must be greater than or equal to 1" 拒绝,
// 引擎从此永远起不来。整数配置必须在加载期做范围校验,越界回落默认值。
TEST(IntegerConfigRangeTest, SplitBelowOneFallsBackToDefault) {
	using gdl::config::ValidateIntegerConfigValue;
	const auto key = gdl::config::Keys::Split.get();

	EXPECT_EQ(ValidateIntegerConfigValue(key, "0", "64"), std::optional<std::string>("64"));
	EXPECT_EQ(ValidateIntegerConfigValue(key, "-3", "64"), std::optional<std::string>("64"));
	EXPECT_EQ(ValidateIntegerConfigValue(key, "65", "64"), std::optional<std::string>("64"));
	EXPECT_EQ(ValidateIntegerConfigValue(key, "1", "64"), std::nullopt);
	EXPECT_EQ(ValidateIntegerConfigValue(key, "64", "64"), std::nullopt);
}

TEST(IntegerConfigRangeTest, Aria2StartupCriticalKeysAreRangeChecked) {
	using gdl::config::ValidateIntegerConfigValue;
	using Keys = gdl::config::Keys;

	// aria2c 对这些参数有硬性下限/区间,越界会直接拒绝启动
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::MaxConnectionPerServer.get(), "0", "16"),
		std::optional<std::string>("16"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::MaxConcurrentDownloads.get(), "0", "5"),
		std::optional<std::string>("5"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::Timeout.get(), "0", "60"), std::optional<std::string>("60"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::Timeout.get(), "601", "60"), std::optional<std::string>("60"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::MinSplitSize.get(), "0", "20"), std::optional<std::string>("20"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::RpcListenPort.get(), "65536", "16888"),
		std::optional<std::string>("16888"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::RpcListenPort.get(), "16888", "16888"), std::nullopt);
}

TEST(IntegerConfigRangeTest, UnlistedIntegerKeysOnlyRequireNonNegative) {
	using gdl::config::ValidateIntegerConfigValue;
	using Keys = gdl::config::Keys;

	// 表外整数配置(如限速、bt-max-peers)仅要求非负,大值放行
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::BtMaxPeers.get(), "100000", "55"), std::nullopt);
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::BtMaxPeers.get(), "-1", "55"), std::optional<std::string>("55"));
	EXPECT_EQ(ValidateIntegerConfigValue(Keys::MaxDownloadLimit.get(), "0", "0"), std::nullopt);
}

TEST(IntegerConfigRangeTest, MalformedIntegerFallsBackToDefault) {
	using gdl::config::ValidateIntegerConfigValue;
	const auto key = gdl::config::Keys::Split.get();

	EXPECT_EQ(ValidateIntegerConfigValue(key, "abc", "64"), std::optional<std::string>("64"));
	EXPECT_EQ(ValidateIntegerConfigValue(key, "", "64"), std::optional<std::string>("64"));
	EXPECT_EQ(ValidateIntegerConfigValue(key, "12.5", "64"), std::optional<std::string>("64"));
	// 超出 long long 的溢出值同样回落默认
	EXPECT_EQ(ValidateIntegerConfigValue(key, "99999999999999999999999", "64"),
		std::optional<std::string>("64"));
}

TEST(Aria2DefaultsTest, DefaultBrowserUserAgentMatchesCurrentPlatform) {
	const QString user_agent = DefaultBrowserUserAgent();

	EXPECT_FALSE(user_agent.trimmed().isEmpty());
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Mozilla/5.0")));

#if defined(Q_OS_WIN)
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Windows NT")));
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Chrome/")) || user_agent.contains(QStringLiteral("Edg/")) ||
				user_agent.contains(QStringLiteral("Firefox/")));
	EXPECT_FALSE(user_agent.contains(QStringLiteral("Macintosh")));
#elif defined(Q_OS_MACOS)
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Macintosh")));
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Safari/")));
#else
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Linux")));
	EXPECT_TRUE(user_agent.contains(QStringLiteral("Chrome/")));
#endif
}

TEST(Aria2DefaultsTest, UserAgentSettingDefaultsToBrowserUserAgent) {
	gdl::ui::settings::UserAgentInstance.Default();

	EXPECT_EQ(gdl::ui::settings::UserAgentInstance.Get().toStdString(), DefaultBrowserUserAgent().toStdString());
}

TEST(StoppedTaskDeleteTest, OrdinaryAria2CleanupFailureStillRemovesLocalRecord) {
	const auto decision = DecideStoppedTaskDeletionAfterAria2Cleanup(
		StoppedTaskAria2CleanupStatus::kFailed, QStringLiteral("connection refused"));

	EXPECT_FALSE(decision.aria2_cleaned);
	EXPECT_TRUE(decision.show_cleanup_warning);
	EXPECT_FALSE(decision.warning_message.trimmed().isEmpty());
}

TEST(StoppedTaskDeleteTest, Aria2CleanupSuccessRemovesLocalTaskWithoutWarning) {
	const auto decision = DecideStoppedTaskDeletionAfterAria2Cleanup(
		StoppedTaskAria2CleanupStatus::kSucceeded, QString());

	EXPECT_TRUE(decision.aria2_cleaned);
	EXPECT_FALSE(decision.show_cleanup_warning);
	EXPECT_TRUE(decision.warning_message.isEmpty());
}

TEST(StoppedTaskDeleteTest, AlreadyMissingAria2ResultAllowsLocalRemoval) {
	const auto decision = DecideStoppedTaskDeletionAfterAria2Cleanup(
		StoppedTaskAria2CleanupStatus::kAlreadyMissing, QStringLiteral("GID not found"));

	EXPECT_TRUE(decision.aria2_cleaned);
}

TEST(StoppedTaskDeleteTest, HttpErrorBodyRecognizesMissingAria2Result) {
	const QString message = ExtractAria2RpcErrorMessage(
		R"({"id":"missing","jsonrpc":"2.0","error":{"code":1,"message":"GID 0123456789abcdef is not found"}})");

	EXPECT_EQ(message, QStringLiteral("GID 0123456789abcdef is not found"));
	EXPECT_TRUE(IsMissingAria2ResultError(message));
	EXPECT_TRUE(IsMissingAria2ResultError(QStringLiteral("GID 0123456789abcdef does not exist")));
}
