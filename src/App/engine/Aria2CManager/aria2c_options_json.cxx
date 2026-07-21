#include "aria2c_options_json.h"

namespace gdl {
	namespace engine {

		void AppendAria2OptionsJson(const std::unordered_multimap<std::string, std::string>& options,
									rapidjson::Value& options_value, rapidjson::Document::AllocatorType& allocator) {
			// unordered_multimap 保证同名键在迭代中相邻,按键分组一次遍历:
			// 从组内第一个元素取 equal_range,处理完整组后跳到下一组
			for (auto it = options.begin(); it != options.end();) {
				const auto range = options.equal_range(it->first);
				rapidjson::Value key_name;
				key_name.SetString(it->first.c_str(), static_cast<rapidjson::SizeType>(it->first.size()), allocator);

				const auto value_count = std::distance(range.first, range.second);
				if (value_count == 1) {
					rapidjson::Value key_value;
					key_value.SetString(range.first->second.c_str(),
										static_cast<rapidjson::SizeType>(range.first->second.size()), allocator);
					options_value.AddMember(key_name, key_value, allocator);
				} else {
					// 同名多值(如 header)聚合为字符串数组,aria2 对 cumulative 选项按数组逐条应用
					rapidjson::Value value_array(rapidjson::kArrayType);
					for (auto value_it = range.first; value_it != range.second; ++value_it) {
						rapidjson::Value item;
						item.SetString(value_it->second.c_str(),
									   static_cast<rapidjson::SizeType>(value_it->second.size()), allocator);
						value_array.PushBack(item, allocator);
					}
					options_value.AddMember(key_name, value_array, allocator);
				}
				it = range.second;
			}
		}

	}  // namespace engine
}  // namespace gdl
