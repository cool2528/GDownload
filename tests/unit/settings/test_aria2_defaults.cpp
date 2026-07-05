#include <gtest/gtest.h>

#include "Browser/stopped_task_delete_utils.h"
#include "Settings/default_user_agent.h"
#include "Settings/setting.h"
#include "config/config_key.h"

using gdl::ui::browser::DecideStoppedTaskDeletionAfterAria2Cleanup;
using gdl::ui::settings::DefaultBrowserUserAgent;

TEST(Aria2DefaultsTest, SplitDefaultsTo64) {
	EXPECT_EQ(std::string(gdl::config::Keys::Split.val()), "64");

	gdl::ui::settings::SplitInstance.Default();
	EXPECT_EQ(gdl::ui::settings::SplitInstance.Get(), 64);
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

TEST(StoppedTaskDeleteTest, Aria2CleanupFailureDoesNotBlockLocalRemoval) {
	const auto decision = DecideStoppedTaskDeletionAfterAria2Cleanup(false, QString());

	EXPECT_TRUE(decision.remove_local_task);
	EXPECT_TRUE(decision.show_cleanup_warning);
	EXPECT_FALSE(decision.warning_message.trimmed().isEmpty());
}

TEST(StoppedTaskDeleteTest, Aria2CleanupSuccessRemovesLocalTaskWithoutWarning) {
	const auto decision = DecideStoppedTaskDeletionAfterAria2Cleanup(true, QString());

	EXPECT_TRUE(decision.remove_local_task);
	EXPECT_FALSE(decision.show_cleanup_warning);
	EXPECT_TRUE(decision.warning_message.isEmpty());
}
