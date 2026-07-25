#pragma once

namespace gdl {
	namespace update {

		// 更新检查请求的合并决策结果
		struct UpdateCheckRequestDecision {
			bool start_new_check{false};  // 是否需要真正发起一次底层检查
			bool silent{true};            // 合并后本轮在途检查的静默属性
		};

		// 合并并发的更新检查请求:同一时刻只允许一个底层检查在途,避免并发
		// 检查交错覆盖共享状态(静默标志/last_error_)。
		// in_flight: 当前是否已有检查在途
		// in_flight_silent: 在途检查当前的静默属性(仅 in_flight 时有意义)
		// requested_silent: 新请求的静默属性
		// 规则:空闲时按请求发起;在途时不再发起第二次,但手动(非静默)请求会把
		// 在途检查升级为非静默,使其完成回执发射给设置页;反向不降级
		inline UpdateCheckRequestDecision CoalesceUpdateCheckRequest(bool in_flight, bool in_flight_silent,
																	 bool requested_silent) {
			if (!in_flight) {
				return {.start_new_check = true, .silent = requested_silent};
			}
			return {.start_new_check = false, .silent = in_flight_silent && requested_silent};
		}

	}  // namespace update
}  // namespace gdl
