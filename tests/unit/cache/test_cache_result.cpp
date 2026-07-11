#include <gtest/gtest.h>

#include <optional>
#include <memory>
#include <string>
#include <type_traits>

#include "cache/cache_result.h"

namespace {

using gdl::cache::CacheError;
using gdl::cache::CacheOperation;
using gdl::cache::CacheResult;

static_assert(!std::is_default_constructible_v<CacheResult<int>>);
static_assert(!std::is_default_constructible_v<CacheResult<void>>);

TEST(CacheResultTest, ValueSuccessPreservesValue) {
	auto result = CacheResult<int>::Success(42);

	EXPECT_TRUE(result.IsOk());
	EXPECT_FALSE(result.HasError());
	EXPECT_EQ(result.Value(), 42);
}

TEST(CacheResultTest, EmptyOptionalIsAValidSuccessValue) {
	auto result = CacheResult<std::optional<std::string>>::Success(std::nullopt);

	EXPECT_TRUE(result.IsOk());
	EXPECT_FALSE(result.Value().has_value());
}

TEST(CacheResultTest, SupportsMoveOnlySuccessValues) {
	auto result = CacheResult<std::unique_ptr<int>>::Success(std::make_unique<int>(7));

	ASSERT_TRUE(result.Value());
	EXPECT_EQ(*result.Value(), 7);
}

TEST(CacheResultTest, VoidSuccessHasNoError) {
	auto result = CacheResult<void>::Success();

	EXPECT_TRUE(result.IsOk());
	EXPECT_THROW(static_cast<void>(result.GetError()), std::logic_error);
}

TEST(CacheResultTest, FailureDescribesStructuredSqliteContext) {
	CacheError error{.operation = CacheOperation::kMigrate,
					 .primary_code = 19,
					 .extended_code = 2067,
					 .path = "C:/data/gdownload.db",
					 .context = "create unique task index"};
	auto result = CacheResult<int>::Failure(error);

	EXPECT_TRUE(result.HasError());
	EXPECT_THROW(static_cast<void>(result.Value()), std::logic_error);
	const std::string description = result.GetError().Describe();
	EXPECT_NE(description.find("migrate"), std::string::npos);
	EXPECT_NE(description.find("primary=19"), std::string::npos);
	EXPECT_NE(description.find("extended=2067"), std::string::npos);
	EXPECT_NE(description.find("C:/data/gdownload.db"), std::string::npos);
	EXPECT_NE(description.find("create unique task index"), std::string::npos);
}

TEST(CacheResultTest, OperationNamesAreStable) {
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kCreateDirectory), "create-directory");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kOpen), "open");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kConfigure), "configure");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kInspect), "inspect");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kMigrate), "migrate");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kPrepare), "prepare");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kBind), "bind");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kStep), "step");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kCommit), "commit");
	EXPECT_STREQ(gdl::cache::CacheOperationName(CacheOperation::kClose), "close");
}

TEST(CacheResultTest, DefinesEveryPersistenceOperationStage) {
	const CacheOperation operations[] = {CacheOperation::kCreateDirectory, CacheOperation::kOpen,
		CacheOperation::kConfigure, CacheOperation::kInspect, CacheOperation::kMigrate,
		CacheOperation::kPrepare, CacheOperation::kBind, CacheOperation::kStep,
		CacheOperation::kCommit, CacheOperation::kClose};
	EXPECT_EQ(std::size(operations), 10);
}

}  // namespace
