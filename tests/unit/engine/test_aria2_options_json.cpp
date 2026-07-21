// aria2 RPC options 序列化测试
// 背景:插件(如夸克网盘)通过 multimap 传递多条同名 header(Cookie/Referer),
// 序列化为 JSON 对象时若逐条 AddMember 会产生重复键,aria2 解析时仅保留一条,
// 导致鉴权头丢失而下载 403。正确行为:同名多值键须输出为 JSON 字符串数组。
#include <gtest/gtest.h>

#include <rapidjson/document.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "Aria2CManager/aria2c_options_json.h"

namespace {

	using gdl::engine::AppendAria2OptionsJson;

	// 构造承载 options 的 JSON 对象
	rapidjson::Document MakeOptionsDocument(const std::unordered_multimap<std::string, std::string>& options) {
		rapidjson::Document doc;
		doc.SetObject();
		AppendAria2OptionsJson(options, doc, doc.GetAllocator());
		return doc;
	}

	// 收集 JSON 数组成员为 vector<string>,便于无序比较
	std::vector<std::string> ArrayToVector(const rapidjson::Value& value) {
		std::vector<std::string> out;
		for (const auto& item : value.GetArray()) {
			out.emplace_back(item.GetString());
		}
		std::sort(out.begin(), out.end());
		return out;
	}

	TEST(Aria2OptionsJsonTest, MultiValueKeySerializedAsArray) {
		std::unordered_multimap<std::string, std::string> options;
		options.emplace("header", "Cookie:__puus=abc");
		options.emplace("header", "Referer:https://pan.quark.cn/");

		const auto doc = MakeOptionsDocument(options);

		// 同名键只能出现一次
		ASSERT_EQ(doc.MemberCount(), 1u);
		ASSERT_TRUE(doc.HasMember("header"));
		// 多值必须为数组,两条 header 都保留
		ASSERT_TRUE(doc["header"].IsArray());
		const auto values = ArrayToVector(doc["header"]);
		const std::vector<std::string> expected{"Cookie:__puus=abc", "Referer:https://pan.quark.cn/"};
		EXPECT_EQ(values, expected);
	}

	TEST(Aria2OptionsJsonTest, SingleValueKeySerializedAsString) {
		std::unordered_multimap<std::string, std::string> options;
		options.emplace("dir", "D:/Downloads");

		const auto doc = MakeOptionsDocument(options);

		ASSERT_EQ(doc.MemberCount(), 1u);
		ASSERT_TRUE(doc.HasMember("dir"));
		// 单值保持字符串,与既有 aria2 选项传参行为兼容
		ASSERT_TRUE(doc["dir"].IsString());
		EXPECT_STREQ(doc["dir"].GetString(), "D:/Downloads");
	}

	TEST(Aria2OptionsJsonTest, EmptyOptionsProducesEmptyObject) {
		const auto doc = MakeOptionsDocument({});

		EXPECT_TRUE(doc.IsObject());
		EXPECT_EQ(doc.MemberCount(), 0u);
	}

	TEST(Aria2OptionsJsonTest, QuarkStyleOptionsKeepAllHeaders) {
		// 还原夸克网盘任务的真实 options 组合
		std::unordered_multimap<std::string, std::string> options;
		options.emplace("header", "Cookie:__puus=abc; __pus=def");
		options.emplace("header", "Referer:https://pan.quark.cn/");
		options.emplace("user-agent", "quark-cloud-drive/2.5.20");
		options.emplace("dir", "D:/Downloads");
		options.emplace("out", "movie.mp4");

		const auto doc = MakeOptionsDocument(options);

		// header 之外的键一律单值字符串
		ASSERT_EQ(doc.MemberCount(), 4u);
		ASSERT_TRUE(doc["header"].IsArray());
		EXPECT_EQ(doc["header"].Size(), 2u);
		ASSERT_TRUE(doc["user-agent"].IsString());
		EXPECT_STREQ(doc["user-agent"].GetString(), "quark-cloud-drive/2.5.20");
		ASSERT_TRUE(doc["dir"].IsString());
		ASSERT_TRUE(doc["out"].IsString());
	}

}  // namespace
