#include <gtest/gtest.h>
#include "App/ui/update/auto_updater.h"

namespace gdl::update {
namespace {

TEST(UpdateConfigTest, StoresPrimaryAndFallbackUpdateUrls) {
    UpdateConfig config;
    config.update_url = "https://gdownload.uk/update/latest.json";
    config.fallback_update_url = "https://api.github.com/repos/cool2528/gdownload/releases/latest";

    EXPECT_EQ(config.update_url, "https://gdownload.uk/update/latest.json");
    EXPECT_EQ(config.fallback_update_url, "https://api.github.com/repos/cool2528/gdownload/releases/latest");
}

}  // namespace
}  // namespace gdl::update
