#include <string>

#include <gtest/gtest.h>

#include "Module/GDLCore/config/config_ini.h"
#include "version.h"

namespace {

TEST(CoreConfigurationTest, ConvertsSupportedScalarValues) {
    using gdl::config::detail::FromString;
    using gdl::config::detail::ToStringValue;

    EXPECT_EQ(ToStringValue(std::string("download")), "download");
    EXPECT_EQ(ToStringValue(true), "true");
    EXPECT_EQ(ToStringValue(false), "false");
    EXPECT_EQ(ToStringValue(-42), "-42");

    EXPECT_EQ(FromString<std::string>("download"), std::optional<std::string>("download"));
    EXPECT_EQ(FromString<int>("-42"), std::optional<int>(-42));
    EXPECT_EQ(FromString<double>("3.5"), std::optional<double>(3.5));
}

TEST(CoreConfigurationTest, ParsesBooleanAliasesAndRejectsInvalidValues) {
    using gdl::config::detail::FromString;

    for (const char* value : {"true", "1", "yes", "on", "TRUE", "On"}) {
        EXPECT_EQ(FromString<bool>(value), std::optional<bool>(true)) << value;
    }
    for (const char* value : {"false", "0", "no", "off", "FALSE", "Off"}) {
        EXPECT_EQ(FromString<bool>(value), std::optional<bool>(false)) << value;
    }

    EXPECT_EQ(FromString<bool>("enabled"), std::nullopt);
    EXPECT_EQ(FromString<int>("not-a-number"), std::nullopt);
    EXPECT_EQ(FromString<double>("not-a-number"), std::nullopt);
}

TEST(CoreVersionTest, GeneratedVersionStringMatchesNumericComponents) {
    EXPECT_GE(GDownload_VERSION_MAJOR, 0);
    EXPECT_GE(GDownload_VERSION_MINOR, 0);
    EXPECT_GE(GDownload_VERSION_BUILD, 0);
    EXPECT_FALSE(std::string(GDownload_VERSION_COMMIT).empty());

    const std::string expected = std::to_string(GDownload_VERSION_MAJOR) + "." +
                                 std::to_string(GDownload_VERSION_MINOR) + "." +
                                 std::to_string(GDownload_VERSION_BUILD) + "." +
                                 std::string(GDownload_VERSION_COMMIT);
    EXPECT_EQ(std::string(GDownload_VERSION_STRING), expected);
}

}  // namespace
